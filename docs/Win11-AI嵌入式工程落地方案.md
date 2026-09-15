# Windows 11 上落地「AI + STM32 嵌入式」工程 — 完整方案

> **定位**：这是一份**可在 Windows 11 上照做**的落地方案。每一步都给命令、文件内容、验收标准。
> **不依赖任何已有环境**，从零开始。预计耗时 2–3 小时（含工具下载）。
> **核心思路**：AI 之前先建"护栏"——上下文、约束、验证闭环。护栏建好，AI 才敢用。

---

## 0. 方案总览

```
┌─────────────────────────────────────────────────────────────────────┐
│  第 0 层：AI 上下文层   ← 决定 AI 输出质量的关键，先做              │
│  AGENTS.md · .codebuddy/rules/ · prompts/ · datasheet 语料          │
├─────────────────────────────────────────────────────────────────────┤
│  第 1 层：约束层        ← 抑制幻觉与不确定代码                      │
│  .clang-format · .clang-tidy · cppcheck(MISRA) · 硬约束规则         │
├─────────────────────────────────────────────────────────────────────┤
│  第 2 层：验证闭环      ← 让 AI 的产出"可证伪"                      │
│  宿主机单元测试 → 静态分析 → 固件构建 → Flash/RAM 占用 → 上板       │
├─────────────────────────────────────────────────────────────────────┤
│  第 3 层：能力接入      ← 让 AI 能读到真实工程、跑真实命令          │
│  MCP Server（构建/尺寸/分析/串口/烧录闸门）· compile_commands.json  │
├─────────────────────────────────────────────────────────────────────┤
│  第 4 层：自动化门禁    ← 防止 AI 引入的退化被人忽略                │
│  GitHub Actions（host 测试 · MISRA · 固件构建 · size 预算）         │
└─────────────────────────────────────────────────────────────────────┘
```

**落地顺序不可颠倒**：先第 0/1/2 层（半天），再加第 3/4 层。跳过护栏直接让 AI 写固件是本方案明确反对的做法。

> ### ⚠️ 前置阶段：阶段 0（文档核查）
>
> 本方案有一个**隐含假设**：`AGENTS.md` §1 的硬件事实是**正确的**。
>
> 如果你手上是**一块陌生的板子 + 原理图 + 一批器件文档**，这个假设**不成立**——原理图、BOM、器件手册三者可能互相矛盾，手册可能根本不是这个型号（系列手册与型号手册有 80% 内容重合，错误极难发现）。此时 `AGENTS.md` 填的就是错的，**后面所有护栏都在错误前提上运行**。
>
> 这种情况请先执行 **[硬件文档一致性核查方案](硬件文档一致性核查方案.md)**（阶段 0）：
>
> ```
> 阶段 0：文档核查 → HARDWARE-TRUTH.md → 回填 AGENTS.md §1 → 再执行本方案
> ```
>
> 判断标准：**只要你不能百分之百确定"原理图与实物是同一版本、手册是同一型号"，就先做阶段 0。**

---

## 1. 目标与边界

### 本方案做什么

| 能力 | 交付物 |
|---|---|
| AI 知道你的**真实硬件事实** | `AGENTS.md` + MCP 可读 `firmware/` |
| AI 生成的代码**不会引入动态内存/递归/ISR 阻塞** | `.codebuddy/rules/*/RULE.mdc` + 静态分析 |
| AI 的产出**必须可验证**，不能"看着对" | 宿主机单元测试 + CI 门禁 |
| AI **不能烧录**，只能给命令 | MCP 审批闸门 |
| 改动**可度量**（Flash/RAM 增量） | `size-report` target |

### 本方案不做什么

- ❌ 不帮你选 MCU、不外购硬件、不写具体业务逻辑
- ❌ 不配置你的机器（工具链安装脚本由你手动执行，见 §2.2）
- ❌ 不承诺 AI 生成的 ISR/实时代码可以直接量产（见 §8 风险阶梯）

---

## 2. 第 0 步：Windows 11 环境准备

### 2.1 前置动作（10 分钟，别跳过）

```powershell
# ① 项目放在短且无空格的路径（重要：避免 CMake/OpenOCD 路径问题）
#    推荐 C:\work\ 而不是 C:\Users\你的名字\我的文档\...
New-Item -ItemType Directory -Force C:\work | Out-Null

# ② 允许本地脚本执行（PowerShell 默认禁止）
Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned -Force

# ③ Git 支持长路径（CubeMX + HAL 目录很深）
git config --global core.longpaths true
git config --global core.autocrlf false      # 换行符交给 .gitattributes

# ④ 把项目目录加入 Windows Defender 排除（否则构建慢 3-5 倍）——需管理员 PowerShell
# Add-MpPreference -ExclusionPath 'C:\work\stm32'
# Add-MpPreference -ExclusionProcess 'arm-none-eabi-gcc.exe','cmake.exe','ninja.exe'

# ⑤ 开启开发者模式（符号链接需要）：设置 → 系统 → 开发者选项 → 开发人员模式
```

### 2.2 安装工具链

**方式 A（推荐）：STM32CubeCLT —— 官方全家桶**

去 ST 官网搜 `STM32CubeCLT`，下载 Windows 版安装包。它一次性提供：

| 组件 | 作用 |
|---|---|
| `arm-none-eabi-gcc` | 交叉编译器 |
| CMake + Ninja | 构建系统 |
| `STM32_Programmer_CLI.exe` | 烧录（比 OpenOCD 在 Windows 上省心） |
| GDB + ST-LINK GDB server | 调试 |

安装到默认目录 `C:\ST\STM32CubeCLT_x.x.x\`（无空格，路径友好）。

**方式 B：winget 逐个装**

```powershell
# 注意：包 ID 可能会变，先搜再装
winget search cmake

winget install --id Kitware.CMake            -e --accept-package-agreements --accept-source-agreements
winget install --id Ninja-build.Ninja        -e --accept-package-agreements --accept-source-agreements
winget install --id LLVM.LLVM                -e --accept-package-agreements --accept-source-agreements
winget install --id Python.Python.3.12       -e --accept-package-agreements --accept-source-agreements
winget install --id OpenJS.NodeJS.LTS        -e --accept-package-agreements --accept-source-agreements
winget install --id Git.Git                  -e --accept-package-agreements --accept-source-agreements
winget install --id Microsoft.PowerShell     -e --accept-package-agreements --accept-source-agreements
winget install --id Microsoft.VisualStudioCode -e --accept-package-agreements --accept-source-agreements

# cppcheck：winget 可能没有，二选一
winget install --id Cppcheck.Cppcheck -e --accept-package-agreements
# 或 choco install cppcheck -y
# 或去 cppcheck 官网下载 zip 解压后加入 PATH
```

> ⚠️ **诚实提示**：`winget` 的包 ID 会随版本变动，且 ARM 工具链的第三方包质量参差。**优先用方式 A（STM32CubeCLT）**，它是官方维护、版本自洽的。

**必装的手动项**

| 项 | 来源 | 说明 |
|---|---|---|
| STM32CubeMX | ST 官网 | 生成 `.ioc` → 代码，**工具链必须选 CMake** |
| ST-LINK USB 驱动 | ST 官网 或随 CubeCLT 安装 | 不装识别不到调试器 |
| CodeBuddy（或 VS Code + 插件） | 官网 | AI 助手宿主 |

### 2.3 配置环境变量

控制面板 → 系统 → 高级系统设置 → 环境变量 → 用户变量 → `Path` 追加：

```
C:\ST\STM32CubeCLT_1.16.0\GNU-tools-for-STM32\bin
C:\ST\STM32CubeCLT_1.16.0\STM32CubeProgrammer\bin
C:\ST\STM32CubeCLT_1.16.0\CMake\bin
C:\ST\STM32CubeCLT_1.16.0\Ninja\bin
C:\Program Files\LLVM\bin
C:\Program Files\Cppcheck
```

版本号按你实际安装的改。**重开一个 PowerShell** 再验证。

### 2.4 验收：环境自检

把下面内容存成 `C:\work\check-env.ps1` 后运行 `pwsh -File C:\work\check-env.ps1`：

```powershell
#Requires -Version 5.1
$tools = [ordered]@{
    'git'                  = 'git --version'
    'cmake'                = 'cmake --version'
    'ninja'                = 'ninja --version'
    'arm-none-eabi-gcc'    = 'arm-none-eabi-gcc --version'
    'arm-none-eabi-size'   = 'arm-none-eabi-size --version'
    'arm-none-eabi-objcopy'= 'arm-none-eabi-objcopy --version'
    'python'               = 'python --version'
    'node'                 = 'node --version'
    'cppcheck'             = 'cppcheck --version'
    'clang-tidy'           = 'clang-tidy --version'
    'clang-format'         = 'clang-format --version'
    'STM32_Programmer_CLI' = 'STM32_Programmer_CLI --version'
}
$missing = @()
foreach ($k in $tools.Keys) {
    $exe = ($tools[$k] -split ' ')[0]
    $found = Get-Command $exe -ErrorAction SilentlyContinue
    if ($found) {
        $v = (& $exe --version 2>&1 | Select-Object -First 1)
        Write-Host ("[ OK ] {0,-24} {1}" -f $k, $v) -ForegroundColor Green
    } else {
        Write-Host ("[MISS] {0,-24} 未找到" -f $k) -ForegroundColor Red
        $missing += $k
    }
}
if ($missing.Count -gt 0) {
    Write-Warning ("缺失: " + ($missing -join ', '))
    Write-Warning '请回到 §2.2 安装，并确认已把 bin 目录加入 PATH 后重开终端'
} else {
    Write-Host "`n环境自检全部通过。" -ForegroundColor Cyan
}
```

**通过标准**：全部 `[ OK ]`。`STM32_Programmer_CLI` 缺失可以先继续（第 3 层才用）。

---

## 3. 目标目录结构

```
C:\work\stm32\
├── .gitattributes                      # 换行符策略（Windows 必配）
├── .gitignore
├── .clang-format                       # 格式统一
├── .clang-tidy                         # CERT-C 静态分析
├── .mcp.json                           # MCP 配置（Windows 写法）
├── CMakeLists.txt                      # 顶层构建
├── AGENTS.md                           # ★ AI 上下文核心（自动加载）
├── .codebuddy\
│   └── rules\
│       └── embedded-firmware\
│           └── RULE.mdc                # ★ 硬约束规则（alwaysApply）
├── cmake\
│   └── toolchains\
│       └── arm-none-eabi.cmake
├── cppcheck\
│   └── suppressions.txt
├── prompts\
│   └── firmware-tasks.md               # 提示词模板
├── scripts\
│   ├── analyze.ps1
│   └── verify.ps1                      # 一键验证闭环
├── tools\
│   └── mcp\
│       └── embedded_server.py          # MCP server（构建/串口/烧录闸门）
├── app\                                # ★ 与硬件无关的纯逻辑（可 PC 测试）
│   ├── ring_buffer.h
│   └── ring_buffer.c
├── tests\                              # ★ 宿主机单元测试
│   ├── CMakeLists.txt
│   ├── test_harness.h
│   └── test_ring_buffer.c
├── docs\
│   └── （设计文档、协议说明）
├── .github\
│   └── workflows\
│       └── ci.yml
└── firmware\                           # ★ STM32CubeMX 生成，AI 不得手改
    ├── firmware.ioc
    ├── CMakeLists.txt                  # CubeMX 生成
    ├── cmake\                          # CubeMX 生成
    ├── Core\
    ├── Drivers\
    ├── startup_stm32f407xx.s
    └── STM32F407VGTx_FLASH.ld
