"""
Function Calling Tools 定义
- TOOLS: 传给 LLM 的 tools schema（OpenAI 格式）
- tool_handlers: 工具名 -> 处理函数的映射
每个处理函数接收参数字典，返回字符串（作为 tool call result 回填给 LLM）
"""
import json
from typing import Callable, Dict
from .metro_bridge import bridge


def _fmt_path(path: dict) -> str:
    """把单条路径 JSON 格式化为可读文本"""
    segs = path.get("segments", [])
    lines = []
    for seg in segs:
        line_name = seg.get("line", "")
        stations = " -> ".join(seg.get("stations", []))
        lines.append(f"  [{line_name}] {stations}")
    return (
        f"耗时 {path.get('total_time', '?')} 分钟，"
        f"换乘 {path.get('transfer_count', '?')} 次，"
        f"途经 {path.get('station_count', '?')} 站\n" + "\n".join(lines)
    )


# ====== 各工具处理函数 ======

def handle_search_station(args: dict) -> str:
    kw = args.get("keyword", "")
    if not kw:
        return "错误：缺少 keyword 参数"
    data = bridge.search_station(kw)
    if not data.get("success"):
        return f"搜索失败：{data.get('error', '未知错误')}"
    stations = data.get("stations", [])
    if not stations:
        return f"未找到包含「{kw}」的站点"
    lines = [f"找到 {len(stations)} 个匹配站点："]
    for s in stations:
        status = "运营中" if s.get("status") == "open" else "已关闭"
        lines.append(f"  - {s['name']}（{s['line']}）[{status}]")
    return "\n".join(lines)


def handle_shortest_time(args: dict) -> str:
    start, end = args.get("start", ""), args.get("end", "")
    if not start or not end:
        return "错误：需要 start 和 end 参数"
    data = bridge.shortest_time(start, end)
    if not data.get("success"):
        return f"查询失败：{data.get('error', '未知错误')}"
    path = data["path"]
    return f"从 {data['from']} 到 {data['to']} 的最短时间路径：\n{_fmt_path(path)}"


def handle_min_transfer(args: dict) -> str:
    start, end = args.get("start", ""), args.get("end", "")
    if not start or not end:
        return "错误：需要 start 和 end 参数"
    data = bridge.min_transfer(start, end)
    if not data.get("success"):
        return f"查询失败：{data.get('error', '未知错误')}"
    path = data["path"]
    return f"从 {data['from']} 到 {data['to']} 的最少换乘路径：\n{_fmt_path(path)}"


def handle_k_shortest_time(args: dict) -> str:
    start, end = args.get("start", ""), args.get("end", "")
    k = args.get("k", 3)
    if not start or not end:
        return "错误：需要 start 和 end 参数"
    data = bridge.k_shortest_time(start, end, k)
    if not data.get("success"):
        return f"查询失败：{data.get('error', '未知错误')}"
    paths = data.get("paths", [])
    lines = [f"从 {data['from']} 到 {data['to']} 的前 {len(paths)} 条最短时间路径："]
    for i, p in enumerate(paths, 1):
        lines.append(f"\n方案 {i}：\n{_fmt_path(p)}")
    return "\n".join(lines)


def handle_k_min_transfer(args: dict) -> str:
    start, end = args.get("start", ""), args.get("end", "")
    k = args.get("k", 3)
    if not start or not end:
        return "错误：需要 start 和 end 参数"
    data = bridge.k_min_transfer(start, end, k)
    if not data.get("success"):
        return f"查询失败：{data.get('error', '未知错误')}"
    paths = data.get("paths", [])
    lines = [f"从 {data['from']} 到 {data['to']} 的前 {len(paths)} 条最少换乘路径："]
    for i, p in enumerate(paths, 1):
        lines.append(f"\n方案 {i}：\n{_fmt_path(p)}")
    return "\n".join(lines)


def handle_station_status(args: dict) -> str:
    kw = args.get("keyword", "")
    if not kw:
        return "错误：缺少 keyword 参数"
    data = bridge.station_status(kw)
    if not data.get("success"):
        return f"查询失败：{data.get('error', '未知错误')}"
    stations = data.get("stations", [])
    if not stations:
        return f"未找到包含「{kw}」的站点"
    lines = [f"{kw} 相关站点状态："]
    for s in stations:
        status = "运营中" if s.get("status") == "open" else "已关闭"
        lines.append(f"  - {s['name']}（{s['line']}）：{status}")
    return "\n".join(lines)


def handle_set_station_status(args: dict) -> str:
    name = args.get("station_name", "")
    status = args.get("status", "")
    if not name or status not in ("open", "close"):
        return "错误：需要 station_name 和 status（open/close）参数"
    data = bridge.set_station_status(name, status)
    if not data.get("success"):
        return f"操作失败：{data.get('error', '未知错误')}"
    action = "解封（恢复运营）" if status == "open" else "封站（停止运营）"
    return f"已对站点「{data.get('station', name)}」执行{action}操作。后续路径规划会自动规避该站。"


def handle_closed_stations(args: dict) -> str:
    data = bridge.closed_stations()
    if not data.get("success"):
        return f"查询失败：{data.get('error', '未知错误')}"
    stations = data.get("stations", [])
    if not stations:
        return "当前所有站点均正常运营，无关闭站点。"
    lines = [f"当前共 {len(stations)} 个站点关闭："]
    for s in stations:
        lines.append(f"  - {s['name']}（{s['line']}）")
    return "\n".join(lines)


