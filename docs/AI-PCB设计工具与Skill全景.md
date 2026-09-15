# AI PCB 设计工具与 Skill 全景（2026-09 快照）

> **定位**：本文是一份**选型地图**，回答「现在有哪些 AI 工具 / Agent Skill 能参与电路板设计与验证，各自能干什么、边界在哪」。
>
> **一句话结论**：**原理图/连接层已有可用工具，布局布线/电气判断仍必须人工把关**；真正能进工程流程的是「代码化 EDA + Skill/MCP 驱动既有 EDA + 独立验证工具」三件套，而不是"一句话出板子"的演示视频。
>
> **配套文档**：
> - `docs/AI辅助立创EDA电路设计与验证流程.md` —— **本文的落地实例**：嘉立创/立创EDA 的具体接入方法 + 四层验证流程。本文负责"有哪些可选"，那篇负责"选一个怎么用"。
> - `docs/硬件文档一致性核查方案.md` —— 阶段 0 文档核查，是设计输入的**前置门槛**。
> - `docs/AI用于嵌入式的最佳实践.md` / `docs/Win11-AI嵌入式工程落地方案.md` —— 固件侧护栏与验证闭环。
>
> **时效声明**：EDA 与 AI 生态迭代极快。本文所有版本号、工具数量、限制条件均为 **2026-09** 快照，使用前请以官方渠道复核。

---

## 0. 结论速览

| 问题 | 结论 |
|---|---|
| 有能一句话生成电路图的工具吗？ | 有，但产出是**原理图骨架**，秒级~分钟级；PCB 布局布线需人工或额外工具 |
| AI 能自己完成布局布线吗？ | 部分能（Quilter、DeepPCB 等自动 P&R），但高速 / 电源完整性 / EMC 关键走线仍需人 |
| 最稳的工程路线是什么？ | **代码化 EDA（可 diff / 可 CI）+ Skill/MCP 操作 KiCad + 独立 DFM/仿真工具** |
| 能直接用 AI 出的板子下单吗？ | **不能**。必须过 ERC → 网表比对 → DRC → DFM → 人工复核 → 首板实测 |
| Skill 生态成熟度？ | KiCad 侧最丰富（MCP + skill 套件），立创 EDA 侧有官方 Skill，但都属"辅助/审查"定位 |
| 当前本项目已装了 PCB 相关 Skill 吗？ | **没有**。需要自行从 GitHub 安装或注册 MCP（见 §7） |

---

## 1. 工具全景分层

```
┌─ 第 1 层  AI 原生平台 ──────── 自然语言 → 原理图/布局（Flux / Quilter / 华秋 AI EDA …）
├─ 第 2 层  传统 EDA 的 AI 增强 ── 在既有工作流里加 AI 助手（Altium / Cadence / Siemens / KiCad）
├─ 第 3 层  代码化 EDA ────────── 电路即代码，可 git diff / 可 CI（atopile / Zener / circuit-synth）
├─ 第 4 层  Skill / MCP 桥梁 ──── 让通用 AI 客户端操控既有 EDA（本次调研重点）
└─ 第 5 层  验证与检查 ────────── DFM / DRC / ERC / SPICE / SI-PI-EMC（人守最后一关）
```

**关键认知**：第 1 层看起来最省事，但**可控性最差**（元件会被静默替换、粒度粗）；第 3、4 层可控性最好，也最适合与版本控制和 CI 结合。**不要只按演示效果选型，要按"出错时能不能定位和回滚"选型。**

---

## 2. AI 原生平台（自然语言 → 原理图 / 布局）