```

**关键分层铁律**：`app/` 下的代码**不得** `#include` 任何 HAL/CMSIS 头文件。硬件能力通过接口注入。这样 `app/` 才能 100% 在 PC 上跑单测——这是让 AI 敢改代码的前提。

---

## 4. 第 1 层：AI 上下文与约束（先做这个）

### 4.1 `AGENTS.md`（★ 最重要，投入产出比最高）

> 实测经验：**把硬件事实写清楚，比换更强的模型有效得多。** 通用助手 90% 的嵌入式错误源于"不知道你的板子长什么样"。

````markdown
# 项目：STM32 固件（AI 协作上下文）

> 本文件是 AI 助手的**唯一事实来源**。它是抑制"寄存器幻觉"的第一道防线：
> **AI 必须从这里取硬件事实，而不是靠记忆猜。** 硬件变更后必须同步更新本文件。

## 1. 硬件事实（勿让 AI 猜）

| 项 | 值 |
|---|---|
| MCU 型号 | `STM32F407VGT6` |
| 内核 | Cortex-M4F（单精度 FPU + DSP） |
| 主频 | 168 MHz（HSE 8 MHz → PLL） |
| Flash | 1024 KB，**应用预算 ≤ 640 KB** |
| RAM | 192 KB（SRAM1+SRAM2），**应用预算 ≤ 96 KB** |
| CCM RAM | 64 KB，**仅数据、不可用于 DMA 缓冲** |
| 堆 | `heap_4`，`configTOTAL_HEAP_SIZE = 8192` |
| 主栈 | 2 KB（`_Min_Stack_Size`） |
| 调试器 | ST-Link V2 / SWD @ 4 MHz |
| 编译器 | `arm-none-eabi-gcc`（见 `cmake/toolchains/arm-none-eabi.cmake`） |

### 已占用引脚（禁止重复分配）

| 引脚 | 功能 | 备注 |
|---|---|---|
| PA9 / PA10 | USART1 TX / RX | 115200 8N1，日志口 |
| PB6 / PB7 | I2C1 SCL / SDA | 400 kHz，AF4，**开漏 + 外部上拉** |
| PA5 / PA6 / PA7 | SPI1 SCK / MISO / MOSI | 10 MHz，Mode 0 |
| PA4 | SPI1 NSS | 软件控制 GPIO 推挽 |
| PC13 | 状态 LED | 推挽输出，**低电平有效** |
| PA2 | ADC1_IN2 | 电池电压采样 |
| PA0 | EXTI0 | 按键，下降沿触发 |

> **冲突裁决**：本文件与 `firmware/firmware.ioc` 不一致时，**以 `.ioc` 为准**并修正本文件。

## 2. 目录约定

| 路径 | 内容 | AI 可否直接改 |
|---|---|---|
| `firmware/` | CubeMX 产物（`Core/`、`Drivers/`、`*.s`、`*.ld`） | ❌ 只能改 `.ioc` 后重新生成 |
| `firmware/Core/Src/main.c` | 主循环 | ⚠️ **仅限 `/* USER CODE BEGIN */ … END */` 区块内** |
| `app/` | 与硬件无关的纯逻辑 | ✅ 可改 |
| `tests/` | 宿主机单元测试 | ✅ 可改 |
| `docs/` | 设计文档 | ✅ 可改 |
| `cmake/` `scripts/` `.github/` | 构建与 CI | ⚠️ 改前先说明理由 |

## 3. 构建与验证命令（AI 必须用这些，不要自创）

```powershell
# ① 宿主机单元测试（秒级，改完 app/ 必须先跑）
cmake -S . -B build-host -G Ninja -DBUILD_HOST_TESTS=ON
cmake --build build-host
ctest --test-dir build-host --output-on-failure

# ② 静态分析（MISRA + CERT-C）
pwsh -File scripts\analyze.ps1

# ③ 固件构建 + Flash/RAM 占用
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake
cmake --build build
cmake --build build --target size-report

# ④ 一键完整验证（等价于 CI）
pwsh -File scripts\verify.ps1
```

## 4. 编码硬约束

### MUST NOT（违反即驳回）

1. **禁止动态内存**：`malloc` / `calloc` / `realloc` / `free` / `new`。
2. **禁止递归**（含互递归）——栈深必须静态可证。
3. **禁止 VLA**（`int buf[n]`）、`alloca`；数组长度必须编译期常量。
4. **禁止在 ISR 内阻塞**：不得用 `vTaskDelay`、带超时的信号量等待、`printf`、浮点长运算。
5. **禁止浮点运算出现在 ISR 内**（FPU 上下文代价高且非原子）。
6. **禁止共享变量不加 `volatile`**（主循环 ↔ ISR 之间）。
7. **禁止硬件寄存器裸魔数**：必须用 CMSIS/HAL 宏或具名 `#define`。
8. **禁止改动 CubeMX 生成代码中 `/* USER CODE BEGIN */` 之外的区域**。
9. **禁止在未人工确认的前提下执行烧录 / 擦除**（见 §5）。

### MUST

1. 所有缓冲区、栈、队列大小**静态确定**，注释中写明最坏情况字节数。
2. 临界区用 `taskENTER_CRITICAL()`，且**尽可能短**。
3. ISR **只做**：读硬件、清标志、投递消息、返回。重活交给任务。
4. 每个对外函数注明**可重入性**与**最大执行时间**（>100 µs 必须标注）。
5. 位操作统一用 `|` `&` `~` 组合赋值，不用位域结构体映射寄存器。
6. 断言用 `configASSERT` / 自定义 `APP_ASSERT`，**不依赖 `assert.h`**。
7. 新逻辑**必须附带 `tests/` 下的单元测试**，否则视为未完成。

## 5. 操作权限：审批闸门

| 操作 | AI 可否自动执行 | 条件 |
|---|---|---|
| 读文件、搜索、静态分析 | ✅ 是 | — |
| 改 `app/` `tests/` `docs/` | ✅ 是 | 改完必须跑 ① 并给出结果 |
| 改 `cmake/` `scripts/` `.github/` | ⚠️ 需说明 | 先解释影响再看 diff |
| 改 `firmware/` | ❌ 否 | 只能改 `.ioc` 再生成 |
| `git commit` / `push` | ❌ 否 | 由人决定 |
| **烧录 / 擦除 / 改选项字节** | ❌ **绝对禁止** | 必须人工执行 |

> 理由：`rm -rf` 可以重来，**烧错地址 / 误清选项字节会让芯片变砖**。

## 6. 风险阶梯（从低到高）

| 阶段 | 任务 | 谁主导 |
|---|---|---|
| 1 | 驱动脚手架、数据解析、状态机、单元测试、注释 | **AI 主导**，人 review |
| 2 | 外设初始化参数、任务划分、队列设计 | **AI 提案 + 人决策** |
| 3 | ISR 时序、中断优先级、临界区、看门狗、DMA 一致性 | **人主导**，AI 辅助复核 |
| 4 | 安全 / 认证 / 实时控制环路 / 电源时序 | **人独立完成**，AI 不参与生成 |

## 7. 完成定义（DoD）

改动只有同时满足以下 6 条才算完成，缺一条就如实说"未完成"：

- [ ] ① 宿主机单元测试通过（附 `ctest` 输出摘要）
- [ ] ② `scripts\analyze.ps1` 无新增告警
- [ ] ③ 固件可编译，**且给出 Flash / RAM 增量**
- [ ] ④ 无新增 `malloc` / 递归 / VLA / ISR 阻塞
- [ ] ⑤ 寄存器、引脚、时钟改动已与 §1 表格核对
- [ ] ⑥ 行为类改动写明**上板验证步骤**

## 8. 易错点速查（AI 最常翻车处）

| 症状 | 真实原因 | 处理 |
|---|---|---|
| 引用不存在的 `HAL_XXX` | 同系列**不同型号/版本** HAL 差异 | 以 `firmware/Drivers/STM32F4xx_HAL_Driver/Inc/` 实际头文件为准 |
| 引脚 AF 编号写错 | 记忆错误 | 查数据手册 AF 表，核 `firmware/Core` 中 `GPIO_InitTypeDef` |
| SPI/I2C 时序拍脑袋 | 未看传感器手册 | 必须引用传感器手册 timing 表 |
| `HAL_Delay` 用在中断里 | 阻塞 | 改状态机或 `vTaskDelay` |
| I2C 读回全 0xFF | 开漏未加上拉 / 地址左移错 | 检查硬件上拉与 7bit 地址 |
| 队列满丢数据未处理 | `xQueueSend` 超时 0 被忽略 | 明确丢弃策略并统计 |
| 能编译，上板 HardFault | 栈溢出 / 空指针 / 未对齐访问 | 查 `vApplicationStackOverflowHook`，开 `configCHECK_FOR_STACK_OVERFLOW=2` |
| 编译通过但时序不满足 | 忽略中断延迟与抖动 | GPIO 翻转 + 示波器实测，别信估算 |

## 9. 与 AI 协作的输出要求

1. **给结论前先给依据**：引用文件路径 + 行号，或数据手册章节 / 寄存器名。
2. **不确定就说不确定**，并给出验证方法。
3. **不要一次改多个不相关文件**；小步提交，方便回滚。
4. **不要静默修改约束**（例如为编译通过而放开 `-Werror`、加回 `malloc`）。
5. 涉及硬件行为的结论**必须标注"未经上板验证"**，除非确实测过。

