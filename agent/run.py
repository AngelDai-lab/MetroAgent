"""
上海地铁智能出行助手 - 交互式入口
用法：
    python -m agent.run
或：
    python agent/run.py
"""
import sys
import os

# 确保能 import agent 包
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from agent.agent_core import MetroAgent
from agent.config import LLM_API_KEY, LLM_MODEL


def main():
    if not LLM_API_KEY:
        print("⚠️  未配置 LLM_API_KEY。")
        print("   请在 agent/config.py 中填入 API Key，或设置环境变量 LLM_API_KEY。")
        print("   智谱 GLM 免费申请：https://open.bigmodel.cn/")
        sys.exit(1)

    agent = MetroAgent()
    print("=" * 50)
    print("  🚇 上海地铁智能出行助手")
    print(f"  模型：{LLM_MODEL}")
    print("  输入问题开始对话，输入 quit/exit 退出")
    print("=" * 50)

    while True:
        try:
            user_input = input("\n你：").strip()
        except (EOFError, KeyboardInterrupt):
            print("\n再见！")
            break

        if not user_input:
            continue
        if user_input.lower() in ("quit", "exit", "q"):
            print("再见！")
            break

        print("🤖 助手：", end="", flush=True)
        try:
            reply = agent.chat(user_input)
            print(reply)
        except Exception as e:
            print(f"出错了：{e}")


if __name__ == "__main__":
    main()
