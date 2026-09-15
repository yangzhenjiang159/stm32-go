# 硬件档案（Hardware Profile）

> **AI 每次开工前请先读本文件。**
> 这里记录开发板信息、接线表、已调试完成的器件结论、以及**只能在实物上确认的坑**。
> 调试新器件时，在"待调试器件"里按模板追加一节即可。

---

## 1. 开发板

| 项目 | 信息 |
|---|---|
| 开发板 | 尚硅谷 STM32 开发板，PCB 丝印 **`v1.01 -204`**（随附原理图 V1.0 / 2024-08-08） |
| MCU | **STM32F103ZET6**（High-density），512KB Flash / 64KB RAM；ST-Link Device ID = `0x414` |
| 系统时钟 | HCLK = **72MHz**（HSE 8MHz × 9） |
| 下载器 | ST-Link V2，SN `066EFF505375485067123748`，固件 `V2J43S0`，虚拟串口 **COM6** |
| SWD | 4MHz，目标电压 3.4~3.6V |
| 原理图 | `D:\BaiduNetdiskDownload\嵌入式\9.尚硅谷STM32全套教程\基础篇\02_资料\05_原理图\STM32-F103ZET6开发板.pdf` |
| 教程代码 | `...\基础篇\03_代码\stm32\`（编号 01~42 的实验工程） |
| 模块资料 | `...\基础篇\02_资料\04_模块手册\`（01_I2C / 02_flash / 03_SRAM / **04_lcd** / 05_w5500 / 06_ESP32-C3 / 07_LoRa） |
| 提前验证 hex | `...\基础篇\02_资料\01_提前验证\04_测试程序\`（流水灯/呼吸灯/串口/I2C/SPI/液晶/CAN/RTC 等 10 个 hex） |

> **2026-09-15 芯片内实测复核**（ST-Link 直读寄存器，只读）：
> UID = `0x57058817 37375547 05D8FF36`、DBGMCU `REV_ID = 0x1003`、Flash 512 KB（实测）、
> HCLK 72 MHz / APB1 36 MHz / APB2 72 MHz（`RCC_CFGR = 0x001D040A`）、
> FSMC NE4 `ADDSET=15 / DATAST=255`、全部 LCD 引脚配置与电平逐条核对一致、
> `RDP = 0xA5`（未启用读保护）。
> 完整原始读数与解码 → [`docs/hardware-truth/HARDWARE-TRUTH.md`](docs/hardware-truth/HARDWARE-TRUTH.md) §2。
> 仍需人 + 仪器确认的项（丝印照片 / 电源 / 晶振负载电容 / BOOT0 下拉 / 触摸 IC）→ 同文件 §6。

### 1.1 工具链（无需 Keil）

| 项目 | 信息 |
|---|---|
| 编译器 | GNU Arm Embedded **10.3**，位于工作区 `gcc-arm/gcc-arm-none-eabi-10.3-2021.10/bin` |
| 说明 | 资料里的 `armcc/armasm` **需要授权**（`A9555E: Failed to check out a license`），不可用 |
| 编译脚本 | `lcd_love/build_gcc.ps1` |
| 烧录工具 | `C:\Program Files (x86)\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe` |

### 1.2 常用命令

```powershell
# 编译
powershell -File lcd_love\build_gcc.ps1

# 烧录 + 运行
& 'C:\Program Files (x86)\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe' `
  -c SWD -Q -P 'lcd_love\Obj\lcd_love.hex' -V -Rst -Run

# 读目标内存（诊断用；注意：不带 HOTPLUG 时每次连接都会复位目标）
& 'C:\...\ST-LINK_CLI.exe' -c SWD HOTPLUG -Q -r32 0x2000F000 16

# 器件信息
& 'C:\...\ST-LINK_CLI.exe' -c SWD -Q -List
```

---

## 2. 已调试完成的器件

### 2.1 ✅ 3.5 寸 TFT 液晶屏（FSMC 并口）