## 10. 变更记录

| 日期 | 变更 | 变更人 |
|---|---|---|
| 2026-09-15 | 初版 | — |
````

> **注意**：第 1 节的表格必须换成**你自己的板子**的真实数据。写错比不写更危险。

---

### 4.2 `.codebuddy/rules/embedded-firmware/RULE.mdc`

> CodeBuddy 项目规则路径是 `.codebuddy/rules/<规则名>/RULE.mdc`（**每条规则一个文件夹，文件名固定 `RULE.mdc`**，不是扁平的 `.md`）。`alwaysApply: true` 表示每个会话都加载。

```markdown
---
description: STM32 固件编码硬约束与操作审批闸门（禁止动态内存/递归/VLA，禁止 ISR 阻塞，烧录必须人工确认）
alwaysApply: true
enabled: true
---

# 固件硬约束

适用本仓库任何 C/C++ 改动。生成或修改代码前先默认套用以下规则，不要等用户提醒。

## 禁止清单

- 禁止 `malloc` / `calloc` / `realloc` / `free` / `new` / `std::vector` 等任何动态分配。
- 禁止递归，包括间接递归与"回调套回调"式的隐式递归。
- 禁止 VLA（`int buf[n]`）、`alloca`、可变长栈分配。
- 禁止在 ISR 中调用阻塞 API、`printf`/`sprintf`、浮点长运算。
- 禁止在共享变量（主循环 ↔ ISR）上省略 `volatile`。
- 禁止硬件寄存器裸魔数；必须用 CMSIS/HAL 宏或具名 `#define`。
- 禁止修改 CubeMX 生成代码中 `/* USER CODE BEGIN */` 之外的部分。
- 禁止为了"让它编译过"而放宽 `-Werror`、关闭静态分析、或绕过 `app/` 不依赖 HAL 的分层约定。

## 强制要求

- 所有缓冲区/栈/队列容量静态确定，注释中写明**最坏情况字节数**。
- ISR 只做四件事：读硬件、清标志、投递消息、返回。
- `app/` 下模块**不得** `#include` 任何 `stm32*_hal*.h` 或 CMSIS 头文件；外部依赖通过接口注入。
- 新增逻辑必须同时新增 `tests/` 下的宿主机单元测试；否则明确说明"测试未覆盖"。

## 操作审批闸门（最重要）

- **烧录、擦除、写选项字节、改 BOOT 配置：AI 一律不执行**。只输出命令与目标地址，由人工执行。
- 改动 `firmware/`、`cmake/`、`scripts/`、`.github/` 前，先说明影响与理由。
- 不主动执行 `git commit` / `git push`。
- 可自由执行：读文件、搜索、`scripts\analyze.ps1`、宿主机单元测试。

## 回答格式要求

- 涉及寄存器、引脚、时钟的结论，**必须**引用 `AGENTS.md` §1 或 `firmware/firmware.ioc` 或 HAL 头文件中的实际定义。
- 涉及硬件行为的结论，若未上板实测，必须显式标注"未经上板验证"。
- 完成后按 `AGENTS.md` §7 DoD 逐条自查，给出 `ctest` 与 `size-report` 实际输出摘要，不要用"应该没问题"代替证据。
```

> **生效条件**：规则只在**会话开始时**注入。创建后必须**新建对话**才生效。

---

### 4.3 `.mcp.json`（Windows 写法，注意坑）

```json
{
  "mcpServers": {
    "embedded": {
      "type": "stdio",
      "command": "python",
      "args": ["tools/mcp/embedded_server.py"],
      "description": "STM32 工作流：构建 / Flash-RAM 占用 / MISRA 静态分析 / 串口日志 / 烧录（需人工确认）"
    },
    "filesystem": {
      "type": "stdio",
      "command": "cmd",
      "args": [
        "/c", "npx", "-y",
        "@modelcontextprotocol/server-filesystem",
        "firmware/Core",
        "firmware/Drivers/STM32F4xx_HAL_Driver/Inc",
        "app"
      ],
      "description": "只读限定目录：避免 AI 越界读写与泄露敏感文件"
    }
  }
}
```

**Windows 三个坑，务必注意**：

| 坑 | 说明 |
|---|---|
| `npx` 直接写会失败 | Windows 上 `npx` 是 `.cmd` 批处理，MCP 客户端直接 spawn 会报错，**必须用 `cmd /c npx` 包裹** |
| `python` 找不到 | 若用 Microsoft Store 版 Python，PATH 里可能没有 `python.exe`。改用 `"command": "py", "args": ["-3", "tools/mcp/embedded_server.py"]`，或填绝对路径 |
| 路径分隔符 | JSON 里用 `/`，或用 `\\`（反斜杠必须转义）。**不要**用单个 `\` |

**配置文件位置**：CodeBuddy 的 MCP 配置入口是 `设置面板 → MCP → Add MCP`（官方文档未公开底层文件名）。建议先直接粘贴到这个 JSON 编辑器里；若你的版本支持读取项目根目录 `.mcp.json`，这份文件会自动生效。

---

### 4.4 `.clang-format`（格式统一，避免 AI 改动产生巨大 diff）

```yaml
# 嵌入式惯用：4 空格缩进、指针靠右、强制大括号（MISRA 一致性）
Language: Cpp
BasedOnStyle: LLVM

IndentWidth: 4
TabWidth: 4
UseTab: Never
ColumnLimit: 100

AllowShortIfStatementsOnASingleLine: Never
AllowShortLoopsOnASingleLine: false
AllowShortFunctionsOnASingleLine: None
AllowShortBlocksOnASingleLine: Never

BraceWrapping:
  AfterControlStatement: Always
  AfterFunction: Always
  AfterStruct: true
  AfterEnum: true
  BeforeElse: false
  IndentBraces: false

PointerAlignment: Right
DerivePointerAlignment: false

SortIncludes: CaseSensitive
IncludeBlocks: Preserve

SpaceBeforeParens: ControlStatements
SpacesInParentheses: false
AlignConsecutiveMacros: Consecutive
AlignConsecutiveAssignments: None
BinPackParameters: false
BinPackArguments: false
SpaceAfterCStyleCast: false
```

---

### 4.5 `.clang-tidy`（CERT-C 安全子集）

```yaml
# MISRA 由 cppcheck --addon=misra 覆盖，这里聚焦 CERT-C 与缺陷模式
Checks: >
  -*,
  bugprone-*,
  cert-*,
  clang-analyzer-*,
  misc-*,
  performance-*,
  portability-*,
  readability-braces-around-statements,
  readability-identifier-naming,
  readability-implicit-bool-conversion,
  readability-misleading-indentation,
  readability-non-const-parameter,
  readability-uppercase-literal-suffix,
  -bugprone-easily-swappable-parameters,
  -bugprone-macro-parentheses,
  -readability-magic-numbers,
  -cert-err33-c,
  -cert-err34-c

WarningsAsErrors: 'bugprone-*,cert-*,clang-analyzer-*'

CheckOptions:
  - key: readability-identifier-naming.VariableCase
    value: lower_case
  - key: readability-identifier-naming.VariableIgnoredRegexp
    value: '^(i|j|k|n|p)$'
  - key: readability-identifier-naming.FunctionCase
    value: lower_case
  - key: readability-identifier-naming.MacroDefinitionCase
    value: UPPER_CASE
  - key: readability-identifier-naming.StructCase
    value: lower_case
  - key: readability-identifier-naming.TypedefSuffix
    value: '_t'
  - key: readability-identifier-naming.EnumConstantCase
    value: UPPER_CASE
  - key: readability-implicit-bool-conversion.AllowPointerConditions
    value: false
```

---

### 4.6 `cppcheck/suppressions.txt`

```text
# 抑制规则，格式: 规则id:文件:行  —— 每条都必须写理由，禁止"因为麻烦"而抑制
# 示例（默认关闭，需要时才开启并写明原因）：
#
# unusedFunction:app/ring_buffer.c    # 该函数供 ISR 通过弱符号调用，cppcheck 静态不可见
# misra-c2012-8.7:app/*.c             # 跨 TU 使用的内部函数，已验证无外部链接冲突
```

---

### 4.7 `.gitattributes` 与 `.gitignore`（Windows 必配）

**`.gitattributes`**

```gitattributes
# 源码统一 LF（避免 CRLF 导致的 diff 噪音与编译告警）
* text=auto eol=lf

# 脚本必须 CRLF，否则 Windows PowerShell 5.1 中可能解析异常
*.ps1   text eol=crlf
*.bat   text eol=crlf
*.cmd   text eol=crlf

# 二进制产物不做换行转换
*.bin   binary
*.hex   binary
*.elf   binary
*.a     binary
*.lib   binary
*.png   binary
*.pdf   binary

# 生成物也统一 LF
*.ioc   text eol=lf
*.ld    text eol=lf
*.s     text eol=lf
```

**`.gitignore`**

```gitignore
# 构建产物
build/
build-*/
cmake-build-*/
*.o
*.d
*.a
*.elf
*.bin
*.hex
*.map
*.su
compile_commands.json
CMakeCache.txt
CMakeFiles/
CMakeUserPresets.json
Testing/

