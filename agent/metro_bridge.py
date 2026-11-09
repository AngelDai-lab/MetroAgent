"""
C++ 计算引擎桥接层
通过 subprocess 调用本地 MetroSystem.exe，解析 JSON 输出。
所有 C++ 命令统一封装为 Python 函数，供 tools.py 调用。
"""
import json
import subprocess
from typing import Optional
from .config import METRO_EXE, PROJECT_ROOT


class MetroBridge:
    """C++ 地铁计算引擎的 Python 桥接"""

    def __init__(self, exe_path: str = METRO_EXE, cwd: str = PROJECT_ROOT):
        self.exe_path = exe_path
        self.cwd = cwd

    def _run(self, *args: str) -> dict:
        """执行 C++ CLI 命令并返回解析后的 JSON"""
        cmd = [self.exe_path, "--cli", *args]
        try:
            result = subprocess.run(
                cmd,
                cwd=self.cwd,
                capture_output=True,
                text=True,
                encoding="utf-8",
                timeout=30,
            )
            stdout = result.stdout.strip()
            if not stdout:
                return {"success": False, "error": "C++ 程序无输出", "stderr": result.stderr}
            # C++ 输出单行 JSON
            return json.loads(stdout)
        except subprocess.TimeoutExpired:
            return {"success": False, "error": "C++ 计算超时"}
        except json.JSONDecodeError as e:
            return {"success": False, "error": f"JSON 解析失败: {e}", "raw": stdout[:500]}
        except FileNotFoundError:
            return {"success": False, "error": f"找不到可执行文件: {self.exe_path}"}

    # ====== 站点搜索 ======
    def search_station(self, keyword: str) -> dict:
        """模糊搜索站点"""
        return self._run("search", keyword)

    # ====== 路径规划 ======
    def shortest_time(self, start: str, end: str) -> dict:
        """单条最短时间路径（Dijkstra）"""
        return self._run("shortest_time", start, end)

    def min_transfer(self, start: str, end: str) -> dict:
        """单条最少换乘路径（BFS）"""
        return self._run("min_transfer", start, end)

    def k_shortest_time(self, start: str, end: str, k: int = 3) -> dict:
        """K 条最短时间路径（Yen）"""
        return self._run("k_shortest_time", start, end, str(k))

    def k_min_transfer(self, start: str, end: str, k: int = 3) -> dict:
        """K 条最少换乘路径"""
        return self._run("k_min_transfer", start, end, str(k))

    # ====== 运营状态管理 ======
    def station_status(self, keyword: str) -> dict:
        """查询站点运营状态"""
        return self._run("station_status", keyword)

    def set_station_status(self, station_name: str, status: str) -> dict:
        """设置站点状态：status 为 'open' 或 'close'"""
        return self._run("set_status", station_name, status)

    def closed_stations(self) -> dict:
        """列出所有关闭站点"""
        return self._run("closed_stations")

    def reset_status(self) -> dict:
        """恢复所有站点初始状态"""
        return self._run("reset_status")

    # ====== 线路与网络信息 ======
    def line_info(self, line_name: str) -> dict:
        """查询指定线路的站点列表"""
        return self._run("line_info", line_name)

    def network_stats(self) -> dict:
        """地铁网络统计信息"""
        return self._run("network_stats")


# 全局单例
bridge = MetroBridge()
