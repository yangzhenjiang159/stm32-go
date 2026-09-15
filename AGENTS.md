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
| 开发板 | **尚硅谷** STM32 开发板，PCB 丝印 **`v1.01 -204`**（物主目视确认）；`www.atguigu.com` 实物照片确认；随附原理图 **V1.0 / 2024-08-08 / 共 13 页**（无缺页） |
| 屏模块 PCB | 版本丝印 **`V1.0-198`**（**与开发板 `v1.01 -204` 是两块不同的板，勿混**） |
| MCU | **STM32F103ZET6**（High-density），封装 LQFP144（型号 `Z` 位 = 144 pin） |
| 芯片身份 | Device ID `0x414`、UID `0x57058817 37375547 05D8FF36`、DBGMCU `REV_ID = 0x1003`（**均为 ST-Link 实测**） |
| 内核 | Cortex-M3，**无 FPU**（`float`/`double` 一律走软件浮点，慢且占 Flash） |
| Flash / RAM | 512 KB / 64 KB（**实测**：ST-Link 报 512 KB；复位后 SP = `0x20010000` → RAM 顶 64 KB） |
| 系统时钟 | **HCLK = 72 MHz**、APB1 = 36 MHz、APB2 = 72 MHz（**实测** `RCC_CFGR = 0x001D040A`：SW/SWS = PLL、PLLSRC = HSE、PLLMUL = ×9、HPRE /1、PPRE1 /2、PPRE2 /1；`RCC_CR` 显示 HSERDY = 1、PLLRDY = 1） |
| LSE / RTC | 未使用（**实测** `RCC_BDCR = 0` → LSE 未开启） |
| 选项字节 | `RDP = 0xA5`（Level 0，**未启用读保护**）；写保护全未启用；`nBOOT1 = 1` → BOOT1 = 0（**实测只读**） |
| 启动配置 | **BOOT0 有 10 kΩ 下拉**（原理图页 2 原文「BOOT0默认为10K拉低，跳线1/2闭合是为拉高」）；BOOT1 = **PB2** |
| 复位电路 | **R13 = 10 kΩ 上拉到 3V3** + 按键 **SW2** + **C22 = 100 nF**（原理图页 2） |
| 电源 | 5V → **`AMS1117-3.3V`（位号 Q1）** → 3V3；输入来自 USB 5V 或 DC12V；C5 = C6 = 10 µF（页 1）。⚠️ 额定电流需查其 datasheet |
| HSE 晶振 | **X2 = 8 MHz**，负载电容 **C9 = C10 = 20 pF**（原理图页 2） |
| LSE 晶振 | X1 = 32.768 kHz + 12 pF ×2（页 2；**固件未使用**，`RCC_BDCR = 0`） |
| 下载器 | ST-Link V2，SN `066EFF505375485067123748`，固件 `V2J43S0`，虚拟串口 **COM6**，SWD @ 4 MHz |
| 工具链 | GNU Arm Embedded **10.3**（工作区 `gcc-arm/gcc-arm-none-eabi-10.3-2021.10/bin`） |
| 编译器 | `arm-none-eabi-gcc`，参数见 `lcd_love/build_gcc.ps1`（`-mcpu=cortex-m3 -mthumb -std=gnu99 -O2`） |
| 烧录工具 | `C:\Program Files (x86)\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe` |
| 资料里的 `armcc`/`armasm` | ❌ **不可用**（`A9555E: Failed to check out a license`），不要建议改用 Keil |

### 屏幕

| 项 | 值 |
|---|---|
| 屏 | 3.5"，**320×480 竖屏**，2×14（28 pin）排针直插开发板 LCD 座（原理图型号 **`PZ254-2-14-S`**，页 12「TFT-LCD接口」） |
| 屏座接线 | 页 12 给出逐脚网络：D0~D15 / **NE4** / **A10** / **NWE** / **NOE** / LCD-RST / LCD-BG / I2C1-SDA,SCL / GT-INT（详见 `HARDWARE-TRUTH.md` §2.8） |
| 驱动 IC | **ILI9488 风格**（读 `0xD3` 返回 `0x9488`）⚠️ 资料目录名写 ILI9486，**照抄其序列必白屏**。驱动 IC 是 **COG 裸片**（压焊在玻璃上）→ **外观不可见、无 datasheet**，只能靠实测风格判定 |
| **触摸控制器** | **`FT5316WE`**（敦泰 / FocalTech 电容触摸，QFN 封在触摸排线上；实物照片 `snapshots/20260915-屏模块背面.jpg`）。⚠️ 原理图网络名却叫 `GT-INT`/`GT-RST`（Goodix 风格）→ **别照 GT911 写驱动**。本工程**未使用触摸** |
| 资料例程 | `3.50LCD焊接37pin-ILI9486技术资料` 面向 **37pin** 模块，与本板 **28pin 不匹配**，其 `Template.hex` 在本板必然白屏 |

### 板载其它器件（实物照片确认，2026-09-15）

> 照片在 `snapshots/`（AI 可直接读图）：`20260915-整板正面.jpg` / `-整板背面.jpg` / `-MCU特写.jpg`。
> ⚠️ 这些器件**本工程都没有初始化**，将来调"下一个器件"时再逐个开。