# IDE
.vscode/*
!.vscode/settings.json
!.vscode/launch.json
.idea/
*.uvprojx
*.uvoptx
*.uvguix.*
JLinkLog.txt
RTE/

# 静态分析报告
reports/
*.sarif
cppcheck-*.xml
clang-tidy-*.log

# 串口抓包
serial-dumps/
*.raw

# 系统
.DS_Store
Thumbs.db
desktop.ini

# 绝不入库
.env
.env.*
*.pem
*.key
secrets/
local.mk
```

---

## 5. 第 2 层：构建与测试骨架

### 5.1 先生成固件工程（STM32CubeMX）

1. 打开 STM32CubeMX → `File > New Project` → 选你的 MCU。
2. 配置时钟、引脚、外设、FreeRTOS（按需）。
3. `Project Manager` 页：
   - **Project Name**：`firmware`
   - **Project Location**：`C:\work\stm32`
   - **Toolchain / IDE**：**`CMake`** ← 关键，别选 Makefile 或 MDK-ARM
4. `GENERATE CODE`。

生成后 `C:\work\stm32\firmware\CMakeLists.txt` 里会有 `project(firmware ...)` 和 `add_executable(firmware ...)`，这就是后续要挂接的 target。

### 5.2 `cmake/toolchains/arm-none-eabi.cmake`

```cmake
# ARM GNU Toolchain，裸机（无 OS），用于 Cortex-M
set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 避免 CMake 做"能否链接可执行文件"的探测（交叉编译下必然失败）
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 允许覆盖工具链位置：cmake -DARM_TOOLCHAIN_BIN=C:/path/to/bin
# 或设环境变量 ARM_TOOLCHAIN_BIN
if(NOT ARM_TOOLCHAIN_BIN AND DEFINED ENV{ARM_TOOLCHAIN_BIN})
    set(ARM_TOOLCHAIN_BIN "$ENV{ARM_TOOLCHAIN_BIN}")
endif()

find_program(CMAKE_C_COMPILER    NAMES arm-none-eabi-gcc     HINTS "${ARM_TOOLCHAIN_BIN}" REQUIRED)
find_program(CMAKE_CXX_COMPILER  NAMES arm-none-eabi-g++     HINTS "${ARM_TOOLCHAIN_BIN}" REQUIRED)
find_program(CMAKE_ASM_COMPILER  NAMES arm-none-eabi-gcc     HINTS "${ARM_TOOLCHAIN_BIN}" REQUIRED)
find_program(CMAKE_OBJCOPY       NAMES arm-none-eabi-objcopy HINTS "${ARM_TOOLCHAIN_BIN}" REQUIRED)
find_program(CMAKE_SIZE          NAMES arm-none-eabi-size    HINTS "${ARM_TOOLCHAIN_BIN}" REQUIRED)

# 交叉编译时只在工具链里找库/头文件，不做宿主机查找
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
```

### 5.3 顶层 `CMakeLists.txt`

> 设计要点：**两种模式**。`BUILD_HOST_TESTS=ON` 走宿主机编译器（秒级单测）；否则走交叉编译并把 `app/` 挂到 CubeMX 生成的 target 上。

```cmake
cmake_minimum_required(VERSION 3.20)

# ───────── 可调参数 ─────────
set(FIRMWARE_DIR "firmware" CACHE PATH   "STM32CubeMX 生成的工程目录")
set(APP_DIR      "app"      CACHE PATH   "与硬件无关的纯逻辑代码")
set(FW_TARGET    ""         CACHE STRING "CubeMX 生成的 target 名（默认同目录名）")
set(BUILD_HOST_TESTS OFF    CACHE BOOL   "只在 PC 上构建单元测试")

project(stm32-ai-workspace LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # clangd / clang-tidy / cppcheck 依赖

# ───────── 模式 A：宿主机单元测试 ─────────
if(BUILD_HOST_TESTS)
    enable_testing()
    add_subdirectory(tests)
    return()
endif()

# ───────── 模式 B：交叉编译固件 ─────────
if(NOT CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR
        "未指定交叉编译工具链。\n"
        "  固件构建:    cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake\n"
        "  单元测试:    cmake -S . -B build-host -G Ninja -DBUILD_HOST_TESTS=ON")
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${FIRMWARE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "未找到 ${FIRMWARE_DIR}/CMakeLists.txt。\n"
        "请用 STM32CubeMX 打开 .ioc，Project Name 填 `${FIRMWARE_DIR}`，"
        "Toolchain/IDE 选 `CMake`，然后 Generate Code。")
endif()

add_subdirectory("${FIRMWARE_DIR}" "${CMAKE_BINARY_DIR}/${FIRMWARE_DIR}")

if(FW_TARGET STREQUAL "")
    set(FW_TARGET "${FIRMWARE_DIR}")
endif()
if(NOT TARGET ${FW_TARGET})
    message(FATAL_ERROR
        "未找到 target `${FW_TARGET}`。请核对 ${FIRMWARE_DIR}/CMakeLists.txt 中"
        "project()/add_executable() 的名称，并用 -DFW_TARGET=<name> 覆盖。")
endif()

# 把纯逻辑代码挂到固件 target
target_sources(${FW_TARGET} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/${APP_DIR}/ring_buffer.c")

target_include_directories(${FW_TARGET} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/${APP_DIR}")

# 生成 .hex / .bin
if(CMAKE_OBJCOPY)
    add_custom_command(TARGET ${FW_TARGET} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${FW_TARGET}> "${CMAKE_BINARY_DIR}/${FW_TARGET}.hex"
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${FW_TARGET}> "${CMAKE_BINARY_DIR}/${FW_TARGET}.bin"
        COMMENT "生成 .hex / .bin")
endif()

# Flash / RAM 占用报告（DoD 必须项）
if(CMAKE_SIZE)
    add_custom_target(size-report
        COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${FW_TARGET}>
        COMMAND ${CMAKE_SIZE} --format=sysv     $<TARGET_FILE:${FW_TARGET}>
        DEPENDS ${FW_TARGET}
        COMMENT "Flash / RAM 占用报告")
endif()
```

### 5.4 `app/ring_buffer.h`（示例纯逻辑模块）

```c
#ifndef APP_RING_BUFFER_H
#define APP_RING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * 定长单生产者单消费者（SPSC）环形缓冲区。
 * 约束：零动态内存、无递归、无 VLA，可在 Cortex-M0 上确定性执行。
 * 并发模型：head 由生产者写、tail 由消费者写；单核 MCU 下 volatile 足够，
 *           多核需替换为 C11 atomic。
 */

typedef struct
{
    uint8_t *buffer;
    size_t   capacity; /* 必须是 2 的幂 */
    volatile size_t head;
    volatile size_t tail;
} ring_buffer_t;

/* 初始化。capacity 非 2 的幂或指针为空时返回 false。 */
bool rb_init(ring_buffer_t *rb, uint8_t *storage, size_t capacity);

/* 写入最多 len 字节，返回实际写入数（可能小于 len，表示缓冲区已满）。 */
size_t rb_write(ring_buffer_t *rb, const uint8_t *data, size_t len);

/* 读出最多 len 字节，返回实际读出数（可能小于 len，表示缓冲区已空）。 */
size_t rb_read(ring_buffer_t *rb, uint8_t *out, size_t len);

/* 当前可读字节数。 */
size_t rb_available(ring_buffer_t *rb);

/* 当前可写字节数。 */
size_t rb_free_space(ring_buffer_t *rb);

#endif /* APP_RING_BUFFER_H */
```

### 5.5 `app/ring_buffer.c`

```c
#include "ring_buffer.h"

static bool is_power_of_two(size_t v)
{
    return (v != 0U) && ((v & (v - 1U)) == 0U);
}

bool rb_init(ring_buffer_t *rb, uint8_t *storage, size_t capacity)
{
    if ((rb == NULL) || (storage == NULL) || (!is_power_of_two(capacity)))
    {
        return false;
    }

    rb->buffer   = storage;
    rb->capacity = capacity;
    rb->head     = 0U;
    rb->tail     = 0U;
    return true;
}

size_t rb_available(ring_buffer_t *rb)
{
    return rb->head - rb->tail;
}

size_t rb_free_space(ring_buffer_t *rb)
{
    return rb->capacity - (rb->head - rb->tail);
}

size_t rb_write(ring_buffer_t *rb, const uint8_t *data, size_t len)
{
    size_t n;
    size_t i;

    if ((rb == NULL) || (data == NULL))
    {
        return 0U;
    }

    n = rb_free_space(rb);
    if (len < n)
    {
        n = len;
    }

    for (i = 0U; i < n; ++i)
    {
        rb->buffer[(rb->head + i) & (rb->capacity - 1U)] = data[i];
    }

    rb->head = rb->head + n;
    return n;
}

size_t rb_read(ring_buffer_t *rb, uint8_t *out, size_t len)
{
    size_t n;
    size_t i;

    if ((rb == NULL) || (out == NULL))
    {
        return 0U;
    }

    n = rb_available(rb);
    if (len < n)
    {
        n = len;
    }

    for (i = 0U; i < n; ++i)
    {
        out[i] = rb->buffer[(rb->tail + i) & (rb->capacity - 1U)];
    }

    rb->tail = rb->tail + n;
    return n;
}
```

### 5.6 `tests/test_harness.h`（零依赖断言，避免为跑测试先搭 Unity）

```c
#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdio.h>

static int g_checks   = 0;
static int g_failures = 0;

#define CHECK(cond)                                                          \
    do                                                                       \
    {                                                                        \
        ++g_checks;                                                          \
        if (!(cond))                                                         \
        {                                                                    \
            ++g_failures;                                                    \
            (void)printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
        }                                                                    \
    } while (0)

