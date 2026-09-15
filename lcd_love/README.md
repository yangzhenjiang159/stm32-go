# STM32F103ZET6 + 3.5 寸 TFT 液晶屏（FSMC）调试笔记

> 本文档记录本工作区的**硬件信息**、**屏幕点亮全过程**与**踩过的坑**，
> 供后续 AI / 开发者直接复用，避免重复排查同样的白屏问题。
>
> 首次调试耗时很长（十几轮对照测试），**结论都在第 2、3 节**，可直接照抄。

---

## 1. 硬件信息

| 项目 | 信息 |
|---|---|
| 开发板 | 尚硅谷 STM32 开发板，PCB 丝印 **`v1.01 -204`**（随附原理图版本 V1.0 / 2024-08-08） |
| MCU | **STM32F103ZET6**（High-density），512KB Flash / 64KB RAM，ST-Link 读回 Device ID = `0x414` |
| 屏幕 | **3.5 寸 TFT，320×480 竖屏**，2×14（28 pin）排针直接插在开发板 LCD 座 |
| 屏幕控制 IC | 实测响应符合 **ILI9488 风格**（读 `0xD3` 返回 `0x9488`），但**必须使用第 2 节的初始化序列** |
| 下载器 | ST-Link V2，SN `066EFF505375485067123748`，固件 `V2J43S0`，虚拟串口 COM6 |
| 工具链 | GNU Arm Embedded Toolchain 10.3（位于工作区 `gcc-arm/`），**不需要 Keil** |
| SWD | 4MHz，目标电压 3.4~3.6V，供电正常 |

### 1.1 屏幕引脚映射（开发板 LCD 座 ↔ MCU）

| 信号 | MCU 引脚 | 说明 |
|---|---|---|
| DB0 ~ DB15 | PD14, PD15, PD0, PD1, PE7, PE8, PE9, PE10, PE11, PE12, PE13, PE14, PE15, PD8, PD9, PD10 | FSMC 数据线 |
| LCD_CS（片选） | **PG12** | FSMC_NE4 |
| LCD_RS（命令/数据选择） | **PG0** | FSMC_A10 |
| LCD_WR | **PD5** | FSMC_NWE |
| LCD_RD | **PD4** | FSMC_NOE |
| LCD_RST（复位） | **PG15** | 低电平复位 |
| LCD_LED（背光） | **PB0** | 高电平点亮 |
| 触摸（本工程未用） | GT-INT / GT-RST / I2C1-SDA / I2C1-SCL | 电容触摸接口 |

FSMC 地址映射：命令口 `0x6C000000`（A10=0），数据口 `0x6C000800`（A10=1）。

### 1.2 原理图核对结论

对 `02_资料/05_原理图/STM32-F103ZET6开发板.pdf` 逐条提取网络标号 + 坐标比对，
**板级接线部分是准确可信的**：

| 网络 | 原理图引脚 | 代码/实测使用 | 结论 |
|---|---|---|---|
| LCD-BG（背光） | **PB0**（第 2 页与 `PB0` 同行） | PB0 | ✅ 一致 |
| LCD-RST（复位） | **PG15** | PG15 | ✅ 一致 |
| FSMC-NE4（CS） | **PG12** | PG12 | ✅ 一致 |
| FSMC-A10（RS） | **PG0** | PG0 | ✅ 一致 |
| FSMC-NWE（WR） | **PD5** | PD5 | ✅ 一致 |
| FSMC-NOE（RD） | **PD4** | PD4 | ✅ 一致 |
| FSMC-D0~D15 | PD14/PD15/PD0/PD1/PE7~PE15/PD8/PD9/PD10 | 同 | ✅ 一致 |
| GT-INT / GT-RST（触摸） | PC1 / PC2 | 未使用 | ✅ 合理 |

**但原理图与配套资料有 3 处"缺信息/不匹配"，正是本次白屏的根源：**

1. **原理图未标注屏幕驱动 IC 型号** → 实际屏响应 **ILI9488 风格**，
   而资料目录名却写 **ILI9486**，极易被误导（照它的序列初始化会全白）。