| 项目 | 信息 |
|---|---|
| 屏幕 | 3.5"，**320×480 竖屏**，2×14（28 pin）排针直插开发板 LCD 座 |
| 驱动 IC | 实测为 **ILI9488 风格**（读 `0xD3` 返回 `0x9488`）⚠️ 资料目录名为 ILI9486，照抄其序列会白屏 |
| 驱动 IC 为何查不到型号 | 驱动 IC 是 **COG 裸片**（金凸点压焊在玻璃边、被边框遮挡）→ 原理图未标、照片也拍不到，**只能靠实测风格判定**（不是遗漏，是物理限制） |
| 屏模块 PCB | 版本丝印 **`V1.0-198`**（⚠️ 与开发板 `v1.01 -204` 是**两块不同的板**，勿混） |
| 触摸控制器 | **`FT5316WE`**（敦泰 / FocalTech 电容触摸，QFN 封在触摸排线上，**本工程未使用**）。⚠️ 原理图网络名却叫 `GT-INT`/`GT-RST`（Goodix 风格）→ **别照 GT911 写驱动** |
| 详细结论 | **见 [`lcd_love/README.md`](lcd_love/README.md)** |

**接线表（已与原理图核对一致）**

| 信号 | MCU 引脚 |
|---|---|
| DB0~DB15 | PD14, PD15, PD0, PD1, PE7, PE8, PE9, PE10, PE11, PE12, PE13, PE14, PE15, PD8, PD9, PD10 |
| LCD_CS | **PG12**（FSMC_NE4） |
| LCD_RS | **PG0**（FSMC_A10） |
| LCD_WR | **PD5**（FSMC_NWE） |
| LCD_RD | **PD4**（FSMC_NOE） |
| LCD_RST | **PG15** |
| LCD_LED（背光） | **PB0**（高电平点亮） |
| 触摸 GT-INT / GT-RST | PC1 / PC2（未使用） |

**必须遵守的 6 条配置**

1. FSMC 时序 `ADDSET=15`、`DATAST=255`（本板 72MHz；教程按 36MHz 写的会白屏）
2. 初始化序列用 ILI9488 风格，**必须含 `F7h`**
3. 像素格式 `0x3A = 0x55`（16bit/像素）
4. 扫描方向 `0x36 = 0x08`
5. 复位脚 `PG15`（官方例程的 `PC5` 在本板无效）
6. 行列地址 `0x2A/0x2B` 各写 **4 次 8 位值（字节流）**

**⚠️ 已知不可靠（勿再尝试）**

- 屏幕 **GRAM 读回恒为 `0xFCFC`** → 不能用"读回校验"做自动诊断
- **读 ID 不稳定**（`0x04` 返回错误值，只有 `0xD3` 偶发正确）
- 资料里的 `3.50LCD焊接37pin-ILI9486技术资料` 面向 **37pin 模块**，与本板 **28pin** 接口不匹配，**其 `Template.hex` 在本板必然白屏**

---

## 3. 待调试器件（模板）

> 调试新器件前，请按下面格式补一节，AI 直接按此信息开工。

### 模板

```
### ⬜ <器件名称>

| 项目 | 信息 |
|---|---|
| 型号 / 丝印 |（把模块正面/背面的丝印文字抄下来，如 "SHT30"、"0.96 OLED SSD1306"）|
| 尺寸 / 分辨率 |（显示屏类必填）|
| 资料位置 |（手册、例程的完整路径；没有就写"无"）|
| 接口 |（I2C / SPI / UART / 并口 / ADC / GPIO…）|
| 工作电压 |（3.3V / 5V）|
| 供电来源 |（开发板 3V3 / 5V 排针 / 外部电源）|

**接线表**

| 模块引脚 | 开发板/MCU 引脚 | 备注 |
|---|---|---|
| VCC | | |
| GND | | |
| SDA/CLK/… | | |

**期望现象**：（我认为正常情况下应该看到什么）

**已知可工作的参考**：（有没有厂商提供的、已验证能跑的 hex/例程？）
```

### 3.1 ⬜ （下一个器件：请在此追加）

---

## 4. 通用提醒

1. **先看原理图再接线**，并核对"网络标号 ↔ MCU 引脚"（本次核对方法：用脚本提取 PDF 文本 + 坐标，判断是否同行）。
2. **供电务必先确认**（3.3V/5V 别接错）；接传感器/模块前先断开，接好再上电。
3. **能接串口就接**（USB-TTL → PA9/PA10）：固件 `printf` 诊断比任何猜测都有效。
4. **一次只改一个变量**，每轮烧录前说清"这轮验证什么假设"。
5. **结论回写文档**（本文件 + 器件专属 README）。