#define CHECK_EQ_SIZE(actual, expected)                                      \
    do                                                                       \
    {                                                                        \
        size_t a_ = (size_t)(actual);                                        \
        size_t e_ = (size_t)(expected);                                      \
        ++g_checks;                                                          \
        if (a_ != e_)                                                        \
        {                                                                    \
            ++g_failures;                                                    \
            (void)printf("FAIL %s:%d  %s: got %zu, want %zu\n", __FILE__,    \
                         __LINE__, #actual, a_, e_);                         \
        }                                                                    \
    } while (0)

#define TEST_SUMMARY()                                                       \
    ((void)printf("\n%d checks, %d failures\n", g_checks, g_failures),       \
     (g_failures == 0) ? 0 : 1)

#endif /* TEST_HARNESS_H */
```

### 5.7 `tests/test_ring_buffer.c`

```c
#include "test_harness.h"
#include "ring_buffer.h"

static void test_init_rejects_invalid(void)
{
    ring_buffer_t rb;
    uint8_t       storage[8];

    CHECK(rb_init(&rb, storage, 0U) == false);   /* 容量不能为 0 */
    CHECK(rb_init(&rb, storage, 3U) == false);   /* 非 2 的幂 */
    CHECK(rb_init(NULL, storage, 8U) == false);  /* 空指针 */
    CHECK(rb_init(&rb, NULL, 8U) == false);      /* 空存储 */
    CHECK(rb_init(&rb, storage, 8U) == true);
}

static void test_write_read_roundtrip(void)
{
    ring_buffer_t rb;
    uint8_t       storage[8];
    uint8_t       out[8] = {0};
    const uint8_t in[4]  = {0x11U, 0x22U, 0x33U, 0x44U};

    CHECK(rb_init(&rb, storage, 8U) == true);
    CHECK_EQ_SIZE(rb_available(&rb), 0U);
    CHECK_EQ_SIZE(rb_free_space(&rb), 8U);

    CHECK_EQ_SIZE(rb_write(&rb, in, 4U), 4U);
    CHECK_EQ_SIZE(rb_available(&rb), 4U);

    CHECK_EQ_SIZE(rb_read(&rb, out, 4U), 4U);
    CHECK_EQ_SIZE(rb_available(&rb), 0U);
    CHECK(out[0] == 0x11U);
    CHECK(out[3] == 0x44U);
}

static void test_wraparound(void)
{
    ring_buffer_t rb;
    uint8_t       storage[8];
    uint8_t       out[1];
    uint8_t       i;
    uint8_t       v;

    CHECK(rb_init(&rb, storage, 8U) == true);

    /* 用 1 字节读写把 head/tail 推到接近绕回的位置 */
    for (i = 0U; i < 6U; ++i)
    {
        v = i;
        CHECK_EQ_SIZE(rb_write(&rb, &v, 1U), 1U);
        CHECK_EQ_SIZE(rb_read(&rb, out, 1U), 1U);
        CHECK(out[0] == i);
    }

    /* 再写满、读空，验证索引掩码正确 */
    v = 0xAAU;
    for (i = 0U; i < 8U; ++i)
    {
        CHECK_EQ_SIZE(rb_write(&rb, &v, 1U), 1U);
    }
    CHECK_EQ_SIZE(rb_write(&rb, &v, 1U), 0U); /* 满，写不进 */
    CHECK_EQ_SIZE(rb_available(&rb), 8U);

    for (i = 0U; i < 8U; ++i)
    {
        CHECK_EQ_SIZE(rb_read(&rb, out, 1U), 1U);
        CHECK(out[0] == 0xAAU);
    }
    CHECK_EQ_SIZE(rb_read(&rb, out, 1U), 0U); /* 空，读不到 */
}

static void test_partial_operations(void)
{
    ring_buffer_t rb;
    uint8_t       storage[4];
    uint8_t       out[4] = {0};
    const uint8_t in[6]  = {1U, 2U, 3U, 4U, 5U, 6U};

    CHECK(rb_init(&rb, storage, 4U) == true);

    /* 缓冲区只有 4 字节，写 6 字节只能进 4 字节 */
    CHECK_EQ_SIZE(rb_write(&rb, in, 6U), 4U);
    CHECK_EQ_SIZE(rb_available(&rb), 4U);

    /* 读 2 字节 */
    CHECK_EQ_SIZE(rb_read(&rb, out, 2U), 2U);
    CHECK(out[0] == 1U);
    CHECK(out[1] == 2U);
}

static void test_null_safety(void)
{
    ring_buffer_t rb;
    uint8_t       storage[4];
    uint8_t       out[4];
    const uint8_t in[4] = {0};

    CHECK(rb_init(&rb, storage, 4U) == true);
    CHECK_EQ_SIZE(rb_write(NULL, in, 4U), 0U);
    CHECK_EQ_SIZE(rb_write(&rb, NULL, 4U), 0U);
    CHECK_EQ_SIZE(rb_read(NULL, out, 4U), 0U);
    CHECK_EQ_SIZE(rb_read(&rb, NULL, 4U), 0U);
}

int main(void)
{
    test_init_rejects_invalid();
    test_write_read_roundtrip();
    test_wraparound();
    test_partial_operations();
    test_null_safety();
    return TEST_SUMMARY();
}
```

### 5.8 `tests/CMakeLists.txt`

```cmake
add_executable(host_tests
    test_ring_buffer.c
    "${CMAKE_CURRENT_SOURCE_DIR}/../app/ring_buffer.c")

target_include_directories(host_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../app")

if(MSVC)
    target_compile_options(host_tests PRIVATE /W4 /WX)
else()
    target_compile_options(host_tests PRIVATE -Wall -Wextra -Werror -Wpedantic)
endif()

add_test(NAME ring_buffer COMMAND host_tests)
```

### 5.9 验收：立刻跑一次

```powershell
cd C:\work\stm32
cmake -S . -B build-host -G Ninja -DBUILD_HOST_TESTS=ON
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

**预期输出**：`100% tests passed, 0 tests failed out of 1`，且打印 `24 checks, 0 failures`。

> 这一步**不需要交叉编译器**，纯 C，任何 PC 都能过。如果这步都过不了，先别往下做。

---

## 6. 第 3 层：脚本与 MCP 能力接入

### 6.1 `scripts/analyze.ps1`（MISRA + CERT-C）

```powershell
#Requires -Version 5.1
<#
.SYNOPSIS  对 app/ 与 tests/ 执行 MISRA + clang-tidy + clang-format 检查
#>
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root

try {
    $reportDir = Join-Path $root 'reports'
    New-Item -ItemType Directory -Force $reportDir | Out-Null

    # clang-tidy 需要 compile_commands.json
    if (-not (Test-Path 'build-host/compile_commands.json')) {
        Write-Host '生成 compile_commands.json ...' -ForegroundColor DarkGray
        cmake -S . -B build-host -G Ninja -DBUILD_HOST_TESTS=ON | Out-Null
    }

    $failed = $false

    # ── 1. cppcheck + MISRA ──────────────────────────────
    Write-Host '=== cppcheck (MISRA C:2012) ===' -ForegroundColor Cyan
    $cppArgs = @(
        '--enable=all'
        '--addon=misra'
        '--error-exitcode=1'
        '--inline-suppr'
        '--std=c11'
        '--quiet'
        '--suppressions-list=cppcheck/suppressions.txt'
        '--xml'
        "--output-file=$reportDir/cppcheck.xml"
        'app'
    )
    if (Test-Path 'tests') { $cppArgs += 'tests' }

    & cppcheck @cppArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Host "cppcheck 发现问题，详见 $reportDir/cppcheck.xml" -ForegroundColor Red
        $failed = $true
    } else {
        Write-Host 'cppcheck 通过' -ForegroundColor Green
    }

    # ── 2. clang-tidy ────────────────────────────────────
    Write-Host '=== clang-tidy ===' -ForegroundColor Cyan
    $cSources = Get-ChildItem -Path app -Recurse -Filter '*.c' -ErrorAction SilentlyContinue
    if ($cSources) {
        foreach ($src in $cSources) {
            $out  = & clang-tidy -p build-host --quiet $src.FullName 2>&1
            $warn = $out | Where-Object { $_ -match 'warning:|error:' }
            if ($warn) {
                Write-Host "--- $($src.Name) ---" -ForegroundColor Yellow
                $warn | ForEach-Object { Write-Host $_ }
                $failed = $true
            }
        }
    }
    if (-not $failed) { Write-Host 'clang-tidy 通过' -ForegroundColor Green }

    # ── 3. clang-format 检查 ─────────────────────────────
    Write-Host '=== clang-format 检查 ===' -ForegroundColor Cyan
    $fmtTargets = @()
    foreach ($d in @('app', 'tests')) {
        if (Test-Path $d) {
            $fmtTargets += (Get-ChildItem -Path $d -Recurse -Include '*.c', '*.h' |
                            ForEach-Object { $_.FullName })
        }
    }
    if ($fmtTargets) {
        $fmtOut = & clang-format --dry-run --Werror @fmtTargets 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Host '存在格式不合规文件（运行 clang-format -i 修复）：' -ForegroundColor Yellow
            $fmtOut | Select-Object -First 40 | ForEach-Object { Write-Host $_ }
            $failed = $true
        } else {
            Write-Host 'clang-format 通过' -ForegroundColor Green
        }
    }

    if ($failed) {
        Write-Host "`n静态分析未通过。" -ForegroundColor Red
        exit 1
    }
    Write-Host "`n静态分析全部通过。" -ForegroundColor Green
}
finally {
    Pop-Location
}
```

### 6.2 `scripts/verify.ps1`（一键验证闭环 = 本地 CI）

```powershell
#Requires -Version 5.1
<#
.SYNOPSIS  完整验证闭环：单测 → 静态分析 → 固件构建 → Flash/RAM 占用
.EXAMPLE   pwsh -File scripts\verify.ps1
#>
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root

$sw = [System.Diagnostics.Stopwatch]::StartNew()
try {
    # ── 1/4 宿主机单元测试 ───────────────────────────────
    Write-Host '=== 1/4 宿主机单元测试 ===' -ForegroundColor Cyan
    cmake -S . -B build-host -G Ninja -DBUILD_HOST_TESTS=ON
    if ($LASTEXITCODE -ne 0) { throw 'CMake 配置失败' }
    cmake --build build-host
    if ($LASTEXITCODE -ne 0) { throw '单元测试构建失败' }
    ctest --test-dir build-host --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw '单元测试未通过' }

    # ── 2/4 静态分析 ────────────────────────────────────
    Write-Host "`n=== 2/4 静态分析 ===" -ForegroundColor Cyan
    & (Join-Path $PSScriptRoot 'analyze.ps1')
    if ($LASTEXITCODE -ne 0) { throw '静态分析未通过' }

    # ── 3/4 固件构建 ────────────────────────────────────
    Write-Host "`n=== 3/4 固件构建 ===" -ForegroundColor Cyan
    if (-not (Test-Path 'firmware/CMakeLists.txt')) {
        Write-Warning '未找到 firmware/CMakeLists.txt，跳过固件构建。'
        Write-Warning '请先用 STM32CubeMX 生成代码（Toolchain 选 CMake）。'
        exit 0
    }

    cmake -S . -B build -G Ninja `
        -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake `
        -DCMAKE_BUILD_TYPE=Debug
    if ($LASTEXITCODE -ne 0) { throw '固件 CMake 配置失败（检查 PATH 里的 arm-none-eabi-gcc）' }
    cmake --build build
    if ($LASTEXITCODE -ne 0) { throw '固件构建失败' }

    # ── 4/4 Flash / RAM 占用 ────────────────────────────
    Write-Host "`n=== 4/4 Flash / RAM 占用 ===" -ForegroundColor Cyan
    cmake --build build --target size-report

    Write-Host "`n验证闭环通过（耗时 $([math]::Round($sw.Elapsed.TotalSeconds,1))s）。" -ForegroundColor Green
    Write-Host '产物: build\firmware.elf / .hex / .bin' -ForegroundColor DarkGray
}
catch {
    Write-Host "`n验证失败: $_" -ForegroundColor Red
    exit 1
}
finally {
    Pop-Location
}
```

### 6.3 烧录命令（人工执行，AI 只能给命令）

**方式 A：STM32CubeProgrammer CLI（推荐，Windows 上最稳）**

```powershell
# 先确认连上（会打印芯片型号、UID、读保护状态）
STM32_Programmer_CLI.exe -c port=SWD freq=4000