| 工具 | 定位 | 特点 | 备注 |
|---|---|---|---|
| **Flux.ai** | 云端 Copilot | 自然语言直接生成 / 编辑原理图与 PCB，浏览器协作 | 需国际网络，云端托管 |
| **Quilter** | 自动布局布线 | 物理驱动 AI，输入完成态原理图，云端自动 place & route，可配合 Altium | SaaS，可与人工作流并行 |
| **DeepPCB** | 自动布线 | 强化学习优化布线，云原生 | 偏布线单点 |
| **pcbdesigner.ai** | 端到端 | 上传原理图 → 自动布局布线 → 导出 Gerber | 偏快速验证 |
| **ProtoFlow** | 原理图捕获 | AI 辅助出原理图，产出干净 KiCad 工程 | 免费起步 |
| **JITX / Celus** | 企业级生成 | 代码化 / AI 生成 | 商业报价 |
| **AllSpice** | 审查与 diff | 以设计审查、版本比对为主，非生成 | 适合 review 环节 |
| **华秋 AI EDA**（`hq.eda.cn`） | **国产一站式** | 2026-03 发布，国内首款深度融合大模型的 AI EDA Agent；自然语言生成原理图 + 智能选料 + 设计审查 + DFM + 一键下单 | **国内可直接用**；配套华秋 DFM |
| **嘉立创 EDA Pro** | 国产免费 | 内置 AI 助手 + 完整扩展 API，可挂 AI Agent 插件 | 详见配套文档 |
| **facetok EDA**（`facetok.cn`） | 国产工业级 | 自动原理图、PCB 布局布线、BGA Escape、高速 SerDes / DDR | 面向工业级 |
| **Blueprint.am**（3E8 Robotics） | Maker 级 | 自然语言 → 接线图 + BOM | 偏模块接线，非量产 PCB |
| **Schematik** | Maker 级（"Cursor for Hardware"） | 自然语言 → 源码 + 接线图 + 装配指导，主打 Arduino / ESP32 / Pico | **未见专业 PCB / Gerber 出图证据**；2026-04 获约 $4.6M 预种子 |

> ⚠️ 国际 SaaS 平台（Flux / Quilter / Celus / JITX 等）需确认**网络可达性、支付方式与合规政策**；国内稳定可用的是国产工具与通过中国区代理销售的国际商业 EDA。

---

## 3. 传统 EDA 的 AI 增强

| 厂商 / 工具 | AI 能力 | 说明 |
|---|---|---|
| **Altium Designer / Altium 365** | AI 助手、云端协作、DFM | 生态成熟，365 侧有云端审查能力 |
| **Cadence Allegro X / OrCAD X** | AI 布局布线优化 | 配套 Sigrity / Clarity / PSpice 做 SI-PI 与仿真 |
| **Siemens EDA**（Xpedition / PADS Pro / HyperLynx / Valor） | AI/ML 辅助布局、DRC/DFM | 2025 DAC 起发布 Aprisa AI、Calibre Vision AI 等 AI 工具集 |
| **KiCad** | 本身无 AI，靠插件 / MCP / skill 扩展 | **免费，生态扩展性最好**，是 AI 接入的主战场 |
| **Autodesk Fusion 360 Electronics / EAGLE** | AI 辅助 | 订阅制 |
| **Zuken CR-8000 / E3.series** | AI 辅助 | 商业报价 |

**KiCad 的地位特殊**：它是代码化 EDA 与 MCP 生态的**共同底座**。走代码化路线会产生 KiCad 工程，走 MCP 路线是驱动 KiCad。**无论选哪条路，布局与出 Gerber 都离不开 KiCad。** 当前稳定版为 **10.0.2**。

---

## 4. 代码化 EDA（硬件即代码）—— 最适合 AI Agent

共同哲学：把电路连接表达成**可读、可 diff、可 build 验证**的文本。

