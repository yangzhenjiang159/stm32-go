# 项目：STM32F103ZET6 + 3.5" TFT（AI 协作上下文）

> **本文件是 AI 助手的唯一硬件事实来源**，对应 `docs/Win11-AI嵌入式工程落地方案.md` §4.1 的「第 0 层：AI 上下文层」。
> **AI 必须从这里取硬件事实，不许靠记忆猜寄存器/引脚/时序。** 事实优先级：
>
> ```
> 实物实测（点亮/示波器） >  docs/hardware-truth/HARDWARE-TRUTH.md  >  本文件  >  教程资料
> ```
>
> 本文件与 `docs/hardware-truth/HARDWARE-TRUTH.md` 冲突时，以后者为准并修正本文件。
> 硬件变更后必须同步更新本文件与 `HARDWARE-TRUTH.md`（见 §10 变更记录）。

**最后更新：2026-09-15**

---

## 1. 硬件事实（勿让 AI 猜）

| 项 | 值 |
|---|---|
| 开发板 | 尚硅谷 STM32 开发板，PCB 丝印 **`v1.01 -204`**（随附原理图 V1.0 / 2024-08-08） |
| MCU | **STM32F103ZET6**（High-density），Device ID `0x414`，封装 LQFP144 |
| 内核 | Cortex-M3，**无 FPU**（`float`/`double` 一律走软件浮点，慢且占 Flash） |
| Flash / RAM | 512 KB / 64 KB（`lcd_love/GCC/stm32_flash.ld`） |
| 系统时钟 | **HCLK = 72 MHz**（HSE 8 MHz × 9） |
| 下载器 | ST-Link V2，SN `066EFF505375485067123748`，固件 `V2J43S0`，虚拟串口 **COM6**，SWD @ 4 MHz |
| 工具链 | GNU Arm Embedded **10.3**（工作区 `gcc-arm/gcc-arm-none-eabi-10.3-2021.10/bin`） |
| 编译器 | `arm-none-eabi-gcc`，参数见 `lcd_love/build_gcc.ps1`（`-mcpu=cortex-m3 -mthumb -std=gnu99 -O2`） |
| 烧录工具 | `C:\Program Files (x86)\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe` |
| 资料里的 `armcc`/`armasm` | ❌ **不可用**（`A9555E: Failed to check out a license`），不要建议改用 Keil |

### 屏幕

| 项 | 值 |
|---|---|
| 屏 | 3.5"，**320×480 竖屏**，2×14（28 pin）排针直插开发板 LCD 座 |
| 驱动 IC | **ILI9488 风格**（读 `0xD3` 返回 `0x9488`）⚠️ 资料目录名写 ILI9486，**照抄其序列必白屏** |
| 资料例程 | `3.50LCD焊接37pin-ILI9486技术资料` 面向 **37pin** 模块，与本板 **28pin 不匹配**，其 `Template.hex` 在本板必然白屏 |

### 已占用引脚（禁止重复分配）

| 引脚 | 功能 | 备注 |
|---|---|---|
| PD14 / PD15 / PD0 / PD1 / PE7~PE15 / PD8 / PD9 / PD10 | FSMC D0~D15 | 顺序即上表顺序，不可改 |
| **PG12** | LCD_CS | FSMC_NE4（基址 `0x6C000000`） |
| **PG0** | LCD_RS | FSMC_A10（数据口 `0x6C000800`） |
| **PD5** | LCD_WR | FSMC_NWE |
| **PD4** | LCD_RD | FSMC_NOE |
| **PG15** | LCD_RST | 低电平复位（**不是官方例程的 PC5**） |
| **PB0** | LCD_LED 背光 | 高电平点亮 |
| PC1 / PC2 | GT-INT / GT-RST | 电容触摸，**本工程未使用**；I2C1（PB6/PB7）为触摸预留 |

### 6 条必须遵守的配置（改错就白屏）

1. FSMC 时序 `ADDSET=15`、`DATAST=255`（本板 72MHz；教程按 36MHz 写的过快）
2. 初始化序列用 ILI9488 风格，**必须含 `0xF7`**
3. 像素格式 `0x3A = 0x55`（16bit/像素）
4. 扫描方向 `0x36 = 0x08`（320×480 竖屏）
5. 复位脚 **PG15**
6. 行列地址 `0x2A/0x2B` 各写 **4 次 8 位值（字节流写法）**

### ⚠️ 已知不可靠（勿再尝试）

- **GRAM 读回恒为 `0xFCFC`**（`0x2E` Read Memory），与写入颜色无关 → **禁止设计依赖"读回校验"的诊断**
- 读 ID 不稳定（`0x04` 返回错误值，只有 `0xD3` 偶尔正确）
- **纯色全屏填充无法判断窗口寻址是否正确**（两种写法"看起来都正常"）→ 必须用**分离的小图形**（四角方块）

