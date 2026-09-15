# 项目状态速览（AI 上下文压缩版）

> 这是当前工作的**上下文快照**。新会话请**先读 [`AGENTS.md`](AGENTS.md)**（硬件事实 + 硬约束 + 烧录闸门，唯一事实来源），
> 再看本快照；需要细节再查：
> [`docs/hardware-truth/HARDWARE-TRUTH.md`](docs/hardware-truth/HARDWARE-TRUTH.md)（带出处的事实/冲突登记）· [`docs/前置工作核查表.md`](docs/前置工作核查表.md)（开工前打勾）· [`HARDWARE.md`](HARDWARE.md)（硬件档案）· [`DEBUG_WORKFLOW.md`](DEBUG_WORKFLOW.md)（协作调试指南）· [`lcd_love/README.md`](lcd_love/README.md)（完整踩坑记录）

**最后更新：2026-09-15**

---

## 一、一句话

尚硅谷 **STM32F103ZET6** 开发板 + **3.5" TFT**（FSMC 并口，屏为 **ILI9488 风格**），
已点亮并运行轮播程序：**爱心跳动 + "老婆我爱你!"逐字出现 + 左右滑动 + 8 套配色轮播**。

## 二、已完成

- ✅ 屏幕点亮（定位过程见 `lcd_love/README.md`，共 7 个坑）
- ✅ "老婆我爱你!"轮播动画（爱心跳动 / 逐字 / 滑动 / 配色轮播 / 上电自检）
- ✅ 心形由"两圆+三角"改为**曲线轮廓**（`heart_table.h` + 扫描线填充）
- ✅ 文档归档（硬件档案 / 协作调试指南 / 详细踩坑记录）
- ✅ 清理无用的 armcc 遗留（`lcd_love/Start/`、`lcd_love.sct`）与工具链 43.9MB 文档

## 三、当前代码与产物

| 文件 | 说明 |
|---|---|
| `lcd_love/User/main.c` | **主程序，通常只需要改这个文件** |
| `lcd_love/User/love_font.h` | 汉字点阵（由 `tools/gen_font.py` 生成） |
| `lcd_love/User/heart_table.h` | 心形轮廓点表（由 `tools/gen_heart.py` 生成） |
| `lcd_love/Obj/lcd_love.hex` | 当前固件（**已烧入开发板**） |
| `lcd_love/build_gcc.ps1` | 一键编译 |

## 四、必须记住的 6 条配置（改错就白屏）

1. FSMC 时序 `ADDSET=15`、`DATAST=255`
2. 初始化序列用 ILI9488 风格，**必须含 `F7h`**
3. 像素格式 `0x3A = 0x55`（16bit/像素）
4. 扫描方向 `0x36 = 0x08`（320×480 竖屏）
5. 复位脚 **PG15**（不是官方例程里的 PC5）
6. 行列地址 `0x2A/0x2B` 各写 **4 次 8 位值**（字节流写法）

## 五、硬件速查

| 项目 | 值 |
|---|---|
| 开发板 | 尚硅谷 STM32，丝印 `v1.01 -204`，HCLK = 72MHz |
| 屏 | 320×480；CS=**PG12**(NE4)，RS=**PG0**(A10)，WR=**PD5**，RD=**PD4**，RST=**PG15**，背光=**PB0** |
| 数据线 | PD14, PD15, PD0, PD1, PE7~PE15, PD8, PD9, PD10 |
| 下载器 | ST-Link V2，SN `066EFF505375485067123748`，固件 `V2J43S0` |
| 工具链 | `gcc-arm/gcc-arm-none-eabi-10.3-2021.10/bin`（**免授权，不需要 Keil**） |

## 六、编译 + 烧录

```powershell
powershell -File lcd_love\build_gcc.ps1
& 'C:\Program Files (x86)\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe' `
  -c SWD -Q -P 'lcd_love\Obj\lcd_love.hex' -V -Rst -Run
```

## 七、已知坑 / 不可靠手段（勿再尝试）

- 屏 **GRAM 读回恒为 `0xFCFC`** → 不能设计依赖"读回校验"的诊断
- 资料里的 `3.50LCD焊接37pin-ILI9486技术资料` 与本草**不匹配**（复位脚 PC5、其 hex 必白屏）
- **纯色全屏测试不能判断窗口寻址**是否正常，必须用**分离的小图形**（四角方块）
- `ST-LINK_CLI` 每次连接都会复位目标；读运行中内存需加 `HOTPLUG`

## 八、待办（可选）

### 前置工作补齐（详见 [`docs/前置工作核查表.md`](docs/前置工作核查表.md) §4）

- [x] 阶段 0 文档核查产物：`docs/hardware-truth/`（硬件事实 + 冲突登记）
- [x] AI 上下文与约束：`AGENTS.md` + `.codebuddy/rules/embedded-firmware/RULE.mdc`
- [ ] **`git init` + `.gitignore`/`.gitattributes`**（换回滚能力）
- [ ] **纯逻辑抽出来做 PC 单测**（验证闭环，当前最大短板）
- [x] **芯片内实测复核**：时钟树 / FSMC NE4 时序 / 全部 GPIO 配置 / 选项字节（`HARDWARE-TRUTH.md` §2）
- [x] 拍板卡照片 3 张 → `snapshots/20260915-整板正面/背面/MCU特写.jpg`（实物与原理图、芯片实测三方一致）
- [x] 拍屏模块背面 → **触摸 IC = `FT5316WE`**（显示驱动是 COG 裸片，物理上拍不到）
- [x] 确认丝印归属：**屏板 = `V1.0-198`**、**开发板 = `v1.01 -204`**（两块板，编号各自正确）
- [x] **阶段 0（Gate P0）基本达成** → 结论见 `HARDWARE-TRUTH.md` §9；可直接进入编码/新器件调试
- [ ] 补完其余待实物验证：3V3 裕量 / HSE 频偏 / BOOT0 下拉 / 触摸 IC

### 功能类

- [ ] 心形两侧弧度进一步优化：把 `tools/gen_heart.py` 的采样换成**贝塞尔曲线**
- [ ] 若需给屏加串口日志：接 USB-TTL 到 PA9/PA10，固件里串口打印诊断（**注意本工程无 `printf` 重定向，需自建**）
- [ ] 下一个器件调试：先按 `AGENTS.md` §1 补引脚、`HARDWARE.md` 第 3 节补器件节
