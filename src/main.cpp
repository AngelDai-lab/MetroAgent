#ifndef _HAS_STD_BYTE
#define _HAS_STD_BYTE 0
#endif
#include "station.h"
#include "graph.h"
#include "menu.h"
#include "pathfinder.h" 
#include "cli_main.h"
#include <iostream>
#include <windows.h>
using namespace std;

// 声明 pathfinder 的全局变量（定义在 pathfinder.cpp 中）
extern vector<vector<Edge>> adj;
extern unordered_map<int, Station> stationMap;

int main(int argc, char** argv) {
    // CLI 模式：供 Python Agent 通过 subprocess 调用，输出 JSON
    // 用法：MetroSystem.exe --cli <command> [args...]
    if (argc >= 2 && string(argv[1]) == "--cli") {
        return runCli(argc, argv);
    }

    // 菜单模式：源码与数据均为 UTF-8，将控制台代码页切到 UTF-8 以正确显示/输入中文
    UINT oldOutCP = GetConsoleOutputCP();
    UINT oldInCP = GetConsoleCP();
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    string filename = "data/Station.csv";
    StationManager sm;
    if (!sm.loadStationsFromCSV(filename)) {
        cout << "站点数据加载失败！" << endl;
        return 1;
    }
    cout << "站点数据加载成功！" << endl;

    // 加载边数据,构建地铁图
    Graph graph(&sm);
    string edgeFile = "data/Edge.csv";
    if (!graph.loadEdgesFromCSV(edgeFile)) {
        cout << "边数据加载失败！" << endl;
        return 1;
    }
    cout << "边数据加载成功！" << endl;

    // 构建 pathfinder 模块所需的全局数据结构
    // adj：邻接表，供所有路径规划算法使用
    // stationMap：站点ID → 站点信息，供算法查询站点状态
    graph.buildPathfinderAdj(adj, stationMap);
    for (auto& s : sm.getAllStations()) {
        stationMap[s.station_id] = s;
    }

    // 进入菜单交互
    showMainMenu(&graph, &sm);

    // 恢复控制台原代码页
    SetConsoleOutputCP(oldOutCP);
    SetConsoleCP(oldInCP);
    return 0;
}
