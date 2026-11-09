"""
Agent 核心：LLM 调用 + Function Calling 循环 + 对话上下文管理
"""
import json
from typing import List, Dict, Optional
from openai import OpenAI
from .config import LLM_BASE_URL, LLM_API_KEY, LLM_MODEL, SYSTEM_PROMPT, MAX_TOOL_CALLS_PER_TURN
from .tools import TOOLS, tool_handlers


class MetroAgent:
    """上海地铁智能出行助手 Agent"""

    def __init__(self, api_key: Optional[str] = None, base_url: str = LLM_BASE_URL, model: str = LLM_MODEL):
        self.model = model
        self.client = OpenAI(
            api_key=api_key or LLM_API_KEY,
            base_url=base_url,
        )
        # 对话上下文：维护 OpenAI messages 格式
        self.messages: List[Dict] = [
            {"role": "system", "content": SYSTEM_PROMPT}
        ]

    def reset(self):
        """重置对话上下文"""
        self.messages = [{"role": "system", "content": SYSTEM_PROMPT}]

    def _execute_tool_call(self, tool_call) -> Dict:
        """执行单次工具调用，返回 tool message"""
        func = tool_call.function
        name = func.name
        try:
            args = json.loads(func.arguments) if func.arguments else {}
        except json.JSONDecodeError:
            args = {}

        handler = tool_handlers.get(name)
        if handler is None:
            content = f"未知工具: {name}"
        else:
            try:
                content = handler(args)
            except Exception as e:
                content = f"工具执行异常: {e}"

        return {
            "role": "tool",
            "tool_call_id": tool_call.id,
            "content": content,
        }

    def chat(self, user_input: str) -> str:
        """
        处理一轮用户输入，支持多轮工具调用。
        返回最终的自然语言回复。
        """
        self.messages.append({"role": "user", "content": user_input})

        for _ in range(MAX_TOOL_CALLS_PER_TURN):
            response = self.client.chat.completions.create(
                model=self.model,
                messages=self.messages,
                tools=TOOLS,
                tool_choice="auto",
                temperature=0.3,
            )
            msg = response.choices[0].message

            # 如果没有工具调用，直接返回文本回复
            if not msg.tool_calls:
                self.messages.append({"role": "assistant", "content": msg.content})
                return msg.content or ""

            # 把 assistant 的 tool_calls 消息加入上下文
            self.messages.append(msg)

            # 执行所有工具调用
            tool_results = [self._execute_tool_call(tc) for tc in msg.tool_calls]
            self.messages.extend(tool_results)

        # 超过最大调用次数，强制生成回复
        response = self.client.chat.completions.create(
            model=self.model,
            messages=self.messages,
            temperature=0.3,
        )
        final = response.choices[0].message.content or ""
        self.messages.append({"role": "assistant", "content": final})
        return final