| 工具 | 语言 / 形态 | 成熟度与特点 |
|---|---|---|
| **atopile** | `.ato` 声明式 DSL → 编译出 KiCad 工程 / BOM / 制造文件 | **最成熟**（0.15.x，2026-04 密集发版，核心经重写）。前 Tesla 工程师创立，开源活跃。支持模块复用、git 版本控制、CI、带单位与容差的参数化、**断言式设计规则校验**、按约束自动选型 |
| **Zener**（Diode + Anthropic） | Starlark 之上的 PCB DSL，配套工具 `pcb` | **唯一有 Anthropic 一手背书**：官方博客《Making Claude a better electrical engineer》（2026-02-05）记述工程师用 Claude Code 把芯片非结构化文档读成完整 Zener 参考设计，Sonnet 4.5 上有可测增益；Diode 称两周产出约 250 个参考设计（工程师仍需逐一签字）。⚠️ 但对外的开源协议与独立可用性**未能一手证实**，需自行核实 |
| **circuit-synth** | Python（为 Claude Code 而生） | MIT 开源，内置 agents 与 skills（`circuit-patterns`、`component-search`），slash 命令 `/find-symbol`、`/generate-validated-circuit`、`/analyze-fmea`；产物覆盖 KiCad 工程 + Gerber + BOM + SPICE。v0.12.1（2026-01）。⚠️ 信息单源，生产成熟度需自评 |
| **SKiDL** | Python 库，生成 netlist | 老牌方案，配 SKiDL Skills 插件（9 个 AI 代理）降低使用门槛 |

### 4.1 与传统 GUI 路线的对比

| 维度 | MCP 直驱 KiCad | 代码化 EDA |
|---|---|---|
| AI 干的事 | 调工具操作 GUI 状态 | 写 `.ato` / `.py` 代码 |
| 需 GUI 常驻 | **是**（实时 IPC 要求 KiCad 开着） | **否**，命令行可编译出网表 |
| 无头 / agent 自动跑 | 低（多一层 GUI 依赖的中间件） | **高** |
| 版本控制友好度 | 差（KiCad 文件 diff 不友好） | **好**（纯文本，git 天然友好） |
| 可复用性 | 靠个人库 | 模块化 + 包管理器 |
| 验证方式 | DRC / ERC | `build` + 断言 + DRC / ERC |
| 学习曲线 | 低（会描述需求即可） | 需学 DSL / API |
| 适合谁 | 坐屏幕前、要实时反馈、在 KiCad 里收尾 | 想"硬件即代码"、无头 / CI / 可复审 |

**选型口诀**：要边改边看 KiCad → MCP 路线；要无头 / 自动跑 / git diff / 可复审 → 代码化路线。

---

## 5. 验证与检查工具

> 与配套文档的"四层验证"是同一套思想，这里只做**工具清单**，方法与清单见 `docs/AI辅助立创EDA电路设计与验证流程.md` §2。

### 5.1 可制造性 / DFM

| 工具 | 形式 | 说明 |
|---|---|---|
| **嘉立创 DFM**（`jlc-dfm.com`） | 在线，免安装 | 上传 Gerber / Altium / PCB 源文件，**30 多项**检测：线路、阻焊、丝印、钻孔；一键输出问题报告；支持 3D 看图 |
| **华秋 DFM** | 在线 / 客户端 | 与华秋 AI EDA 联动 |
| **望友 VayoPro** | 商业 | DFM / DFX 检查 |
| **Siemens Valor / Calibre Vision AI** | 商业 | 工业级 DFM / 光刻友好性检查 |

### 5.2 电路级仿真

| 工具 | 说明 |
|---|---|
| **LTspice**（ADI） | 免费，模拟电路仿真主力 |
| **ngspice** | 开源，**可被 AI 通过 MCP 驱动**（见 §6.2 Spicebridge） |
| **立创EDA 仿真** | 内置，覆盖无源 / 模拟小电路 |
| **PSpice**（Cadence） | 商业 |

> **边界**：SPICE 只覆盖无源 / 模拟小电路，**别指望它验证 MCU 逻辑与固件行为**。

### 5.3 SI / PI / EMC / 热

| 类别 | 工具 |
|---|---|
| 国际主流 | Ansys（SIwave / HFSS / Icepak / SimAI）、Keysight ADS / PathWave、Cadence Sigrity / Clarity、Siemens HyperLynx |
| 国产仿真 | 芯和 Xpeedic、巨霖、法动、飞谱、九同方、芯瑞微 |
| 规则库方案 | `kicad-happy` 的 **EMC 预合规**：44 条 FCC / CISPR / 汽车规则（见 §6.3） |

