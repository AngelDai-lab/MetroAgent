#ifndef GRAPH_H
#define GRAPH_H

#include "station.h"
#include <vector>
#include <unordered_map>
#include <string>
using namespace std;

// 前置声明：Edge 结构体定义在 pathfinder.h 中
struct Edge;

/* 邻接表节点结构体（表示从当前站点到相邻站点的一条边）*/
struct AdjNode {
    int to_station_id;   // 终点站点ID
    string line_name;    // 所属线路名称（用于判断换乘）
    int direction;       // 线路方向（4号线：1=内圈，2=外圈；其他线路为0）
    int travel_time;     // 通行耗时（分钟）

    AdjNode(int to = 0, string line = "", int dir = 0, int time = 0)
        : to_station_id(to), line_name(line), direction(dir), travel_time(time) {
    }
};

/* 地铁线路图类（使用邻接表存储整个地铁网络） */
class Graph {
private:
    unordered_map<int, vector<AdjNode>> adj_list;  // 邻接表：站点ID → 相邻站点列表
    StationManager* station_mgr;                   // 站点管理器指针

public:
    Graph(StationManager* mgr) : station_mgr(mgr) {}

    /* 从CSV文件加载边数据，构建邻接表 */
    bool loadEdgesFromCSV(const string& filename);

    /* 获取邻接表的引用（供外部遍历）*/
    unordered_map<int, vector<AdjNode>>& getAdjList() {
        return adj_list;
    }

    /* 将当前邻接表转换为 pathfinder 模块需要的格式
     * @param pathAdj  输出：vector<vector<Edge>> 格式的邻接表
     * @param stationMap 输出：站点ID → Station 的映射
     */
    void buildPathfinderAdj(vector<vector<Edge>>& pathAdj,
        unordered_map<int, Station>& stationMap);
};

#endif