2. **资料里的模块例程不是给这块屏的**：
   `02_资料/04_模块手册/04_lcd/3.50LCD焊接37pin-ILI9486技术资料` 面向的是 **37 pin 焊接模块**，
   而本开发板 LCD 座是 **2×14（28 pin）**；且其 `main.c` 接线注释写 **`LCD_RST = PC5`**，
   与原理图的 **PG15** 不符 →
   **直接烧它的 `Template.hex` 必然白屏，不能用它判断硬件好坏**。
3. **原理图不体现 FSMC 时序要求**：教程与官方代码按 **HCLK=36MHz** 写时序，
   本板实测 **72MHz** 下必须放大 `ADDSET/DATAST`，否则白屏（原理图看不出这一层）。

> 结论：**接线照原理图抄没问题；初始化序列、时序、驱动 IC 参数必须按本文档第 2 节来，不能照抄资料里的模块例程。**

---

## 2. 必须遵守的配置（照抄即可）

| # | 项目 | 值 | 不这么做的后果 |
|---|---|---|---|
| 1 | **FSMC 时序** | `ADDSET=15`、`DATAST=255`（HCLK=72MHz） | 教程/官方按 36MHz 写的时序太快 → **纯白屏** |
| 2 | **初始化序列** | ILI9488 风格，**必须含 `F7h` (Adjust Control 3)** | 缺 `0xF7` → 白屏 |
| 3 | **像素格式** | `3Ah = 0x55`（16bit/像素，每像素 1 次 16 位写） | 用 `0x66` 会导致数据量错位、画面只铺 2/3 |
| 4 | **扫描方向** | `36h = 0x08`（320×480 竖屏） | 方向错误 → 内容跑到屏幕外 |
| 5 | **复位脚** | `PG15` | 官方例程写的是 `PC5`，在本板上无效 → 白屏 |
| 6 | **行列地址写法** | `0x2A/0x2B` **各写 4 次 8 位值（字节流）** | 改成 2 次 16 位值 → 窗口地址错乱，内容挤成一条细线 |

### 2.1 关键代码片段

```c
/* 时序：必须在 FSMC_Init() 之后覆盖 */
FSMC_Bank1->BTCR[7] &= ~((uint32_t)0xF | ((uint32_t)0xFF << 8));
FSMC_Bank1->BTCR[7] |= (uint32_t)15 | ((uint32_t)255 << 8);

/* 初始化序列关键命令 */
LCD_WriteCmd(0xF7); LCD_WriteData(0xA9); LCD_WriteData(0x51); LCD_WriteData(0x2C); LCD_WriteData(0x82);
LCD_WriteCmd(0xC0); LCD_WriteData(0x11); LCD_WriteData(0x09);
LCD_WriteCmd(0xC1); LCD_WriteData(0x41); LCD_WriteData(0x00);
LCD_WriteCmd(0xC5); LCD_WriteData(0x00); LCD_WriteData(0x0A); LCD_WriteData(0x80);
LCD_WriteCmd(0xB1); LCD_WriteData(0xB0); LCD_WriteData(0x11);
LCD_WriteCmd(0xB4); LCD_WriteData(0x02);
LCD_WriteCmd(0xB6); LCD_WriteData(0x02); LCD_WriteData(0x22);
LCD_WriteCmd(0xB7); LCD_WriteData(0xC6);
LCD_WriteCmd(0xBE); LCD_WriteData(0x00); LCD_WriteData(0x04);
LCD_WriteCmd(0xE0); /* 15 字节正伽马 */ LCD_WriteCmd(0xE1); /* 15 字节负伽马 */
LCD_WriteCmd(0x3A); LCD_WriteData(0x55);      /* 16bit/像素 */
LCD_WriteCmd(0x36); LCD_WriteData(0x08);      /* 扫描方向 */
LCD_WriteCmd(0x11); Delay_ms(120);            /* 退出睡眠 */
LCD_WriteCmd(0x29);                           /* 开显示 */

/* 窗口：8 位字节流写法（本屏要求） */
LCD_WriteCmd(0x2A);
LCD_WriteData(x >> 8); LCD_WriteData(x & 0xFF);
LCD_WriteData((x + w - 1) >> 8); LCD_WriteData((x + w - 1) & 0xFF);
LCD_WriteCmd(0x2B);
LCD_WriteData(y >> 8); LCD_WriteData(y & 0xFF);
LCD_WriteData((y + h - 1) >> 8); LCD_WriteData((y + h - 1) & 0xFF);
LCD_WriteCmd(0x2C);                            /* 开始写像素 */
```

