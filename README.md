# MetroAgent — 基于 LLM Agent 的上海地铁智能出行助手

> 当传统数据结构课程设计遇上大模型：用 **C++17 图算法引擎**做精准计算，用 **LLM Function Calling** 做自然语言调度。
> 用户说"从人民广场到陆家嘴怎么走最快"，Agent 自动调用 Dijkstra 引擎并用人话回答你。

[![C++17](https://img.shields.io/badge/C++-17-blue?logo=c%2B%2B)](https://en.cppreference.com/w/cpp/17)
[![Python](https://img.shields.io/badge/Python-3.10+-3776AB?logo=python&logoColor=white)](https://www.python.org/)
[![LLM](https://img.shields.io/badge/LLM-GLM--4--flash-7B61FF)](https://open.bigmodel.cn/)
[![Algorithm](https://img.shields.io/badge/Algorithms-Dijkstra%20%7C%20BFS%20%7C%20Yen-orange)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-Windows-0078D6?logo=windows&logoColor=white)]()

---

## 🎯 项目背景

上海地铁网络规模庞大：**20 条线路、530 个站点、1300 条区间**，日均客流超千万人次。乘客出行最关心两件事——**怎么最快到达**、**怎么少换乘**；运营方则关心——**某站关闭对全网影响有多大**。

本项目最初是《数据结构》课程设计：一个基于邻接表 + Dijkstra/BFS/Yen 算法的 C++ 控制台系统。在此基础上，我将其升级为 **LLM Agent 智能助手**：

- 用 **Python 调度层**接入大模型（智谱 GLM，OpenAI 兼容）
- 通过 **Function Calling** 将自然语言转化为结构化指令
- 调用 **C++ 编译产物**（CLI/JSON 模式）获取毫秒级精确计算结果
- 由大模型把结果组织成自然语言回复

> 💡 大模型负责"理解意图、组织语言"，C++ 负责"精确计算"——让 LLM 不胡编路径，让算法开口说人话。

---

## 📑 目录

- [🎯 项目背景](#-项目背景)
- [✨ 核心亮点](#-核心亮点)
- [🛠️ 技术栈](#️-技术栈)
- [🏗️ 系统架构](#️-系统架构)
- [🚀 快速开始](#-快速开始)
- [⚙️ 环境变量](#️-环境变量)
- [📁 项目结构](#-项目结构)
- [🧠 算法设计](#-算法设计)
- [🤝 贡献指南](#-贡献指南)
- [📜 License](#-license)

---

## ✨ 核心亮点

### 🤖 LLM Agent 调度层（Function Calling）

- 定义 **11 个 Tools**（站点搜索、时间/换乘最优、K 条备选、封站管理、网络分析等）
- 维护多轮对话上下文，支持 **多步工具调用循环**（最多 8 轮防死循环）
- 站名模糊时 Agent 会先调 `search_station` 确认，再规划路径

### ⚡ C++17 高性能计算引擎

| 算法 | 场景 | 时间复杂度 |
|------|------|-----------|
| **Dijkstra + 优先队列** | 单条最短时间路径 | O(E log V) |
| **BFS**（状态 = 站点×线路） | 单条最少换乘路径 | O(V + E) |
| **Yen 算法** | K 条备选路径 | O(K·E log V) |

- **邻接表**存储稀疏图：空间 O(V+E)，远优于邻接矩阵 O(V²)
- **虚拟换乘边**建模换乘耗时（5 分钟），同名换乘站自动互通
- 单次路径查询**毫秒级**返回

### 🔗 运营状态与路径规划联动

- 封站后 Dijkstra/BFS 扩展时**自动规避**该站，路径实时更新
- 同名换乘站所有线路**同步开关**（如封"人民广场"则 1/2/8 号线同时生效）
- "先读后写"策略：CSV 一次性读入内存、修改后整体写回，保证数据一致性

### 🌉 跨语言桥接与编码治理

- C++ 侧新增 `--cli` 模式：**stdout 只输出单行 JSON**，零第三方依赖（手写 JSON 序列化）
- Python 侧 `subprocess` 调用 + `json.loads` 解析，跨语言解耦
- 全链路统一 **UTF-8**（源码 / CSV / 控制台代码页 / argv），彻底消灭中文乱码

---

## 🛠️ 技术栈

| 层次 | 技术 | 说明 |
|------|------|------|
| **大模型层** | 智谱 GLM-4-flash（OpenAI 兼容接口） | Function Calling 工具调度 |
| **调度层** | Python 3.10+ · openai SDK | Agent 循环、上下文管理 |
| **计算层** | C++17 · STL | Dijkstra / BFS / Yen / 邻接表 |
| **数据源** | CSV（Station 530 条 + Edge 1300 条） | "先读后写"持久化 |
| **桥接** | subprocess + JSON | stdout 单行 JSON 契约 |

---

## 🏗️ 系统架构

| 层级 | 组成 | 职责 |
|------|------|------|
| 👤 **用户层** | 自然语言输入 | 「从人民广场到陆家嘴怎么走最快？」 |
| 🧠 **LLM Agent 调度层**<br/>（Python） | `agent_core.py` | 维护对话上下文，驱动工具调用循环 |
| | `tools.py` | 定义 11 个 Function Calling Tools |
| | `metro_bridge.py` | subprocess 桥接 C++ 引擎 + JSON 解析 |
| ⚡ **C++ 计算引擎**<br/>（原课程设计，零侵入扩展） | 业务逻辑层 | Dijkstra / BFS / Yen / 运营状态管理 |
| | 数据层 | CSV 解析 → 邻接表图结构 |
| | 交互层 | 原控制台菜单（保留，可独立使用） |

**调用链路：**

```
用户 → LLM Agent 调度层 →（subprocess: MetroSystem.exe --cli <cmd> → stdout 单行 JSON）→ C++ 计算引擎
```

**一次对话的完整生命周期：**

```
用户："从人民广场到陆家嘴怎么走最快？"
  │
  ▼
LLM ──Function Call──▶ shortest_time("人民广场", "陆家嘴")
  │                        │
  │                        ▼
  │              metro_bridge.py ──subprocess──▶ MetroSystem.exe --cli shortest_time ...
  │                        │                        │
  │                        │◀─────── JSON ──────────┘  (Dijkstra 计算，毫秒级)
  │                        ▼
  │              {"total_time":9, "segments":[...2号线...]}
  │◀─── tool result ───────┘
  ▼
LLM 组织自然语言回复：
"推荐您乘坐 2 号线直达：人民广场 → 南京东路 → 陆家嘴，
 全程约 9 分钟，无需换乘。"
```

---

## 🚀 快速开始

### 前置要求

- Windows 10/11
- Visual Studio 2022（MSVC，用于编译 C++）
- Python 3.10+
- 智谱 GLM API Key（[免费申请](https://open.bigmodel.cn/)）

### 1. 克隆与依赖安装

```bash
git clone https://github.com/AngelDai-lab/MetroAgent.git
cd MetroAgent
pip install -r requirements.txt
```

### 2. 编译 C++ 引擎

用 Visual Studio 打开 `MetroSystem.sln`，选择 **Release x64** 编译，生成 `x64/Release/MetroSystem.exe`。

### 3. 配置 API Key

```bash
copy .env.example .env
# 编辑 .env，填入你的 LLM_API_KEY
```

### 4. 运行（两种模式任选）

**🤖 Agent 模式（推荐）—— 自然语言对话：**

```bash
python -m agent.run
```

```
你：人民广场到陆家嘴怎么走最快？
🤖 助手：推荐您乘坐 2 号线直达：人民广场 → 南京东路 → 陆家嘴，
全程约 9 分钟，无需换乘，途经 3 个站点。

你：那如果人民广场封站了呢？
🤖 助手：（自动调用 set_station_status 封站后重新规划…）
```

**🖥️ 传统菜单模式 —— 原课程设计交互：**

```powershell
cd MetroAgent
.\x64\Release\MetroSystem.exe
```

> ⚠️ 注意：程序用相对路径 `data/` 加载数据，**工作目录必须是项目根目录**。

---

## ⚙️ 环境变量

| 变量 | 是否必须 | 默认值 | 说明 |
|------|---------|--------|------|
| `LLM_API_KEY` | ✅ 是（Agent 模式） | 无 | 智谱 GLM API Key |
| `LLM_BASE_URL` | 否 | `https://open.bigmodel.cn/api/paas/v4` | 可切换 OpenAI 官方接口 |
| `LLM_MODEL` | 否 | `glm-4-flash` | 免费模型；`glm-4` 效果更好 |

> 🔐 `.env` 已在 `.gitignore` 中，密钥不会被提交。配置优先级：环境变量 > `.env` 文件。

---

## 📁 项目结构

```
MetroSystem/
├── agent/                      # 🧠 Python Agent 调度层
│   ├── config.py               #   模型配置 / System Prompt / .env 加载
│   ├── metro_bridge.py         #   C++ 桥接层（subprocess + JSON）
│   ├── tools.py                #   11 个 Function Calling Tools
│   ├── agent_core.py           #   Agent 核心：上下文 + 工具调用循环
│   └── run.py                  #   交互式入口
│
├── data/                       # 📊 数据源（CSV，UTF-8）
│   ├── Station.csv             #   530 个站点（ID/站名/线路/运营状态）
│   ├── Station_init.csv        #   初始状态备份（用于一键恢复）
│   ├── Edge.csv                #   1300 条边（含虚拟换乘边）
│   └── update_station_status.csv  # 批量状态更新样例
│
├── src/                        # ⚡ C++ 计算引擎
│   ├── main.cpp                #   🚪 入口：--cli 走 JSON 模式，否则进菜单
│   ├── cli_main.cpp / .h       #   🌉 CLI/JSON 模式（供 Python 调用）
│   ├── station.cpp / .h        #   数据层：Station 结构 + CSV 解析
│   ├── graph.cpp / .h          #   数据层：邻接表图结构
│   ├── pathfinder.cpp / .h     #   业务层：Dijkstra / BFS / Yen
│   ├── manager.cpp             #   业务层：运营状态管理
│   └── menu.cpp / .h           #   交互层：控制台二级菜单
│
├── .env.example                # 🔐 环境变量模板
├── requirements.txt            # Python 依赖
└── MetroSystem.sln / .vcxproj  # VS2022 工程
```

---

## 🧠 算法设计要点

### 为什么时间最优用 Dijkstra，换乘最少却用 BFS？

- 边权（通行时间）非负且不等 → **Dijkstra** 贪心 + 优先队列，O(E log V)
- 换乘次数本质是"层数"——BFS 按层次扩展，第 k 层节点的换乘次数恰好是 k，首次到达终点即最优 → **BFS** 线性 O(V+E)，无需堆

### K 条备选路径：Yen 算法

在最短路径上依次设置**偏离点**，封锁已用偏离边，调用 Dijkstra 生成候选路径，取候选集最优作为下一条。本质是 K 次 Dijkstra → O(K·E log V)，K=3 时完全实时。

### 换乘站建模：虚拟换乘边

同名站点（如人民广场 1/2/8 号线）之间添加耗时 5 分钟的"换乘"边，使得：

- Dijkstra 自动把换乘代价计入总时间
- BFS 用 **(站点ID, 线路)** 组合作为 visited 状态，避免换乘时"原地绕环"

---

## 🤝 贡献指南

欢迎提交 Issue / Pull Request：

1. Fork 本仓库
2. 创建特性分支（`git checkout -b feature/AmazingFeature`）
3. 提交更改（`git commit -m 'Add some AmazingFeature'`）
4. 推送分支（`git push origin feature/AmazingFeature`）
5. 打开 Pull Request

**改进方向**：可视化 GUI 界面（Qt/Web）、实时客流数据接入、首末班车时间查询、支持更多城市地铁网络。

---

## 📜 License

本项目基于 [MIT License](LICENSE) 开源。

> 📝 说明：底层 C++ 路径规划与运营管理系统为《数据结构》课程设计（小组协作完成）；`agent/` 目录下的 LLM Agent 调度层与 C++ 侧的 CLI/JSON 接口改造为个人独立扩展。

---

## 🙏 联系与致谢

- 项目地址：**https://github.com/AngelDai-lab/MetroAgent**
- 作者：[@AngelDai-lab](https://github.com/AngelDai-lab)
- 感谢 [智谱 AI](https://open.bigmodel.cn/) 提供免费的大模型 API 额度
- 感谢所有使用和关注本项目的人。每一个优秀的项目背后，都离不开社区的反馈与支持
**⭐️ 如果这个项目对你有帮助，欢迎点一个 Star 支持一下！**