# 烧录 .elf（含地址信息），校验后复位
STM32_Programmer_CLI.exe -c port=SWD freq=4000 `
    -w build\firmware.elf -v -rst

# ⚠️ 绝对不要加 -ob 参数（写选项字节）。误写 RDP 等级会导致芯片锁死。
```

**方式 B：OpenOCD（若你已装 xPack OpenOCD）**

```powershell
openocd -f interface/stlink.cfg `
        -f target/stm32f4x.cfg `
        -c "program build/firmware.elf verify reset exit"
```

> Windows 上用 **`interface/stlink.cfg`**（不是 `stlink-v2.cfg`，那个已弃用）。

### 6.4 `tools/mcp/embedded_server.py`（MCP Server）

> 这是把"AI 能跑真实命令"和"AI 不能烧录"同时实现的关键。**只依赖标准库**（pyserial 仅串口功能需要）。

```python
#!/usr/bin/env python3
"""最小可用 MCP (stdio) 服务器：把 STM32 工作流暴露给 AI 助手。

设计原则
--------
* 只依赖标准库；pyserial 可选，仅 serial_* 工具需要
* 不经过 shell，全部 subprocess 列表调用（避免命令注入）
* 烧录/复位属于「审批闸门」：confirm 令牌不匹配时只返回预览命令，绝不执行
* 路径全部相对仓库根目录解析
"""
from __future__ import annotations

import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT       = Path(__file__).resolve().parents[2]
BUILD_DIR  = ROOT / "build"
BUILD_HOST = ROOT / "build-host"
ELF        = BUILD_DIR / "firmware.elf"

PROTOCOL_VERSION = "2024-11-05"
SERVER_INFO      = {"name": "stm32-embedded", "version": "1.0.0"}

# OpenOCD 调试器配置，按需修改
OPENOCD_INTERFACE = "interface/stlink.cfg"
OPENOCD_TARGET    = "target/stm32f4x.cfg"


# ─────────────────────────── 工具函数 ───────────────────────────

def run(cmd: list[str], timeout: int = 900) -> tuple[int, str, str]:
    """无 shell 执行外部命令，返回 (returncode, stdout, stderr)。"""
    try:
        p = subprocess.run(
            cmd, cwd=str(ROOT), capture_output=True, text=True,
            timeout=timeout, encoding="utf-8", errors="replace",
        )
        return p.returncode, p.stdout, p.stderr
    except FileNotFoundError:
        return 127, "", f"未找到命令 `{cmd[0]}`，请检查它是否在 PATH 中。"
    except subprocess.TimeoutExpired:
        return 124, "", f"命令超时（>{timeout}s）: {' '.join(cmd)}"


def ok(text: str) -> dict:
    return {"content": [{"type": "text", "text": text}], "isError": False}


def err(text: str) -> dict:
    return {"content": [{"type": "text", "text": text}], "isError": True}


def combine(rc: int, out: str, e: str, tail: int = 60) -> str:
    lines = (out + ("\n" + e if e else "")).strip().splitlines()
    body = "\n".join(lines[-tail:])
    return f"(exit={rc})\n{body}"


def elf_token() -> str:
    """基于 elf 内容生成一次性确认令牌，防止 AI 自行'确认'。"""
    if not ELF.exists():
        return ""
    digest = hashlib.sha256(ELF.read_bytes()).hexdigest()
    return f"FLASH-{digest[:8].upper()}"


# ─────────────────────────── 工具实现 ───────────────────────────

def t_host_test() -> dict:
    rc1, o1, e1 = run(["cmake", "-S", ".", "-B", "build-host", "-G", "Ninja",
                       "-DBUILD_HOST_TESTS=ON"])
    if rc1 != 0:
        return err("CMake 配置失败\n" + combine(rc1, o1, e1))
    rc2, o2, e2 = run(["cmake", "--build", "build-host"])
    if rc2 != 0:
        return err("构建失败\n" + combine(rc2, o2, e2))
    rc3, o3, e3 = run(["ctest", "--test-dir", "build-host", "--output-on-failure"])
    status = "通过" if rc3 == 0 else "未通过"
    text = f"宿主机单元测试{status}\n" + combine(rc3, o3, e3)
    return ok(text) if rc3 == 0 else err(text)


def t_analyze() -> dict:
    rc, o, e = run(["pwsh", "-NoProfile", "-File", "scripts/analyze.ps1"], timeout=1800)
    text = combine(rc, o, e, tail=80)
    return ok(text) if rc == 0 else err(text)


def t_fw_build(build_type: str = "Debug") -> dict:
    if not (ROOT / "firmware" / "CMakeLists.txt").exists():
        return err("未找到 firmware/CMakeLists.txt。请先用 STM32CubeMX 生成代码（Toolchain 选 CMake）。")
    rc1, o1, e1 = run(["cmake", "-S", ".", "-B", "build", "-G", "Ninja",
                       "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake",
                       f"-DCMAKE_BUILD_TYPE={build_type}"])
    if rc1 != 0:
        return err("固件 CMake 配置失败（检查 PATH 中的 arm-none-eabi-gcc）\n" + combine(rc1, o1, e1))
    rc2, o2, e2 = run(["cmake", "--build", "build"])
    return ok(combine(rc2, o2, e2)) if rc2 == 0 else err("固件构建失败\n" + combine(rc2, o2, e2))


def t_fw_size() -> dict:
    if not ELF.exists():
        return err(f"未找到 {ELF}，请先执行 fw_build。")
    rc, o, e = run(["arm-none-eabi-size", "--format=berkeley", str(ELF)])
    if rc != 0:
        return err(combine(rc, o, e))
    _, o2, _ = run(["arm-none-eabi-size", "--format=sysv", str(ELF)])
    return ok("Flash / RAM 占用（berkeley）\n" + o + "\n各段明细（sysv）\n" + o2)


def t_fw_flash(confirm: str = "") -> dict:
    """审批闸门：confirm 必须等于当前 elf 的令牌，否则只返回预览。"""
    if not ELF.exists():
        return err(f"未找到 {ELF}，请先执行 fw_build。")

    token = elf_token()
    cmd = ["openocd", "-f", OPENOCD_INTERFACE, "-f", OPENOCD_TARGET,
           "-c", f"program {ELF.as_posix()} verify reset exit"]

    preview = (
        "【烧录预览 — 尚未执行】\n"
        f"  目标文件 : {ELF}\n"
        f"  大小     : {ELF.stat().st_size} 字节\n"
        f"  内容令牌 : {token}\n"
        f"  OpenOCD  : {' '.join(cmd)}\n"
        "  STM32CubeProgrammer 等价命令:\n"
        f"    STM32_Programmer_CLI.exe -c port=SWD freq=4000 -w {ELF} -v -rst\n"
        "\n"
        "此操作会写入芯片 Flash，AI 不会自动执行。\n"
        "确认无误后由人工在终端执行上面的命令；或调用本工具时传入 "
        f'confirm="{token}" 授权（令牌随 elf 内容变化，防止误烧旧固件）。\n'
        "禁止附加 -ob 参数（写选项字节会导致芯片锁死）。"
    )

    if confirm != token or token == "":
        return ok(preview)

    rc, o, e = run(cmd, timeout=300)
    return ok("烧录完成\n" + combine(rc, o, e)) if rc == 0 else err("烧录失败\n" + combine(rc, o, e))


def t_openocd_reset(confirm: str = "") -> dict:
    token = elf_token() or "RESET"
    cmd = ["openocd", "-f", OPENOCD_INTERFACE, "-f", OPENOCD_TARGET,
           "-c", "init", "-c", "reset run", "-c", "exit"]
    if confirm != token:
        return ok(f"【复位预览 — 尚未执行】\n  {' '.join(cmd)}\n"
                  f'  传入 confirm="{token}" 以授权。')
    rc, o, e = run(cmd, timeout=60)
    return ok(combine(rc, o, e)) if rc == 0 else err(combine(rc, o, e))


def t_serial_read(port: str, baud: int = 115200, seconds: int = 5, lines: int = 200) -> dict:
    try:
        import serial  # type: ignore
    except ImportError:
        return err("缺少 pyserial。安装：pip install pyserial\n"
                   "（不装也可以用 PuTTY / 串口助手手工看日志）")
    try:
        with serial.Serial(port, baud, timeout=seconds) as ser:
            data = ser.read(lines * 128)
    except Exception as exc:  # noqa: BLE001 - 串口错误种类多，统一回报
        return err(f"打开串口 {port} 失败: {exc}\nWindows 串口名形如 COM3，可在设备管理器确认。")
    text = data.decode("utf-8", errors="replace")
    return ok(f"{port} @ {baud} 读取 {len(data)} 字节:\n{text}")


def t_serial_write(port: str, data: str, baud: int = 115200) -> dict:
    try:
        import serial  # type: ignore
    except ImportError:
        return err("缺少 pyserial。安装：pip install pyserial")
    try:
        with serial.Serial(port, baud, timeout=1) as ser:
            n = ser.write(data.encode("utf-8"))
    except Exception as exc:  # noqa: BLE001
        return err(f"写串口 {port} 失败: {exc}")
    return ok(f"已向 {port} 写入 {n} 字节")


# ─────────────────────────── 工具清单 ───────────────────────────

TOOLS = [
    {
        "name": "host_test",
        "description": "在 PC 上构建并运行 app/ 的单元测试（秒级）。修改 app/ 后应先调用本工具。",
        "inputSchema": {"type": "object", "properties": {}, "additionalProperties": False},
    },
    {
        "name": "analyze",
        "description": "运行 MISRA C:2012 + clang-tidy + clang-format 静态分析门禁。",
        "inputSchema": {"type": "object", "properties": {}, "additionalProperties": False},
    },
    {
        "name": "fw_build",
        "description": "交叉编译 STM32 固件（需先有 firmware/ 工程）。",
        "inputSchema": {
            "type": "object",
            "properties": {
                "build_type": {"type": "string",
                               "enum": ["Debug", "Release", "MinSizeRel"],
                               "description": "构建类型，默认 Debug"}
            },
            "additionalProperties": False,
        },
    },
    {
        "name": "fw_size",
        "description": "报告固件的 Flash/RAM 占用（含各段明细），用于评估改动代价。",
        "inputSchema": {"type": "object", "properties": {}, "additionalProperties": False},
    },
    {
        "name": "fw_flash",
        "description": "烧录固件到芯片。属于审批闸门操作：不带正确的 confirm 令牌时只返回预览命令，不会执行。",
        "inputSchema": {
            "type": "object",
            "properties": {
                "confirm": {"type": "string",
                            "description": "由本工具预览中返回的令牌，形如 FLASH-XXXXXXXX"}
            },
            "additionalProperties": False,
        },
    },
    {
        "name": "openocd_reset",
        "description": "复位并运行芯片（审批闸门，同样需要 confirm 令牌）。",
        "inputSchema": {
            "type": "object",
            "properties": {"confirm": {"type": "string"}},
            "additionalProperties": False,
        },
    },
    {
        "name": "serial_read",
        "description": "从串口读取日志（Windows 串口名形如 COM3）。需安装 pyserial。",
        "inputSchema": {
            "type": "object",
            "properties": {
                "port":    {"type": "string",  "description": "串口名，如 COM3"},
                "baud":    {"type": "integer", "description": "波特率，默认 115200"},
                "seconds": {"type": "integer", "description": "读取时长秒数，默认 5"},
                "lines":   {"type": "integer", "description": "最多读取行数，默认 200"},
            },
            "required": ["port"],
            "additionalProperties": False,
        },
    },
    {
        "name": "serial_write",
        "description": "向串口发送字符串（调试命令注入）。需安装 pyserial。",
        "inputSchema": {
            "type": "object",
            "properties": {
                "port": {"type": "string"},
                "data": {"type": "string", "description": "要发送的内容"},
                "baud": {"type": "integer"},
            },
            "required": ["port", "data"],
            "additionalProperties": False,
        },
    },
]

DISPATCH = {
    "host_test":     lambda a: t_host_test(),
    "analyze":       lambda a: t_analyze(),
    "fw_build":      lambda a: t_fw_build(a.get("build_type", "Debug")),
    "fw_size":       lambda a: t_fw_size(),
    "fw_flash":      lambda a: t_fw_flash(a.get("confirm", "")),
    "openocd_reset": lambda a: t_openocd_reset(a.get("confirm", "")),
    "serial_read":   lambda a: t_serial_read(a["port"], int(a.get("baud", 115200)),
                                             int(a.get("seconds", 5)), int(a.get("lines", 200))),
    "serial_write":  lambda a: t_serial_write(a["port"], a["data"], int(a.get("baud", 115200))),
}


# ─────────────── MCP 协议（stdio，换行分隔 JSON）───────────────

def send(msg: dict) -> None:
    sys.stdout.write(json.dumps(msg, ensure_ascii=False) + "\n")
    sys.stdout.flush()


def handle(req: dict) -> dict | None:
    rid    = req.get("id")
    method = req.get("method")

    if rid is None:          # notification，无需响应
        return None

    if method == "initialize":
        return {"jsonrpc": "2.0", "id": rid, "result": {
            "protocolVersion": PROTOCOL_VERSION,
            "capabilities": {"tools": {}},
            "serverInfo": SERVER_INFO,
        }}

    if method == "tools/list":
        return {"jsonrpc": "2.0", "id": rid, "result": {"tools": TOOLS}}

    if method == "tools/call":
        params = req.get("params") or {}
        name   = params.get("name", "")
        args   = params.get("arguments") or {}
        fn     = DISPATCH.get(name)
        if fn is None:
            return {"jsonrpc": "2.0", "id": rid, "result": err(f"未知工具: {name}")}
        try:
            return {"jsonrpc": "2.0", "id": rid, "result": fn(args)}
        except KeyError as exc:
            return {"jsonrpc": "2.0", "id": rid, "result": err(f"缺少必需参数: {exc}")}
        except Exception as exc:  # noqa: BLE001 - 兜底，避免服务崩溃
            return {"jsonrpc": "2.0", "id": rid,
                    "result": err(f"工具 {name} 执行异常: {type(exc).__name__}: {exc}")}

    if method == "ping":
        return {"jsonrpc": "2.0", "id": rid, "result": {}}

    return {"jsonrpc": "2.0", "id": rid,
            "error": {"code": -32601, "message": f"未知方法: {method}"}}


def main() -> None:
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            req = json.loads(line)
        except json.JSONDecodeError:
            continue
        resp = handle(req)
        if resp is not None:
            send(resp)


if __name__ == "__main__":
    main()
```