---

## 3. 踩坑记录（现象 → 根因 → 解法）

### 坑 1：烧录教程官方 hex 后**纯白屏**
- **现象**：烧 `03_代码/stm32/41_lcd_register/Objects/led_register.hex`、以及资料里的官方 ILI9486 例程 `Template.hex`，屏幕全白无任何变化。
- **根因**：教程/官方例程的 FSMC 时序是按 **HCLK=36MHz** 写的（注释里写着 `1/36M=27ns`），而本板实际跑 **72MHz**，导致写不进数据。
- **解法**：把 `ADDSET` 设为 15、`DATAST` 设为 200+（实测 255 最稳），画面立刻出现。

### 坑 2：官方 `Template.hex` 也不亮
- **根因**：官方例程的复位脚是 **PC5**（见其 `main.c` 接线注释），而本开发板 `LCD-RST` 接的是 **PG15**。
  官方程序在本板上根本没给屏幕复位，白屏属正常。
- **推论**：**不能用官方 hex 判断硬件好坏**，必须按本板的引脚映射自己写。

### 坑 3：画面能出，但所有内容都挤成"一侧的一条细线"
- **现象**：横条、竖条、方块、汉字，全都显示成屏幕边缘的很细一条。
- **根因**：`0x2A/0x2B` 的行列地址参数写法。本屏要求**每次写 8 位（共 4 次）**的字节流写法；
  改成"每次写 16 位（共 2 次）"后窗口地址完全错乱。
- **定位方法**：写对照测试固件，同一图案分别用两种写法轮流显示，看哪种正常。
- **注意**：全屏纯色填充在两种写法下"看起来都正常"，**不能用纯色测试判断**，必须用分离的小图形（四角方块）。

### 坑 4：爱心/文字画不出来，或位置乱
- **根因**：同上（窗口寻址）+ 时序处于临界状态时，小尺寸窗口比全屏填充更容易失败。
- **解法**：保持 `DATAST=255`，窗口写法严格按第 2 节第 6 条。

### 坑 5：想通过"读回像素"自动校验 —— 不可行
- **现象**：用 `0x2E`(Read Memory Start) 读 GRAM，**所有坐标都恒返回 `0xFCFC`**，与写入颜色无关；
  用 `0x04` 读 ID 返回 `0x548066`（错误值）。
- **结论**：这条屏的**读通路不可靠**（只有 `0xD3` 读 ID 偶尔返回正确值 `0x9488`）。
- **推论**：**不要设计依赖"读回校验"的自动诊断**，必须用"目视 + 分阶段对照"的方式定位。

### 坑 6：没有 Keil、资料里的 armcc 又报授权失败
- **现象**：`armasm/armcc` 报 `Error: A9555E: Failed to check out a license.`
- **解法**：下载 **GNU Arm Embedded Toolchain 10.3**（免授权），用 `arm-none-eabi-gcc` 编译，见第 5 节。
- **附加**：老版 CMSIS 的 `core_cm3.c` 与新 GCC 汇编不兼容，**不要编译它**（其中的弱函数用不到）。

### 坑 7：爱心形状不圆润
- **初版**：两个实心圆 + 直线三角形拼成 → 有明显折角。
- **二版**：隐式方程 `(x²+y²-1)³ - x²y³ ≤ 0` → 中间段几乎是垂直边（像圆角方块）。
- **现版**：参数方程 `x = 16sin³t, y = 13cos t - 5cos2t - 2cos3t - cos4t` 生成轮廓点表 + 扫描线填充。
  顶部双凸、底部尖角都很好；**两侧仍略偏直**（x 峰值附近变化慢）。