---

## 2. 目录约定（AI 可否直接改）

| 路径 | 内容 | AI 可否直接改 |
|---|---|---|
| `lcd_love/User/main.c` | 主程序（动画/配色/文字） | ✅ 可改 |
| `lcd_love/User/delay.c/.h` | SysTick 延时 | ✅ 可改 |
| `lcd_love/User/love_font.h` | 汉字点阵 | ❌ **脚本生成**（`tools/gen_font.py`） |
| `lcd_love/User/heart_table.h` | 心形轮廓点表 | ❌ **脚本生成**（`tools/gen_heart.py`） |
| `lcd_love/tools/*.py` | 生成脚本（字体/心形） | ✅ 可改 |
| `lcd_love/Interface/LCD/lcd.c` | LCD 驱动（写命令/数据/复位/背光/画图） | ⚠️ 改前先说明理由 |
| `lcd_love/Hardware/FSMC/fsmc.c` | FSMC + GPIO 初始化 | ⚠️ 改前先说明理由 |
| `lcd_love/GCC/` | CMSIS、启动文件、链接脚本 | ❌ 只读（改 = 动芯片启动/内存布局） |
| `lcd_love/Obj/` | 编译产物 | ❌ 由构建脚本生成 |
| `HARDWARE.md` / `DEBUG_WORKFLOW.md` / `PROJECT_STATE.md` / `docs/` | 文档 | ✅ 可改（结论必须回写） |
| `snapshots/` | 用户拍的实物照片 | ✅ 可读 |

**铁律**：`love_font.h` / `heart_table.h` 是生成物，**不要手改**，要改就改 `tools/*.py` 后重新生成。

---

## 3. 构建与验证命令（AI 必须用这些，不要自创）

```powershell
# ① 编译（唯一入口，产物 Obj/lcd_love.hex/.elf/.map，末尾会打印 size）
powershell -File lcd_love\build_gcc.ps1

# ② 烧录 + 运行；③ 仅复位运行；④ 读运行中内存（诊断用）
& 'C:\Program Files (x86)\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe' -c SWD -Q -P 'lcd_love\Obj\lcd_love.hex' -V -Rst -Run
& 'C:\Program Files (x86)\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe' -c SWD -Q -Rst -Run
& 'C:\Program Files (x86)\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe' -c SWD HOTPLUG -Q -r32 0x2000F000 16
```

**编译后必须看 `arm-none-eabi-size` 输出**：Flash 512KB / RAM 64KB 是硬上限，改动要报增量。

> ⚠️ `ST-LINK_CLI` **每次连接都会复位目标**；要读"运行中"的内存必须加 `HOTPLUG`。

---

## 4. 编码硬约束

> 本项目是**裸机、无 RTOS、无 HAL、无堆**的工程，约束按实际形态裁剪（非 CubeMX，故无 `/* USER CODE */` 区块规则）。

### MUST NOT（违反即驳回）

1. **禁止动态内存**：`malloc` / `calloc` / `realloc` / `free`。
2. **禁止递归**（含互递归）——栈深必须静态可证（`stm32_flash.ld` 里 `_Min_Stack_Size` 有限）。
3. **禁止 VLA**（`int buf[n]`）、`alloca`；数组长度必须编译期常量。
4. **禁止浮点出现在高频热路径**（本 MCU **无 FPU**）：逐像素/逐行循环内不得新增 `float`/`double`，优先整数定点或查表。
5. **禁止改 `lcd_love/GCC/`**（启动文件、`system_stm32f10x.c`、链接脚本）。
6. **禁止编译 `core_cm3.c`**（老版 CMSIS 与新 GCC 汇编不兼容，其弱函数用不到）。
7. **禁止硬件寄存器裸魔数**：必须用 CMSIS 宏或具名 `#define`（现有 `0xF7`/`0x3A` 等为屏 IC 命令码，需保留字面值 + 注释）。
8. **禁止改 `0x2A/0x2B` 的字节流写法**为 16 位写法（会导致内容挤成细线）。
9. **禁止在未人工确认的前提下执行烧录 / 擦除 / 写选项字节**（见 §5）。

### MUST

1. 所有缓冲区、点阵表、调色板**静态确定**，注释写明最坏情况字节数。
2. 屏参数（时序、初始化序列、扫描方向、引脚）改动**必须在注释里写明依据**（本文件 §1 或实测结论）。
3. 新逻辑修改 `main.c` 时，动画的**阶段划分与停留时间**要能自解释（便于"分步验证"）。
4. 每次改动给出**上板验证步骤 + 期望现象**（"上电后应该看到什么"）。
5. 断言用自定义 `#define`，**不依赖 `assert.h`**（裸机无 `printf`）。
6. 结论必须回写文档（`HARDWARE.md` / `PROJECT_STATE.md` / `lcd_love/README.md`）。