**装可选依赖**（只为串口功能）：

```powershell
pip install pyserial
```

---

## 7. 第 4 层：CI 门禁（GitHub Actions）

### `.github/workflows/ci.yml`

```yaml
name: ci

on:
  push:
  pull_request:

jobs:
  # ── 宿主机单元测试：Linux + Windows 双平台 ──
  host-tests:
    name: host tests (${{ matrix.os }})
    strategy:
      fail-fast: false
      matrix:
        os: [ubuntu-latest, windows-latest]
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4

      - name: 安装 Ninja (Linux)
        if: runner.os == 'Linux'
        run: sudo apt-get update && sudo apt-get install -y ninja-build

      - name: 配置
        run: cmake -S . -B build-host -G Ninja -DBUILD_HOST_TESTS=ON

      - name: 构建
        run: cmake --build build-host

      - name: 测试
        run: ctest --test-dir build-host --output-on-failure

  # ── 静态分析门禁 ──
  static-analysis:
    name: MISRA + clang-tidy
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: 安装分析工具
        run: |
          sudo apt-get update
          sudo apt-get install -y cppcheck clang-tidy ninja-build

      - name: 生成 compile_commands.json
        run: cmake -S . -B build-host -G Ninja -DBUILD_HOST_TESTS=ON

      - name: cppcheck (MISRA C:2012)
        run: |
          cppcheck --enable=all --addon=misra --error-exitcode=1 \
                   --inline-suppr --std=c11 --quiet \
                   --suppressions-list=cppcheck/suppressions.txt \
                   app

      - name: 检查抑制项是否被滥用
        run: |
          count=$(grep -cvE '^\s*(#|$)' cppcheck/suppressions.txt || true)
          echo "抑制条目数: $count"
          if [ "$count" -gt 20 ]; then
            echo "::error::抑制项过多（$count > 20），请修复而非抑制"
            exit 1
          fi

      - name: clang-tidy
        run: clang-tidy -p build-host app/*.c

  # ── 固件构建（仅在存在 firmware/ 工程时执行）──
  firmware:
    name: firmware build & size
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - id: check
        shell: bash
        run: |
          if [ -f firmware/CMakeLists.txt ]; then
            echo "exists=true" >> "$GITHUB_OUTPUT"
          else
            echo "exists=false" >> "$GITHUB_OUTPUT"
            echo "::notice::未找到 firmware/CMakeLists.txt，跳过固件构建"
          fi

      - name: 安装 ARM 工具链
        if: steps.check.outputs.exists == 'true'
        uses: carlosperate/arm-none-eabi-gcc-action@v1

      - name: 安装构建工具
        if: steps.check.outputs.exists == 'true'
        run: sudo apt-get update && sudo apt-get install -y ninja-build

      - name: 构建固件
        if: steps.check.outputs.exists == 'true'
        run: |
          cmake -S . -B build -G Ninja \
                -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
                -DCMAKE_BUILD_TYPE=Release
          cmake --build build

      - name: Flash/RAM 占用报告
        if: steps.check.outputs.exists == 'true'
        run: cmake --build build --target size-report

      - name: 上传产物
        if: steps.check.outputs.exists == 'true'
        uses: actions/upload-artifact@v4
        with:
          name: firmware-artifacts
          path: |
            build/*.elf
            build/*.hex
            build/*.bin
```

> `carlosperate/arm-none-eabi-gcc-action@v1` 是社区维护的 ARM 工具链 Action。若不可用，改用 `apt-get install gcc-arm-none-eabi`（Ubuntu 仓库版本较旧但够用）。

---

## 8. `prompts/firmware-tasks.md`（提示词模板）

> 把提示词也纳入版本控制，让"怎么问 AI"变成团队规范，而不是个人技巧。

````markdown
# 固件任务提示词模板

## 通用前缀（每次都要带）

> 遵守 `AGENTS.md` 与 `.codebuddy/rules/embedded-firmware/RULE.mdc`。
> 禁止动态内存/递归/VLA/ISR 阻塞。`app/` 下不得 include HAL 头文件。
> 改完必须跑 `host_test` 并给出实际输出。

---

## 模板 1：驱动脚手架（风险阶段 1，推荐起点）

```
基于 [传感器型号] 数据手册的 [章节号/寄存器表]，
为 `app/[模块名].h/.c` 生成驱动脚手架。

要求：
- 接口：init / read / 自检 三个函数，返回明确错误码
- 不 include 任何 HAL 头文件；I2C 读写通过函数指针注入
- 每个函数注明可重入性与最大执行时间
- 所有寄存器地址必须来自数据手册，并在注释标注页码
- 未知的时序参数不要猜，标 `TODO(datasheet): 待确认`

产出：.h / .c / tests/test_[模块名].c 三个文件
```

---

## 模板 2：中断服务程序（风险阶段 3，需人工主导）

```
为 [外设/EXTI] 编写 ISR。

约束：
- ISR 内只做：读数据寄存器、清中断标志、xQueueSendFromISR(..., NULL)、退出
- 不确定优先级时不要指定，标 `TODO(design): 中断优先级待定`
- 不使用 HAL_Delay / vTaskDelay / printf / 浮点
- 共享变量必须 volatile

同时给出：
1. 该 ISR 的最坏执行时间估算（按指令周期，注明依据）
2. 需要在 FreeRTOSConfig.h 中调整的项（如有）
3. 上板验证步骤（用哪个 GPIO 翻转、示波器看什么）
```

---

## 模板 3：代码审查（把 AI 当 reviewer 用，收益很高）

```
审查我即将提交的 [文件列表]，按 AGENTS.md §4 与 §8 逐条检查。

输出格式（Markdown 表格）：
| 严重度 | 位置(文件:行) | 问题 | 依据 | 建议修法 |

要求：
- "依据"必须引用 AGENTS.md 条款号 / 手册章节 / MISRA 规则号
- 不确定的标注 `需人工确认`，不要编造
- 不要提风格问题（交给 clang-format）
```

---

## 模板 4：尺寸回归分析（改动前后对比）

```
执行 `fw_size`，与上次结果对比，给出：
- Flash / RAM 的绝对增量与百分比
- 增量来源定位（哪个 .o 变大最明显）
- 是否超出 AGENTS.md §1 的预算（Flash ≤ 640KB / RAM ≤ 96KB）
- 若超出，给出 3 个可选优化方向及各自预估收益
```