---

## 6. Skill / MCP 生态（本次调研重点）

### 6.1 能力矩阵

| Skill / MCP | 宿主 EDA | 覆盖阶段 | 协议 | 备注 |
|---|---|---|---|---|
| **`easyeda-api-skill`**（嘉立创官方） | 立创EDA Pro | 原理图生成 + DRC | Agent Skills | 官方维护，**上手最快**，粒度粗；需跑 `Run API Gateway` 桥接。详见配套文档 |
| **`pcb-skill`**（`daishuge/pcb-skill`） | EasyEDA Pro | 概念→原理图→选料→布局→布线→验证→下单 | Skill + MCP bridge | MCP 驱动 EasyEDA Pro；**下单停在支付页之前**；三条强制原则：实时计价、手工可装配、免费打样包络 |
| **`KiCAD-MCP-Server`**（`mixelpixx`） | KiCad | 原理图 / 符号封装生成 / 布线 / Freerouting / 导出 | MCP | 工具最全（号称 52~122 个工具），含 LCSC、JLCPCB 价格库存查询 |
| **`kicad-mcp-server`**（`lamaalrajih`） | KiCad | 分析校验：可视化、DRC、netlist 提取、BOM | MCP | 适合**审查排错**，非从零生成 |
| **`kicad-mcp-server`**（`bunnyf`，PyPI） | KiCad 9.x | 全套 | MCP | 可把 KiCad + kicad-cli + pcbnew + FreeRouting 跑在 VPS，本地 SSH 调用 |
| **`kicad-happy`** | KiCad | 审查 / 分析 / 选型 / 出 fab | Skill 集合（12 个） | 原理图 / PCB / Gerber 解析、SPICE 测试台、**EMC 预合规（44 条规则）**、datasheet 提取、BOM 全流程、Digikey / Mouser / LCSC / Element14 选型、JLCPCB / PCBWay 出 fab、文档生成。兼容 Claude Code / Codex / Copilot CLI / Gemini CLI |
| **`eda-pcb`**（`l3wi/claude-eda`） | KiCad | 布局走线（**需已有原理图/网表**） | Skill + MCP | 连接器优先摆放、晶振距 MCU ≤5mm、USB 差分 90Ω、按 JLCPCB 配 DRC 数值、**架构风险预警**（如"USB + 2 层板无法实现 90Ω"） |
| **`Spicebridge`** | ngspice | 电路仿真验证 | MCP | AI 直连 ngspice，自然语言 → 电路 → 仿真验证 |
| **`SPICEPilot`** | SPICE | 仿真 | 开源 | LLM 生成并运行 SPICE |
| **`datasheet-reader`** | KiCad | 文档读取 | Skill | 用 `pcb scan` 从文件 / URL / KiCad `Datasheet` 属性读数据手册 |
| **`zener-language`** | — | Zener `.zen` 文件 | Skill | 处理 Zener 语义、包 API、依赖工作流、验证 |
| **`jlceda-agent`** / **`JLCEDA AI Agent`** | 立创EDA Pro | 原理图 / PCB 上下文分析 | 开源框架 / 官方扩展 | 支持聊天问答、图片输入、文本文件上传 |

### 6.2 硬件类 Skill 的公开索引

`awesomeskill.ai` 的 `hardware` 标签下当前收录 8 个 skill，可作为生态观测窗口：

| 族 | 技能 | 用途 |
|---|---|---|
| 通用硬件 | `datasheet-reader`、`zener-language` | 数据手册读取、Zener DSL 处理 |
| M5Stack / ESP32 | `m5-onboard`、`cardputer-buddy` | 设备检测、刷 UIFlow 2.0、MicroPython 应用迭代（热度最高，33,730） |
| NVIDIA HSB | `hsb-setup`、`hsb-flash`、`hsb-app`、`hsb-test` | Holoscan Sensor Bridge：环境搭建、FPGA 刷写、示例运行、QA 测试 |