---

## 5. 操作权限：审批闸门

| 操作 | AI 可否自动执行 | 条件 |
|---|---|---|
| 读文件、搜索、编译 | ✅ 是 | — |
| 改 `User/`、`tools/`、文档 | ✅ 是 | 改完必须编译并给出 size 输出 |
| 改 `Interface/`、`Hardware/` | ⚠️ 需说明 | 先解释影响（属于"已调通"的配置） |
| 改 `GCC/`（启动/链接/CMSIS） | ❌ 否 | 需明确理由 + 人工确认 |
| `git commit` / `push` | ❌ 否 | 由人决定 |
| **烧录 / 擦除 / 改选项字节** | ❌ **绝对禁止** | AI 只输出命令，**必须人工执行** |

> 理由：`rm -rf` 可以重来，**烧错地址 / 误清选项字节会让芯片变砖**。
> 另：**一次只烧一个固件**，等用户反馈现象后再烧下一个（历史返工原因，见 `DEBUG_WORKFLOW.md` §5）。

---

## 6. 风险阶梯（从低到高）

| 阶段 | 任务 | 谁主导 |
|---|---|---|
| 1 | 动画/配色/文字排版、生成脚本、注释 | **AI 主导**，人 review |
| 2 | FSMC 时序、初始化序列、扫描方向、引脚分配 | **AI 提案 + 人决策**（这些已定型，不要随意动） |
| 3 | 中断、DMA、RS485/触摸等高实时性外设 | **人主导**，AI 辅助复核 |
| 4 | 电源时序、安规、认证 | **人独立完成**，AI 不参与生成 |

---

## 7. 完成定义（DoD）

改动只有同时满足以下 5 条才算完成，缺一条就如实说"未完成"：

- [ ] ① `build_gcc.ps1` 编译通过，并给出 `arm-none-eabi-size` 实际输出（含 Flash / RAM 增量）
- [ ] ② 无新增 `malloc` / 递归 / VLA / 热路径浮点
- [ ] ③ 引脚、寄存器、时序、初始化序列改动已与 §1 表格逐条核对
- [ ] ④ 行为类改动写明**上板验证步骤 + 期望现象**
- [ ] ⑤ 未上板实测的结论必须显式标注「**未经上板验证**」

---

## 8. 易错点速查（本项目真实翻车记录）

| 症状 | 真实原因 | 处理 |
|---|---|---|
| 烧官方/教程 hex **纯白屏** | 教程时序按 HCLK=36MHz 写，本板 72MHz | `ADDSET=15`、`DATAST=255` |
| 官方 `Template.hex` 也不亮 | 它写 `LCD_RST = PC5`，本板是 **PG15** | 按本板引脚自己写；**不能用它判断硬件好坏** |
| 内容全挤成"一侧一条细线" | `0x2A/0x2B` 写成 2 次 16 位 | 改回 4 次 8 位字节流 |
| 全屏纯色"看着正常"但内容错 | **纯色不能验证窗口寻址** | 用四角方块等分离小图形 |
| 读 GRAM 全是 `0xFCFC` | 该屏读通路不可靠 | 不设计读回校验，改"目视 + 分阶段对照" |
| 初始化缺 `0xF7` → 白屏 | ILI9488 风格要求 Adjust Control 3 | 序列里必须有 `0xF7` |
| 画面只铺 2/3 / 数据错位 | `0x3A` 用了 `0x66` | 必须 `0x55`（16bit/像素） |
| 内容跑到屏幕外 | 扫描方向错 | `0x36 = 0x08` |
| `armasm/armcc` 报授权失败 | 需要 license | 用工作区 `gcc-arm/` 的 GCC 10.3 |
| 画面卡顿/Flash 增长异常 | 无 FPU，`float` 走软件浮点 | 热路径改定点/查表 |

---

## 9. 与 AI 协作的输出要求

1. **给结论前先给依据**：引用文件路径 + 行号，或本文件 §1 / 实测结论。
2. **不确定就说不确定**，并给出验证方法（本板首选"分阶段对照 + 目视"）。
3. **不要一次改多个不相关文件**；小步修改，方便回滚。
4. **不要静默修改约束**（例如为通过编译而放宽时序、改回 16 位地址写法）。
5. 涉及硬件行为的结论**必须标注"未经上板验证"**，除非确实测过。
6. 现象描述用**可数、可比**的语言（数量/比例/颜色/位置）；**能拍照就拍**，图片放 `snapshots/`（见 `DEBUG_WORKFLOW.md` §1）。

---

## 10. 变更记录

| 日期 | 变更 | 变更人 |
|---|---|---|
| 2026-09-15 | 初版（依据 `HARDWARE.md` + `lcd_love/README.md` 的实测结论建立） | AI |
