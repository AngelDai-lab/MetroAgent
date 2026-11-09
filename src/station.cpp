#include "station.h"
#include <fstream>
#include <sstream>
#include <iostream>
using namespace std;

bool StationManager::loadStationsFromCSV(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "无法打开" << filename << endl;
        return false;
    }

    stations.clear();
    id_to_index.clear();
    name_to_indexs.clear();

    string s;
    getline(file, s);  // 跳过表头

    while (getline(file, s)) {
        stringstream ss(s);
        string id_str, name, line, st_str;
        getline(ss, id_str, ',');
        getline(ss, name, ',');
        getline(ss, line, ',');
        getline(ss, st_str);

        int id = stoi(id_str);
        int st = (st_str == "开启") ? 1 : 0;

        stations.emplace_back(id, name, line, st);
        int index = stations.size() - 1;
        id_to_index[id] = index;
        name_to_indexs[name].push_back(index);
    }

    file.close();
    return true;
}

bool StationManager::checkStation(int id) const {
    return id_to_index.find(id) != id_to_index.end();
}

string StationManager::getStationName(int id) const {
    auto it = id_to_index.find(id);
    if (it != id_to_index.end()) {
        return stations[it->second].station_name;
    }
    return "未知站点";
}

// 判断某个站名是否为换乘站
bool StationManager::isTransferStation(const string& stationName) const {
    auto it = name_to_indexs.find(stationName);
    if (it == name_to_indexs.end()) {
        return false;
    }
    return it->second.size() > 1;
}