| 器件 | 型号（照片丝印） | 总线（推断） |
|---|---|---|
| 以太网 | **W5500** + RJ45 `HR911105A` | SPI |
| ESP32-C3 | `ESP32-C3 模块` 插座 | UART / SPI |
| RS232 / RS485 | `SP3232` + DB9 / 绿色端子 + 收发器 | UART |
| CAN | `CAN` 区收发器 | CAN |
| **SDRAM** | **`ISSI IS62WV51216BLL-55TLI`**（512 K×16） | **FSMC，与屏共用 D0~D15**（占 NE1~NE3 之一，**具体 bank 待确认**） |
| NOR FLASH | `W25Q16JVSSIQ`（2 MB，Winbond） | SPI |
| EEPROM | `U3`（丝印尾 `C027`，疑 AT24C02 系） | I2C |
| SD 卡 / NRF24L01 / RTC 电池 | `SD卡` 座 / `NRF24L01 模块` 座 / `CR1220` 座 | SPI / — |
| IO 引出 | 左/右 `A/B/C/D/F/G` 全 IO 排针（`MCU所有IO引出排针`） | — |

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

### 芯片内实测核对（2026-09-15，只读）

> 上表引脚已用 ST-Link 直读寄存器**逐条核对一致**，原始读数 + 解码见
> [`docs/hardware-truth/HARDWARE-TRUTH.md`](docs/hardware-truth/HARDWARE-TRUTH.md) §2。

| 实测项 | 结论 |
|---|---|
| 数据线 PD14/PD15/PD0/PD1/PE7~PE15/PD8/PD9/PD10 | 全部 **复用推挽 50 MHz**（`CRL/CRH` = `B`） ✅ |
| PG0 / PG12 | **复用推挽**（FSMC A10 / NE4） ✅ |
| PG15（RST） | **通用推挽**，ODR + IDR 实测为高 → 复位已释放 ✅ |
| PB0（背光） | **通用推挽**，ODR + IDR 实测为高 → 背光点亮 ✅ |
| PD4 / PD5（RD/WR） | 空闲实测为高 ✅ |
| PB6 / PB7（触摸 I2C） | 实测为**浮空输入**（未配 I2C，与"未使用"一致） ✅ |
| GPIOC 相关引脚（PC1/PC2） | ⚠️ **GPIOC 时钟未开**（`APB2ENR` bit4 = 0）→ 其寄存器读数**不可作判据**，标 `TODO(待确认)` |
| 未使用外设 | `APB1ENR = 0`；`APB2ENR` 仅 GPIOB/D/E/F/G；未开 AFIO → **无重映射、无中断外设** |

### 6 条必须遵守的配置（改错就白屏）

1. FSMC 时序 `ADDSET=15`、`DATAST=255`（本板 72MHz；教程按 36MHz 写的过快）
   —— **已实测确认**：芯片内 `FSMC_BTR4`（NE4）= `0x0FFFFFFF` → ADDSET = 15、DATAST = 255
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

# ⑤ 只读寄存器核实（时钟 / FSMC / GPIO / 器件签名 / 选项字节）
& $cli -c SWD -Q -List                      # 器件身份、Flash 容量、目标电压
& $cli -c SWD HOTPLUG -Q -r32 0x40021000 8  # RCC：CR, CFGR
& $cli -c SWD HOTPLUG -Q -r32 0xA0000000 32 # FSMC：BCR/BTR（NE4 = BCR4/BTR4）
& $cli -c SWD HOTPLUG -Q -r32 0x40012000 16 # GPIOG；D=0x40011400, E=0x40011800, B=0x40010C00
& $cli -c SWD HOTPLUG -Q -r32 0x1FFFF7E0 16 # 器件签名 + 96-bit UID
& $cli -c SWD HOTPLUG -Q -r32 0x1FFFF800 16 # 选项字节（**只读**）
```

> ⚠️ **只读**。`-ob`（写选项字节）、擦除、烧录一律**人工执行**（§5 闸门）。
> ⚠️ **未开时钟的外设，其寄存器读数不可作判据**（例：GPIOC 时钟未开 → `CRL/CRH/IDR` 读数无意义）。
> ⚠️ `BTRx` 的复位值是 `0x0FFFFFFF`（全 1），与 `ADDSET=15/DATAST=255` 数值巧合相同 →
> **不能由读数区分"固件写入生效"还是"保持复位值"**，只能确认"芯片内实际时序 = 15/255"。

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
| 读到的 GPIO/外设寄存器全是复位值或 0 | **该外设时钟未开**，读数无效 | 先查 `RCC->APB2ENR/AHBENR`，再下结论 |
| 以为"固件写的时序没生效" | `BTR` 复位值本就是 `0x0FFFFFFF`（全 1） | 比对复位值，勿用读数反推是否写入 |
| 读寄存器后屏幕动画重启 | 不带 `HOTPLUG` 时每次连接都会复位目标 | 读稳态用 `HOTPLUG`；`RCC_CSR.SFTRSTF` 可佐证 |

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
| 2026-09-15 | §1 硬件事实改为**芯片内实测结果**（时钟树 / FSMC NE4 时序 / 全部 GPIO 配置 / 选项字节 / UID），引脚表逐条核对一致 | AI |
