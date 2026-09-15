# STM32F103ZET6 + 3.5" TFT（FSMC）—— 工作区说明

本工作区用 **ST-Link + 命令行工具链**（不需要 Keil）驱动尚硅谷 STM32F103ZET6 开发板上的
3.5 寸 TFT 液晶屏，当前效果：**循环轮播"老婆我爱你!"**（爱心跳动 + 逐字出现 + 左右滑动 + 8 套配色）。

> 🚀 **AI / 新会话请先读 [`AGENTS.md`](AGENTS.md)** —— 硬件事实 + 编码硬约束 + 烧录审批闸门（**唯一事实来源**）
> 📌 事实的出处与置信度：[`docs/hardware-truth/HARDWARE-TRUTH.md`](docs/hardware-truth/HARDWARE-TRUTH.md)（阶段 0 核查产物 + 冲突登记）
> 📊 上下文快照，一分钟掌握全貌 → [`PROJECT_STATE.md`](PROJECT_STATE.md)
> ✅ 开工前对照打勾 → [`docs/前置工作核查表.md`](docs/前置工作核查表.md)
> 📌 详细调试笔记 / 踩坑记录 / 全部硬件参数 → [`lcd_love/README.md`](lcd_love/README.md)

---

## 硬件速查（重要，勿改错）

| 项目 | 值 |
|---|---|
| 开发板 | 尚硅谷 STM32 开发板，丝印 `v1.01 -204` |
| MCU | STM32F103ZET6（512KB Flash / 64KB RAM），Device ID `0x414` |
| 屏幕 | 3.5 寸 TFT，**320×480 竖屏**，2×14 排针直插 LCD 座 |
| 屏驱动 IC | ILI9488 风格（读 `0xD3` = `0x9488`） |
| 下载器 | ST-Link V2，SN `066EFF505375485067123748`，固件 `V2J43S0` |
| 工具链 | GNU Arm Embedded 10.3（`gcc-arm/`，免授权） |

**屏幕接线**：DB0-15 = PD14/PD15/PD0/PD1/PE7-PE15/PD8/PD9/PD10；
CS = **PG12**(NE4)，RS = **PG0**(A10)，WR = **PD5**，RD = **PD4**，RST = **PG15**，背光 = **PB0**。

---

## 六条必须遵守的配置（否则白屏/画面错乱）

| # | 项目 | 正确值 |
|---|---|---|
| 1 | FSMC 时序 | `ADDSET=15`、`DATAST=255`（本板 HCLK=72MHz，教程按 36MHz 写的太快） |
| 2 | 初始化序列 | ILI9488 风格，**必须含 `F7h`** |
| 3 | 像素格式 | `0x3A = 0x55`（16bit/像素） |
| 4 | 扫描方向 | `0x36 = 0x08` |
| 5 | 复位脚 | **PG15**（官方例程写的 `PC5` 在本板无效） |
| 6 | 行列地址 | `0x2A/0x2B` 各写 **4 次 8 位值**（字节流写法） |

**已知不可靠**：屏幕的 GRAM 读回恒为 `0xFCFC`，读 ID 也不稳定 → **不要设计依赖"读回校验"的诊断**。

---

## 一键编译 + 烧录

```powershell
powershell -File lcd_love\build_gcc.ps1
& 'C:\Program Files (x86)\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe' `
  -c SWD -Q -P 'lcd_love\Obj\lcd_love.hex' -V -Rst -Run
```

## 调试协作（换新器件前必读）

| 文件 | 用途 |
|---|---|
| [`AGENTS.md`](AGENTS.md) | **AI 事实来源 + 硬约束 + 烧录闸门**（`.codebuddy/rules/` 里的规则随会话自动加载） |
| [`docs/hardware-truth/`](docs/hardware-truth/HARDWARE-TRUTH.md) | **阶段 0 核查产物**：带出处与置信度的硬件事实 · 冲突登记 · 待实物验证清单 |
| [`docs/前置工作核查表.md`](docs/前置工作核查表.md) | **开工前打勾清单**：阶段 0~5 对照现状 + 剩余待办 |
| [`HARDWARE.md`](HARDWARE.md) | **硬件档案**：开发板信息、接线表、已调试器件结论、**新器件填写模板** |
| [`DEBUG_WORKFLOW.md`](DEBUG_WORKFLOW.md) | **协作调试指南**：怎样描述现象最高效、拍照放工作区、AI 侧调试协议 |
| [`lcd_love/README.md`](lcd_love/README.md) | 3.5" TFT 完整调试笔记（含原理图核对结论、7 个坑） |

> 接新器件时：先照 `HARDWARE.md` 第 3 节的模板补一节（型号/资料/接线/期望现象），
> 再按 `DEBUG_WORKFLOW.md` 的方式反馈现象，可省掉大量来回。

## 目录

```
AGENTS.md         AI 事实来源 + 硬约束 + 烧录闸门（AI 先读这个）
.codebuddy/       CodeBuddy 项目规则（rules/embedded-firmware/RULE.mdc，自动加载）
PROJECT_STATE.md  项目状态快照
HARDWARE.md       硬件档案（新器件调试前先看）
DEBUG_WORKFLOW.md AI × 人 协作调试指南
docs/             方法论 6 篇
  └ hardware-truth/  阶段 0 核查产物（硬件事实 / 冲突登记）
lcd_love/         屏幕工程源码 + README.md（详细调试笔记）
snapshots/        把手机拍的屏幕/接线照片放这里，AI 可直接读取分析
gcc-arm/          GNU Arm Embedded 10.3 工具链（免授权，解压即用）
```
