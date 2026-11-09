"""
Agent 配置：大模型 API 接入
- 默认使用智谱 GLM（OpenAI 兼容接口，提供免费额度）
- API Key 从环境变量或项目根目录 .env 文件读取，绝不硬编码

配置方式（二选一）：
  1. 环境变量：setx LLM_API_KEY "你的Key"  （PowerShell: $env:LLM_API_KEY="..."）
  2. 复制 .env.example 为 .env，填入 LLM_API_KEY=你的Key
"""
import os


def _load_dotenv():
    """轻量 .env 加载（不依赖 python-dotenv）：读取项目根目录 .env 中的 KEY=VALUE"""
    env_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".env"))
    if not os.path.exists(env_path):
        return
    with open(env_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, _, value = line.partition("=")
            key, value = key.strip(), value.strip()
            if key and key not in os.environ:  # 环境变量优先于 .env
                os.environ[key] = value


_load_dotenv()

# ====== 大模型配置 ======
# 智谱 GLM（推荐，免费额度）：https://open.bigmodel.cn/
LLM_BASE_URL = os.getenv("LLM_BASE_URL", "https://open.bigmodel.cn/api/paas/v4")
LLM_API_KEY = os.getenv("LLM_API_KEY", "")  # 仅从环境变量 / .env 读取
LLM_MODEL = os.getenv("LLM_MODEL", "glm-4-flash")  # glm-4-flash 免费，glm-4 更强

# 如果使用 OpenAI 官方，在 .env 中覆盖：
# LLM_BASE_URL=https://api.openai.com/v1
# LLM_MODEL=gpt-4o-mini

# ====== C++ 可执行文件配置 ======
# 项目根目录（data 目录所在处，C++ 程序用相对路径加载数据）
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
# C++ 可执行文件路径
METRO_EXE = os.path.join(PROJECT_ROOT, "x64", "Release", "MetroSystem.exe")

# ====== Agent 行为配置 ======
MAX_TOOL_CALLS_PER_TURN = 8  # 单轮对话最多工具调用次数，防止死循环
SYSTEM_PROMPT = """你是一个上海地铁智能出行助手，基于真实的上海地铁数据（530个站点、20条线路）提供精准的出行规划服务。

你可以通过工具调用 C++ 计算引擎获取精确结果，包括：
- 模糊搜索站点
- 最短时间路径规划（Dijkstra）
- 最少换乘路径规划（BFS）
- K条备选路径（Yen算法）
- 查询站点运营状态
- 封站/解封操作
- 查询线路站点列表
- 网络统计信息

回答要求：
1. 路径规划结果要清晰展示：总耗时、换乘次数、分段线路与站点序列
2. 如果用户输入的站名模糊或有多个匹配，先调用搜索工具确认，或直接告知匹配结果让用户选择
3. 涉及封站操作时要提醒用户这会影响路径规划
4. 用自然、友好的中文回复，不要直接输出原始 JSON
5. 对于计算结果，务必基于工具返回的数据，不要编造
"""