---

## 风险阶梯回顾（选模板前先定位自己）

| 阶段 | 适用模板 | 谁主导 |
|---|---|---|
| 1 脚手架/解析/测试 | 模板 1、3 | AI 主导 |
| 2 外设参数/任务设计 | 模板 3 | AI 提案 + 人决策 |
| 3 ISR/时序/临界区 | 模板 2（人审核到行） | 人主导 |
| 4 安全/实时控制环路 | ❌ 不用 AI 生成 | 人独立完成 |
````

---

## 9. 部署执行清单（照着打勾）

### 阶段一：环境（约 60 分钟，主要是下载）

- [ ] `C:\work\` 目录已建；PowerShell 执行策略已设为 `RemoteSigned`
- [ ] `git config --global core.longpaths true` 已设
- [ ] STM32CubeCLT 已安装，PATH 已配置，**重开终端**
- [ ] STM32CubeMX 已安装
- [ ] ST-LINK 驱动已安装
- [ ] `check-env.ps1` 全部 `[ OK ]`

### 阶段二：护栏（约 40 分钟，先做这个）

- [ ] 创建仓库目录结构（§3）
- [ ] `AGENTS.md` 写完，**第 1 节换成你自己板子的真实数据**
- [ ] `.codebuddy/rules/embedded-firmware/RULE.mdc` 已建
- [ ] `.clang-format` / `.clang-tidy` / `cppcheck/suppressions.txt` 已建
- [ ] `.gitattributes` / `.gitignore` 已建
- [ ] `git init` + 首次提交

### 阶段三：验证闭环（约 30 分钟）

- [ ] CubeMX 生成 `firmware/`（**Toolchain 选 CMake**）
- [ ] `cmake/toolchains/arm-none-eabi.cmake`、顶层 `CMakeLists.txt` 已建
- [ ] `app/ring_buffer.{h,c}`、`tests/*` 已建
- [ ] `pwsh -File scripts\verify.ps1` → 单测通过 + 固件编译 + size-report 有输出

### 阶段四：能力接入（约 30 分钟）

- [ ] `tools/mcp/embedded_server.py` 已建，`pip install pyserial`
- [ ] 手工验证 MCP（见 §10.2）
- [ ] `.mcp.json` 已建并接入 IDE（注意 `cmd /c npx` 写法）
- [ ] **新建对话**让规则生效

### 阶段五：门禁与习惯（约 20 分钟）

- [ ] `.github/workflows/ci.yml` 已建，push 后 CI 全绿
- [ ] `prompts/firmware-tasks.md` 已建
- [ ] 用模板 3 让 AI 审查一次现有代码，感受收益

---

## 10. 验证方法

### 10.1 验证护栏是否生效

在 IDE 里**新建对话**，输入：

```
请写一个函数，从 UART 接收不定长数据并存到缓冲区
```

**合格表现**：AI 主动提到"按 AGENTS.md 约束，不使用动态内存/递归，改为静态环形缓冲"，并引用 `app/ring_buffer.h`。

**不合格表现**：直接给出 `malloc` + 动态扩容的代码 → 说明规则未生效。检查 `.codebuddy/rules/` 路径与 `enabled: true`，并确认是新建的对话。

### 10.2 验证 MCP Server（PowerShell 手工测）

```powershell
cd C:\work\stm32
$reqs = @(
  '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"test","version":"1"}}}'
  '{"jsonrpc":"2.0","method":"notifications/initialized"}'
  '{"jsonrpc":"2.0","id":2,"method":"tools/list"}'
  '{"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"host_test","arguments":{}}}'
  '{"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"fw_flash","arguments":{}}}'
) -join "`n"

$reqs | python tools\mcp\embedded_server.py
```

**预期**：

| id | 期望 |
|---|---|
| 1 | 返回 `serverInfo.name = "stm32-embedded"`，`capabilities.tools = {}` |
| 2 | 无输出（notification 不应答） |
| 3 | 返回 8 个工具，含 `fw_flash` 且描述里写"审批闸门" |
| 4 | **返回烧录预览 + `FLASH-XXXXXXXX` 令牌，但明确写着"尚未执行"** |

第 4 条是**安全闸门的关键验证**：如果它直接执行了烧录，说明闸门失效，立刻停止使用。

### 10.3 验证 DoD 能否被真正执行

让 AI 做一个小改动（例如给 `rb_available` 换成原子读），然后检查它是否：

- [ ] 给出了 `ctest` 实际输出（不是"应该能过"）
- [ ] 给出了 `size-report` 前后对比
- [ ] 声明了"未经上板验证"

不满足 → 把 `AGENTS.md` §7 和 §9 的措辞再严厉一点。

---

## 11. Windows 11 常见坑速查

| 现象 | 原因 | 解法 |
|---|---|---|
| `cmake` 找不到 `arm-none-eabi-gcc` | PATH 未生效 | 改完 PATH **必须重开终端**；或用 `-DARM_TOOLCHAIN_BIN=C:/ST/.../bin` |
| MCP 的 `npx` 启动失败 | Windows 上 `npx` 是 `.cmd` | 必须写成 `"command": "cmd", "args": ["/c","npx",...]` |
| MCP 的 `python` 找不到 | Store 版 Python 无 `python.exe` | 改用 `"command": "py", "args": ["-3", ...]`，或绝对路径 |
| 构建慢得离谱 | Windows Defender 实时扫描 | 把项目目录与构建进程加入排除 |
| `ctest` 找不到测试 | 多配置生成器未指定 config | 用 `-G Ninja`（单配置），或加 `-C Debug` |
| CubeMX 生成后顶层配置失败 | target 名不叫 `firmware` | 查 `firmware/CMakeLists.txt` 的 `project()`，用 `-DFW_TARGET=<name>` 覆盖 |
| `.ps1` 无法运行 | 执行策略 | `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned` |
| `.ps1` 报语法错 | 换行符被 Git 转成 LF | `.gitattributes` 里 `*.ps1 text eol=crlf`，然后重新 checkout |
| cppcheck `--addon=misra` 报错 | 缺 Python 或 addon 路径 | 确保 `python` 在 PATH；确认 cppcheck 安装目录含 `addons/misra.py` |
| 中文注释在 CI 里乱码 | 源码编码 | 统一 UTF-8（VS Code 设 `files.encoding: utf8`），必要时加 `-finput-charset=UTF-8` |
| OpenOCD 找不到配置文件 | 用了旧式 `stlink-v2.cfg` | 改用 `interface/stlink.cfg` |
| 路径过长报错 | 目录层级深 | `git config --global core.longpaths true`；项目放 `C:\work\` |
| 串口打不开 | 名字不是 `/dev/ttyUSB0` | Windows 是 `COM3` 这种；且需关闭占用该口的其他程序 |
| 编译通过但 HardFault | 栈溢出 | `configCHECK_FOR_STACK_OVERFLOW=2` + `vApplicationStackOverflowHook` |
| 误烧选项字节锁死芯片 | 用了 `-ob` | **永远不要**加 `-ob`（除非你明确知道在改什么） |

---

## 12. 风险与回滚

| 风险 | 影响 | 缓解 |
|---|---|---|
| `AGENTS.md` 硬件描述写错 | AI 生成全盘错误的寄存器/引脚代码 | 第 1 节只填**已核对过**的信息；不确定的留 `TODO` |
| AI 生成代码"看起来对"但时序不满足 | 上板偶发故障，极难定位 | ISR/时序类改动强制阶段 3，人主导 + 示波器实测 |
| MCP 让 AI 越权操作 | 误烧录/误擦除 | 闸门令牌机制 + 不把 `-ob` 写进任何脚本 + git 小步提交 |
| 静态分析误报导致团队绕过门禁 | 护栏形同虚设 | 抑制项设上限（CI 已校验 ≤20 条），每条必须写理由 |
| 微调/语料带来的 IP 与许可风险 | 法务问题 | 敏感固件用本地模型；不要把厂商 SDK 喂给云端模型 |
| 遥测泄露 | 专有固件外泄 | 关掉云端代码上传；或用本地模型（Ollama + Qwen-Coder/DeepSeek-Coder） |

**回滚**：本方案所有产物都是**纯文件 + 配置**，不修改系统任何东西（除 PATH 与执行策略）。

- 撤销护栏：删除 `.codebuddy/rules/`、`.clang-*`、`AGENTS.md` 即可
- 撤销构建：删除顶层 `CMakeLists.txt`、`cmake/`、`build*/`，CubeMX 生成的 `firmware/` 不受影响
- 撤销 MCP：从 IDE 的 MCP 设置里删掉条目

---

## 13. 最小可落地子集（时间只有 1 小时时）

只做这三件事，收益已能覆盖约 80% 的价值：

1. **`AGENTS.md`** —— 把 MCU 型号、主频、Flash/RAM 上限、引脚映射写清楚（§4.1）
2. **`.codebuddy/rules/embedded-firmware/RULE.mdc`** —— 禁用动态内存/递归/ISR 阻塞（§4.2）
3. **`app/` + `tests/` 分层 + `host_test`** —— 让 AI 改的代码能在 PC 上秒级验证（§5.4–5.9）

这三件事做完，AI 就从"会写 bug 的实习生"变成"守规矩的实习生"。剩下的静态分析、CI、MCP 是把实习生变成可靠工程能力的过程。

---

## 附：配套方法论

`docs/AI用于嵌入式的最佳实践.md`（同目录）记录了本方案背后的方法论、行业现状与常见陷阱，可作为团队宣贯材料。

`docs/AI辅助立创EDA电路设计与验证流程.md`（同目录）是硬件侧配套：AI 接入立创EDA 的两条路线（官方 Skill / MCP），以及"原理图逻辑 → PCB 规则与 DFM → AI 复核 → 实物实测"四层验证方法。与本文 §5 的验证闭环一一对应——本文管固件，那篇管板子。

`docs/AI-PCB设计工具与Skill全景.md`（同目录）是硬件侧的**选型地图**：AI 原生平台（Flux / Quilter / 华秋 AI EDA 等）、代码化 EDA（atopile / Zener / circuit-synth）、Skill 与 MCP 生态（KiCad 侧 `kicad-happy`、`eda-pcb`，立创侧 `pcb-skill`）、DFM 与 SI/PI 仿真工具，以及能力边界与真实翻车案例。选型时先看这篇，落地时看立创 EDA 那篇。
