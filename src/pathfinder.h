#ifndef PATHFINDER_H
#define PATHFINDER_H

#include "station.h"
#include "graph.h"   
#include <vector>
#include <queue>
#include <unordered_map>
#include <string>
#include <set>
#include <climits>
using namespace std;

// 邻接边结构体，修复字符串引用问题
struct Edge {
    int to;
    string line;
    int cost;
    int direction;

    // 默认构造函数
    Edge() : to(0), line(""), cost(0), direction(0) {}

    // 3参数构造函数（兼容旧代码）
    Edge(int t, const string& l, int c) : to(t), line(l), cost(c), direction(0) {}

    // 4参数构造函数（新增）
    Edge(int t, const string& l, int c, int d) : to(t), line(l), cost(c), direction(d) {}
};

// 路径信息结构体
struct PathInfo
{
    vector<int> stationSeq;    // 路径上的站点ID序列（有序）
    vector<string> lineSeq;    // 路径上的线路序列（stationSeq[i]→stationSeq[i+1]的线路）
    vector<int> directionSeq;  // 每条边的方向（0=无方向，1=内圈，2=外圈）
    int totalTime=0;             // 总耗时（分钟，含换乘时间）
    int transferCnt=0;           // 换乘次数

    // 按总时间升序，时间相同按换乘次数升序
    // 用于 Yen 算法中对候选路径排序
    bool operator<(const PathInfo& other) const
    {
        if (totalTime != other.totalTime)
            return totalTime < other.totalTime;
        return transferCnt < other.transferCnt;
    }

    // 按换乘次数升序，次数相同按时间升序
    // 用于最少换乘路径排序
    bool cmpTransfer(const PathInfo& other) const
    {
        if (transferCnt != other.transferCnt)
            return transferCnt < other.transferCnt;
        return totalTime < other.totalTime;
    }

    // 路径去重判断：站点序列完全相同则视为重复路径
    bool equalPath(const PathInfo& other) const
    {
        return stationSeq == other.stationSeq;
    }
};

// 全局变量声明（仅写在头文件，带extern）
extern vector<vector<Edge>> adj;// 全局邻接表（供所有算法使用）
extern unordered_map<int, Station> stationMap;// 全局站点映射（供算法查询站点状态）

// 函数声明
PathInfo getSingleShortestTime(int startId, int endId);              //获取单条最短时间路径（Dijkstra）
vector<PathInfo> getKShortestTime(int startId, int endId, int k = 3);//获取 K 条最短时间路径(Yen)
PathInfo getSingleMinTransfer(int startId, int endId);               //获取单条最少换乘路径(BFS)
vector<PathInfo> getKMinTransfer(int startId, int endId, int k = 3); //获取 K 条最少换乘路径(先通过 Yen 获取多条时间最优路径，再按换乘次数排序筛选)

bool isStationValid(int sid);//校验站点是否有效且开放
void calcPathTimeTransfer(PathInfo& path);//计算路径的总耗时和换乘次数
PathInfo dijkstraBlockNode(int start, int end, set<int>& blockNodes);

// ============================================================
// 路径输出与去重模块
// ============================================================
/* 打印单条路径的详细信息 */
void printPath(const PathInfo& path, const StationManager& sm);
void printPathWithNumber(const PathInfo& path, const StationManager& sm, int pathNumber);  
/* 打印多条路径（用于 K 条路径查询结果展示）*/
void printKPaths(const vector<PathInfo>& paths, const StationManager& sm);
/* 判断两条路径是否重复（通过站点序列逐位比较）*/
bool isPathDuplicate(const PathInfo& a, const PathInfo& b);
/* 从路径列表中去除重复路径 */
vector<PathInfo> removeDuplicatePaths(const vector<PathInfo>& paths);

// ============================================
// 菜单调用的接口函数（供 menu.cpp 调用）
// ============================================
/* 菜单选项1：单条最短时间路径查询 */
void shortestTimePath(Graph* graph, StationManager* sm);
/* 菜单选项2：单条最少换乘路径查询 */
void minTransferPath(Graph* graph, StationManager* sm);
/* 菜单选项3：K条最短时间路径查询 */
void kShortestTimePaths(Graph* graph, StationManager* sm);
/* 菜单选项4：K条最少换乘路径查询 */
void kMinTransferPaths(Graph* graph, StationManager* sm);
/* 菜单选项6：网络分析（预留）*/
void showNetworkAnalysis(Graph* graph, StationManager* sm);

#endif