> **观察**：硬件类 skill 目前**以"设备操作 / 板卡配置 / QA 执行"为主**，纯 PCB 设计的只占少数（`eda-pcb`、`pcb-skill`）。说明 AI 在硬件侧的落地重心仍是**流程自动化**，而非取代工程判断。

### 6.3 安装方式（通用形态）

```bash
# Skill 类：拷贝到客户端 skill 目录
git clone https://github.com/daishuge/pcb-skill.git
cp -r pcb-skill/skills/pcb ~/.claude/skills/pcb     # Claude Code
# 或 ~/.codex/skills/pcb                            # Codex

# MCP 类：注册到客户端
claude mcp add --transport stdio kicad uvx kicad-mcp-server
```

### 6.4 已知限制（务必先读）

| 限制 | 后果 | 应对 |
|---|---|---|
| KiCad **IPC-API 仍是实验性** | 官方标"开发中"，行为可能变 | 别把关键流程绑死在 IPC 上 |
| IPC-API **需 GUI 常驻** | 无头服务器 / CI 用不了实时模式 | 走代码化 EDA 或纯文件分析类 skill |
| **KiCad 9/10 的 IPC 只实现在 PCB 编辑器** | **原理图编辑器不支持**，且不能出图 / 导出（要靠插件调 `kicad-cli` 补），**要到 KiCad 11 才进 IPC** | 升到 KiCad 10 并未解除此限制；原理图侧走文件分析或代码化路线 |
| `kicad-happy` 与 `eda-pcb` 的 MCP 需**自建** | `claude-eda` CLI 未发布到 npm，需从源码建 venv + 装 `kicad-sch-api` + build MCP server | 安装门槛不低，预留配置时间 |
| Skill 信息多为**单源**（项目自述） | 生产成熟度未经独立验证 | 先小项目试用，别直接上正式项目 |

---

## 7. 当前环境的实际状态

| 项 | 状态 |
|---|---|
| 本工作区已安装的 PCB / EDA 相关 Skill | **无** |
| 本工作区现有 docs | 4 篇（硬件文档核查、立创EDA 验证流程、嵌入式最佳实践、Win11 落地方案），均为方法论，非工具 |
| 要启用上述能力需做什么 | ① 装 KiCad 10 ② 从 GitHub 拉 skill 到 `~/.claude/skills/` 或注册 MCP ③ 需要 GUI 实时操作时另装 `Run API Gateway`（立创 EDA 路线） |
| 硬件前置 | 需要一台能跑 EDA + AI 客户端 + 本地服务的机器（**≥ 8GB 内存**，建议 16GB） |

---

## 8. 选型决策表

| 你的场景 | 推荐组合 | 理由 |
|---|---|---|
| 学习 / 玩票 / 快速验证想法 | 立创 EDA Pro + 官方 Skill / 华秋 AI EDA | 免费、国内可用、秒级出骨架 |
| 模板化开发板（如 STM32 系列） | atopile 或 circuit-synth 做原理图 + KiCad 收尾 | 无头、可 git diff、AI 铺草稿效率高 |
| 团队协作 / 需版本控制 & CI | **代码化 EDA**（atopile 优先） | 纯文本、可 review、可 CI 门禁 |
| 设计审查 / 排错 / BOM 核对 | `kicad-happy` 或偏分析类 KiCad MCP | 不改设计，只出结论，风险低 |
| 已有原理图，缺 PCB 布局 | `eda-pcb` skill | 规则驱动的放置 / 走线流水线 |
| 需要自动布线 | Quilter / DeepPCB，或 KiCad + Freerouting | 但高速 / 电源关键走线仍需人工 |
| 下单前把关 | 嘉立创 DFM / 华秋 DFM | 免费在线，30+ 项检测 |
| 高速 / 高功率 / 安规 | 仅作辅助：Ansys / Xpeedic / Sigrity + 人工 | 关键决策必须人来 |
| **直接出量产板不复核** | **不要这么干** | AI 输出的板子必须完整人工 review 后才能投板 |