- **进一步优化方向**：在 `tools/gen_heart.py` 里改用**贝塞尔曲线**拟合轮廓（控制点决定两侧弧度），生成后重新编译即可。

---

## 4. 工程结构

```
lcd_love/
├─ User/
│  ├─ main.c            # 主程序（轮播“老婆我爱你!” + 爱心 + 配色轮播）
│  ├─ delay.c/.h        # SysTick 延时
│  ├─ love_font.h       # 生成的汉字点阵（32x32）
│  └─ heart_table.h     # 生成的心形轮廓点表
├─ Interface/LCD/       # 教程的 LCD 驱动（lcd.c 提供底层 写命令/写数据/复位/背光）
├─ Hardware/FSMC/       # FSMC + GPIO 初始化（fsmc.c）
├─ GCC/                 # GCC 用的 CMSIS、启动文件、链接脚本（stm32_flash.ld：512K Flash/64K RAM）
├─ tools/
│  ├─ gen_font.py       # 用 Windows GDI 取汉字点模 → User/love_font.h
│  └─ gen_heart.py      # 生成心形轮廓点表 → User/heart_table.h
├─ build_gcc.ps1        # 一键编译（arm-none-eabi-gcc + objcopy → Obj/lcd_love.hex）
└─ Obj/                 # 编译产物（lcd_love.elf / .hex / .map）
```

---

## 5. 编译与烧录

```powershell
# 编译 → Obj/lcd_love.hex
powershell -File lcd_love\build_gcc.ps1

# 烧录并运行（ST-LINK_CLI）
& 'C:\Program Files (x86)\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe' `
  -c SWD -Q -P 'lcd_love\Obj\lcd_love.hex' -V -Rst -Run

# 仅复位运行
& 'C:\...\ST-LINK_CLI.exe' -c SWD -Rst -Run

# 读回目标内存（调试用，例如把诊断结果写到 0x2000F000）
& 'C:\...\ST-LINK_CLI.exe' -c SWD HOTPLUG -Q -r32 0x2000F000 16
```

> 注意：`ST-LINK_CLI` **每次连接都会复位目标**。要读"运行中"的内存，用 `HOTPLUG` 参数。

---

## 6. 修改内容的方法

| 想改什么 | 改哪里 | 操作 |
|---|---|---|
| 显示的文字 | `tools/gen_font.py` 里的 `CHARS` | 改完运行 `python tools\gen_font.py` → 重新编译 |
| 爱心形状 | `tools/gen_heart.py`（贝塞尔控制点） | 运行脚本生成 `User/heart_table.h` → 重新编译 |
| 爱心大小 | `User/main.c` 里 `DrawHeart(..., r, ...)` | 当前 r = 33 / 38 / 46（心跳三档） |
| 爱心位置 | `User/main.c` 里的 `HEART_CY`（当前 170） | |
| 文字位置 | `User/main.c` 里的 `TEXT_Y`（当前 330）、`TEXT_X0` 自动居中 | |
| 配色方案 | `User/main.c` 里的 `g_palette[][2]` 数组 | `{前景色, 背景色}`，共 8 套循环 |
| 动画节奏 | `User/main.c` 各处的 `Delay_ms(...)` | |

---

## 7. 调试方法论（可复用于其它屏）

1. **先用 ST-Link 读回 MCU 内部状态**（把寄存器值写进固定 RAM 如 `0x2000F000`，
   再用 `-r32` 读回），确认 GPIO/FSMC/时钟配置无误，先把"软件配置错误"排除干净。
2. **只改一个变量**做对照测试，同一图案分阶段轮流显示，让现象自解释。
3. **纯色测试不足以判断窗口寻址**（全屏填充容易"看起来正常"），要用**分离的小图形**（四角方块）。
4. **分步验证**：先"四角定位"证明坐标系正确，再"单个 32×32 汉字"证明字模绘制正确，最后合成完整画面。
5. **先信号后内容**：白屏/花屏优先怀疑 **FSMC 时序**（降速验证），而不是先怀疑初始化序列。