def handle_reset_status(args: dict) -> str:
    data = bridge.reset_status()
    if not data.get("success"):
        return f"操作失败：{data.get('error', '未知错误')}"
    return "已恢复所有站点到初始运营状态。"


def handle_line_info(args: dict) -> str:
    line = args.get("line_name", "")
    if not line:
        return "错误：缺少 line_name 参数"
    data = bridge.line_info(line)
    if not data.get("success"):
        return f"查询失败：{data.get('error', '未知错误')}"
    stations = data.get("stations", [])
    lines = [f"{line} 共 {len(stations)} 个站点："]
    for s in stations:
        tags = []
        if s.get("is_transfer"):
            tags.append("换乘站")
        if s.get("status") == "closed":
            tags.append("已关闭")
        tag_str = f" [{', '.join(tags)}]" if tags else ""
        lines.append(f"  {s['id']}. {s['name']}{tag_str}")
    return "\n".join(lines)


def handle_network_stats(args: dict) -> str:
    data = bridge.network_stats()
    if not data.get("success"):
        return f"查询失败：{data.get('error', '未知错误')}"
    return (
        f"上海地铁网络统计：\n"
        f"  总站点数：{data['total_stations']}（运营中 {data['open_stations']}，关闭 {data['closed_stations']}）\n"
        f"  换乘站：{data['transfer_stations']} 个\n"
        f"  运营线路：{data['line_count']} 条\n"
        f"  轨道边数：{data['edge_count']}"
    )


# ====== Tools Schema（OpenAI Function Calling 格式）======
TOOLS = [
    {
        "type": "function",
        "function": {
            "name": "search_station",
            "description": "模糊搜索地铁站点，返回匹配的站点列表（站名、线路、运营状态）。当用户提到的站名不确定或需要确认时使用。",
            "parameters": {
                "type": "object",
                "properties": {
                    "keyword": {"type": "string", "description": "站点名称关键词，如「人民广场」「虹桥」"},
                },
                "required": ["keyword"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "shortest_time",
            "description": "查询从起点到终点的最短时间路径（基于 Dijkstra 算法，含换乘时间）。",
            "parameters": {
                "type": "object",
                "properties": {
                    "start": {"type": "string", "description": "起点站名"},
                    "end": {"type": "string", "description": "终点站名"},
                },
                "required": ["start", "end"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "min_transfer",
            "description": "查询从起点到终点的最少换乘路径（基于 BFS 算法）。",
            "parameters": {
                "type": "object",
                "properties": {
                    "start": {"type": "string", "description": "起点站名"},
                    "end": {"type": "string", "description": "终点站名"},
                },
                "required": ["start", "end"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "k_shortest_time",
            "description": "查询从起点到终点的前 K 条最短时间路径（基于 Yen 算法），用于提供多个出行方案。",
            "parameters": {
                "type": "object",
                "properties": {
                    "start": {"type": "string", "description": "起点站名"},
                    "end": {"type": "string", "description": "终点站名"},
                    "k": {"type": "integer", "description": "路径条数，默认 3", "default": 3},
                },
                "required": ["start", "end"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "k_min_transfer",
            "description": "查询从起点到终点的前 K 条最少换乘路径，用于提供多个出行方案。",
            "parameters": {
                "type": "object",
                "properties": {
                    "start": {"type": "string", "description": "起点站名"},
                    "end": {"type": "string", "description": "终点站名"},
                    "k": {"type": "integer", "description": "路径条数，默认 3", "default": 3},
                },
                "required": ["start", "end"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "station_status",
            "description": "查询指定站点的运营状态（开启/关闭）。",
            "parameters": {
                "type": "object",
                "properties": {
                    "keyword": {"type": "string", "description": "站点名称关键词"},
                },
                "required": ["keyword"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "set_station_status",
            "description": "封站或解封某个站点。封站后路径规划会自动规避该站。注意：此操作会修改数据文件。",
            "parameters": {
                "type": "object",
                "properties": {
                    "station_name": {"type": "string", "description": "站点名称（需精确匹配）"},
                    "status": {"type": "string", "enum": ["open", "close"], "description": "open=解封，close=封站"},
                },
                "required": ["station_name", "status"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "closed_stations",
            "description": "列出当前所有关闭（封站）的站点。",
            "parameters": {"type": "object", "properties": {}},
        },
    },
    {
        "type": "function",
        "function": {
            "name": "reset_status",
            "description": "恢复所有站点到初始运营状态（撤销所有封站操作）。",
            "parameters": {"type": "object", "properties": {}},
        },
    },
    {
        "type": "function",
        "function": {
            "name": "line_info",
            "description": "查询指定线路的所有站点列表（含换乘站、关闭状态标记）。",
            "parameters": {
                "type": "object",
                "properties": {
                    "line_name": {"type": "string", "description": "线路名称，如「1号线」「2号线」「浦江线」"},
                },
                "required": ["line_name"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "network_stats",
            "description": "获取上海地铁网络整体统计信息（站点数、线路数、换乘站数等）。",
            "parameters": {"type": "object", "properties": {}},
        },
    },
]


# 工具名 -> 处理函数映射
tool_handlers: Dict[str, Callable[[dict], str]] = {
    "search_station": handle_search_station,
    "shortest_time": handle_shortest_time,
    "min_transfer": handle_min_transfer,
    "k_shortest_time": handle_k_shortest_time,
    "k_min_transfer": handle_k_min_transfer,
    "station_status": handle_station_status,
    "set_station_status": handle_set_station_status,
    "closed_stations": handle_closed_stations,
    "reset_status": handle_reset_status,
    "line_info": handle_line_info,
    "network_stats": handle_network_stats,
}