---

## 9. 能力边界（最重要的一节）

### 9.1 AI 擅长的（仍需复核）

- ✅ 标准模块的连接关系：MCU + USB-C + LDO + 状态灯 + 按键 这类"教科书拓扑"
- ✅ 去耦电容、上下拉、CC 电阻等常规配套件补齐
- ✅ 按部件号选型、生成 BOM
- ✅ 重复性、模板化设计（模块复用加成很大）
- ✅ 设计审查的"第一遍筛子"：找悬空 net、明显 DRC 违规

### 9.2 AI 不擅长的（必须人盯）

- ❌ 关键元件值的合理性（时间常数、分压比、限流）
- ❌ 高速 / 差分 / 电源完整性的布局布线
- ❌ 散热、EMC、可制造性这些"物理世界"约束
- ❌ 偏门元件的引脚映射（容易张冠李戴）
- ❌ **"能 build / 能跑通" ≠ "电气正确"**

### 9.3 真实翻车记录（社区公开案例，原型性很强）

有人用 AI + atopile 全程序"只说 yes、不看代码"做 ESP32-S3 开发板：

1. 第一次 build 成功、元件都找到，但**打开 KiCad 发现一根线都没连**——提醒后才补上
2. 连上后发现 **EN 复位电路漏了一颗电容**，再提醒才补
3. AI 给 EN 电阻选了 **330Ω**，作者手动换成更合理的 **10K**
4. 覆铜、布局整理都是人工完成

> **结论**：AI 是"热情但健忘的实习生"，不是"持证工程师"。**AI 负责把 80% 体力活快速铺好，工程师守住那致命的 20%。**

### 9.4 量化旁证

2026 年 arXiv 论文 **PCBSchemaGen** 评测「自然语言 → 原理图」（LLM 出 SKiDL 代码再生成 KiCad 原理图）：23 个电路任务、9 个 LLM，最好的模型（Gemini 3 Flash）按难度分别拿到约 **93% / 93% / 78%** 的 Pass@1，并声称相对人工约 **37×** 提速。

> ⚠️ 该结论为**单篇、学术、未经独立复现**；且它测的是"**连接对不对**"，而非布局 / 电气工程判断。与定性结论一致：**简单 / 中等拓扑命中率高，难题明显掉档。**

---

## 10. 针对本项目的推荐组合（STM32）

```
① 原理图 / 连接层
   atopile 或 circuit-synth（无头、git 友好、可上 CI）
   └─ 铁律：把 GPIO 分配、USB D+/D- 差分对 net 名钉死；强制用 LCSC 料号（C 号）

② 布局布线
   KiCad 10 + eda-pcb skill（规则驱动放置/走线 + 架构风险预警）
   └─ 或 KiCad + Freerouting，但差分/电源走线人工收尾

③ 校验把关
   kicad-happy（ERC / DRC / EMC 预合规）→ 嘉立创 DFM 二次确认
   └─ 与 docs/硬件文档一致性核查方案.md 的引脚表逐条对照

④ 选型 / BOM
   锁定 LCSC C 号 / MPN，不让 AI 凭记忆编型号
   └─ 逐个核对 AI 是否偷换器件

⑤ 仿真
   Spicebridge（ngspice）做电路级验证
   └─ SI/PI 关键路径再上 Xpeedic / Ansys

⑥ 纪律文件
   在项目记忆（AGENTS.md / CLAUDE.md）或自定义 skill 里写死：
   - 每次改完必须 build / DRC，失败先修再继续
   - 元件一次加一个，不批量塞
   - 严禁擅自修改已导入元件的引脚名 / 引脚号
   - 阻容默认 0603；关键值必须显式指定，不许 AI 猜
```

> 与 `docs/AI辅助立创EDA电路设计与验证流程.md` §4 的衔接：这里产出的引脚表、电源树、BOM，正是那篇文档"回填 `AGENTS.md` §1"的输入。

