#include "graph.h"
#include "pathfinder.h"  
#include <fstream>
#include <sstream>
#include <iostream>
using namespace std;

bool Graph::loadEdgesFromCSV(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "无法打开" << filename << endl;
        return false;
    }

    string s;
    getline(file, s); // 跳过表头

    while (getline(file, s)) {
        stringstream ss(s);
        string start_id_str, end_id_str, line, dir_str, time_str;

        getline(ss, start_id_str, ',');
        getline(ss, end_id_str, ',');
        getline(ss, line, ',');
        getline(ss, dir_str, ',');
        getline(ss, time_str);

        int start_id = stoi(start_id_str);
        int end_id = stoi(end_id_str);

        // 验证起点和终点是否存在
        if (!station_mgr->checkStation(start_id)) {
            cerr << "警告：起点车站ID " << start_id << "不存在，跳过该边" << endl;
            continue;
        }
        if (!station_mgr->checkStation(end_id)) {
            cerr << "警告：终点车站ID " << end_id << "不存在，跳过该边" << endl;
            continue;
        }

        // 解析方向：4号线特殊处理
        int dir = 0;
        if (dir_str == "内圈") dir = 1;
        else if (dir_str == "外圈") dir = 2;

        // 添加到邻接表
        adj_list[start_id].emplace_back(end_id, line, dir, stoi(time_str));
    }

    file.close();
    return true;
}

void Graph::buildPathfinderAdj(vector<vector<Edge>>& pathAdj,
    unordered_map<int, Station>& stationMap) {
    // 第一步：找出所有站点ID中的最大值，用于确定vector大小
    int maxId = 0;
    for (auto& pair : adj_list) {
        if (pair.first > maxId) maxId = pair.first;
        for (auto& node : pair.second) {
            if (node.to_station_id > maxId) maxId = node.to_station_id;
        }
    }
    pathAdj.resize(maxId + 1);

    // 第二步：将 adj_list 转换为 vector<vector<Edge>> 格式
    for (auto& pair : adj_list) {
        int u = pair.first;
        for (auto& node : pair.second) {
            int v = node.to_station_id;
            string line = node.line_name;
            int cost = node.travel_time;
            int dir = node.direction;  // 获取方向
            pathAdj[u].emplace_back(v, line, cost, node.direction);
        }
    }

    // 注意：stationMap 由 main.cpp 调用 sm.getAllStations() 填充
}