---

## 11. 风险与回滚

| 风险 | 影响 | 缓解 |
|---|---|---|
| AI 生成的电路直接投板 | 报废 PCB + 元件 + 打样周期 | 四层验证 + 人工必查清单，禁止跳过 |
| AI 静默替换元件型号 | BOM 成本 / 性能偏离设计意图 | 逐个核对 BOM；要求输出"与需求的差异清单" |
| AI 给"能 build 但不合理"的参数值 | 隐蔽、上电才暴露 | 上拉/下拉/限流/时间常数自己核算或显式指定 |
| 引脚映射张冠李戴 | **直接烧板** | 每步查数据手册并说明引脚来源；禁止 AI 改引脚 |
| 设计文件被 AI 批量改动 | 无法定位是哪一步改坏的 | Git 版本控制 + 小步提交 + 改完重跑 DRC/DFM |
| 过度信任 DRC / DFM 全绿 | 忽略电气逻辑与装配问题 | DRC 只查几何规则，DFM 只查可制造性，电气与装配仍需人工 |
| 把厂商 / 专有设计喂给云端模型 | IP 泄露 | 敏感设计用本地模型 |

**回滚**：AI 对 EDA 的所有操作都是**设计文件内的改动**，用 Git（或 EDA 自带历史版本）回退即可；**不要**用"再让 AI 改回去"的方式恢复。

---

## 12. 参考来源

**代码化 EDA 与实战评估**
- Tommy《用 Claude Code 生成 PCB 电路图：技巧、工具与实战评估》（`tommickey.cn`，2026-06-09）—— 本文 §4 / §9.3 / §10 的主要来源
- Anthropic 官方博客《Making Claude a better electrical engineer》（2026-02-05，与 Diode 联名）
- arXiv **PCBSchemaGen**（2026，自然语言 → 原理图评测，单源）

**Skill / MCP**
- `github.com/mixelpixx/KiCAD-MCP-Server`、`github.com/lamaalrajih/kicad-mcp-server`、PyPI `kicad-mcp-server`（bunnyf）
- `kicad-happy`（MIT，12 个 skill）、`l3wi/claude-eda` 的 `eda-pcb`
- `github.com/daishuge/pcb-skill`（ClaudeMap 收录，2026-09-10）
- `github.com/yangl0610/jlceda-agent`、立创EDA 扩展广场 `JLCEDA AI Agent`
- `awesomeskill.ai/tag/hardware`（硬件类 skill 索引）
- LobeHub Skills Marketplace、ClaudeMap、AgentHub

**平台与厂商**
- 华秋 AI EDA：`hq.eda.cn`（2026-03 发布国内首款 AI EDA Agent）
- 嘉立创 EDA：`lceda.cn` / `prodocs.lceda.cn`；嘉立创 DFM：`jlc-dfm.com`
- facetok EDA：`facetok.cn`
- ProtoFlow、Quilter（`quilter.ai`）、Flux.ai、pcbdesigner.ai、JITX、Celus、AllSpice、Blueprint.am、Schematik

**生态综述（含对比表）**
- AtlasPCB《AI PCB Design Tools in 2026》《AI-Powered EDA Tools in 2026》
- makerpcb《AI PCB Design Tools 2026: Auto-Routing, EDA Suites & DFM》
- morepcb《Top AI PCB Design Software in 2026》
- 51CTO《2026 年主流 AI PCB 设计工具全景分析》（页面有反爬，仅参考索引）

> **本工作区内相关文档**：`AI辅助立创EDA电路设计与验证流程.md`（落地实例 + 四层验证）、`硬件文档一致性核查方案.md`（阶段 0 门槛）、`AI用于嵌入式的最佳实践.md`、`Win11-AI嵌入式工程落地方案.md`。
>
> 本文档写于 **2026-09-15**。工具清单、版本号与限制条件请在使用前以官方渠道复核。
