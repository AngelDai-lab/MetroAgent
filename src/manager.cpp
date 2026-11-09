#include "station.h"   
#include "pathfinder.h" 
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
using namespace std;

// 声明 StationManager（因为 manager.cpp 需要用到）
class StationManager;

// ============================================
// 工具函数：读取CSV所有行到内存
// ============================================
static vector<string> readAllLines(const string& filename) {
    vector<string> lines;
    ifstream inFile(filename);
    if (!inFile) {
        return lines;
    }
    string line;
    while (getline(inFile, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    inFile.close();
    return lines;
}

// ============================================
// 工具函数：将所有行写回CSV
// ============================================
static bool writeAllLines(const string& filename, const vector<string>& lines) {
    ofstream outFile(filename);
    if (!outFile) {
        return false;
    }
    for (size_t i = 0; i < lines.size(); i++) {
        outFile << lines[i] << endl;
    }
    outFile.close();
    return true;
}

// ============================================
// 功能1：CSV批量更新站点状态
// ============================================
void updateStationCsvStatus() {
    cout << "\n【CSV批量更新站点状态】" << endl;

    ifstream updateFile("data/update_station_status.csv");
    if (!updateFile) {
        cout << "错误：找不到 data/update_station_status.csv 文件！" << endl;
        return;
    }

    string header;
    getline(updateFile, header);
    if (!header.empty() && header.back() == '\r') header.pop_back();

    vector<pair<string, string>> updates;
    string line;
    while (getline(updateFile, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        stringstream ss(line);
        string stationName, status;
        getline(ss, stationName, ',');
        getline(ss, status, ',');
        updates.push_back(make_pair(stationName, status));
    }
    updateFile.close();

    if (updates.empty()) {
        cout << "更新文件为空，无需更新。" << endl;
        return;
    }

    cout << "读取到 " << updates.size() << " 条更新记录" << endl;

    vector<string> allLines = readAllLines("data/Station.csv");
    if (allLines.empty()) {
        cout << "错误：找不到 data/Station.csv 文件！" << endl;
        return;
    }

    int updateCount = 0;
    for (size_t i = 1; i < allLines.size(); i++) {
        stringstream ss(allLines[i]);
        string id, name, lineName, status;
        getline(ss, id, ',');
        getline(ss, name, ',');
        getline(ss, lineName, ',');
        getline(ss, status, ',');

        for (size_t j = 0; j < updates.size(); j++) {
            if (name == updates[j].first) {
                string oldStatus = status;
                status = updates[j].second;
                allLines[i] = id + "," + name + "," + lineName + "," + status;
                cout << "  更新：" << name << " (" << lineName << ") "
                    << oldStatus << " -> " << status << endl;
                updateCount++;
                break;
            }
        }
    }

    if (writeAllLines("data/Station.csv", allLines)) {
        cout << "批量更新完成！共更新 " << updateCount << " 条记录。" << endl;

        // ========== 同步更新 stationMap ==========
        // 重新加载站点数据到 stationMap
        ifstream syncFile("data/Station.csv");
        if (syncFile.is_open()) {
            string syncLine;
            getline(syncFile, syncLine);  // 跳过表头
            while (getline(syncFile, syncLine)) {
                if (!syncLine.empty() && syncLine.back() == '\r') syncLine.pop_back();
                if (syncLine.empty()) continue;

                stringstream ss(syncLine);
                string idStr, name, lineName, statusStr;
                getline(ss, idStr, ',');
                getline(ss, name, ',');
                getline(ss, lineName, ',');
                getline(ss, statusStr, ',');

                int id = stoi(idStr);
                int status = (statusStr == "开启") ? 1 : 0;

                if (stationMap.find(id) != stationMap.end()) {
                    stationMap[id].status = status;
                }
            }
            syncFile.close();
        }
        // =====================================
    }
    else {
        cout << "错误：写入 data/Station.csv 失败！" << endl;
    }
}

// ============================================
// 功能2：手工更新站点状态
// ============================================
void manualUpdateStation() {
    cout << "\n【手工更新站点状态】" << endl;

    string targetName, newStatus;
    cout << "请输入站点名称（如：徐家汇）：";
    cin >> targetName;

    // 读取所有行
    vector<string> allLines = readAllLines("data/Station.csv");
    if (allLines.empty()) {
        cout << "错误：找不到 data/Station.csv 文件！" << endl;
        return;
    }

    // 第一步：收集匹配的站点（站名相同）
    vector<pair<int, string>> matches;  // (行号, 线路名)
    for (size_t i = 1; i < allLines.size(); i++) {
        stringstream ss(allLines[i]);
        string id, name, lineName, status;
        getline(ss, id, ',');
        getline(ss, name, ',');
        getline(ss, lineName, ',');
        getline(ss, status, ',');

        if (name == targetName) {
            matches.push_back({ (int)i, lineName });
        }
    }

    if (matches.empty()) {
        cout << "未找到站点：" << targetName << "，请检查站名是否正确。" << endl;
        return;
    }

    // 第二步：如果匹配到多个，让用户选择
    int selectedIndex = 0;
    if (matches.size() == 1) {
        cout << "已匹配：" << targetName << "（" << matches[0].second << "）" << endl;
        selectedIndex = 0;
    }
    else {
        cout << "找到多个匹配的站点，请选择：" << endl;
        for (size_t i = 0; i < matches.size(); i++) {
            cout << "  " << i + 1 << ". " << targetName << "（" << matches[i].second << "）" << endl;
        }
        cout << "请输入编号（1-" << matches.size() << "），或输入 0 全部更新：";
        int choice;
        cin >> choice;
        if (choice == 0) {
            selectedIndex = -1;  // -1 表示全部更新
        }
        else if (choice >= 1 && choice <= (int)matches.size()) {
            selectedIndex = choice - 1;
        }
        else {
            cout << "无效编号！" << endl;
            return;
        }
    }

    cout << "请输入新状态（开启/关闭）：";
    cin >> newStatus;

    if (newStatus != "开启" && newStatus != "关闭") {
        cout << "错误：状态只能是'开启'或'关闭'！" << endl;
        return;
    }

    // 第三步：执行更新
    int changeCount = 0;
    for (size_t i = 0; i < matches.size(); i++) {
        // 如果选择了具体某条，只更新那一条；否则全部更新
        if (selectedIndex != -1 && (int)i != selectedIndex) continue;

        int row = matches[i].first;
        stringstream ss(allLines[row]);
        string id, name, lineName, status;
        getline(ss, id, ',');
        getline(ss, name, ',');
        getline(ss, lineName, ',');
        getline(ss, status, ',');

        allLines[row] = id + "," + name + "," + lineName + "," + newStatus;
        cout << "  已更新：" << name << "（" << lineName << "） " << status << " -> " << newStatus << endl;
        changeCount++;
    }

    // 写回文件
    if (writeAllLines("data/Station.csv", allLines)) {
        cout << "手工更新完成！共修改 " << changeCount << " 条记录。" << endl;

        // ============================================================
        // 同步更新 stationMap（让路径规划立即生效）
        // ============================================================
        ifstream syncFile("data/Station.csv");
        if (syncFile.is_open()) {
            string syncLine;
            getline(syncFile, syncLine);  // 跳过表头
            while (getline(syncFile, syncLine)) {
                if (!syncLine.empty() && syncLine.back() == '\r') syncLine.pop_back();
                if (syncLine.empty()) continue;

                stringstream ss(syncLine);
                string idStr, name, lineName, statusStr;
                getline(ss, idStr, ',');
                getline(ss, name, ',');
                getline(ss, lineName, ',');
                getline(ss, statusStr, ',');

                int id = stoi(idStr);
                int status = (statusStr == "开启") ? 1 : 0;

                if (stationMap.find(id) != stationMap.end()) {
                    stationMap[id].status = status;
                }
            }
            syncFile.close();
            cout << "路径规划数据已同步更新。" << endl;
        }
        // ============================================================
    }
    else {
        cout << "错误：写入文件失败！" << endl;
    }
}

// ============================================
// 功能3：显示当前关闭站点
// ============================================
void showClosedStations() {
    cout << "\n【当前关闭站点列表】" << endl;
    cout << "-----------------------------------" << endl;

    vector<string> allLines = readAllLines("data/Station.csv");
    if (allLines.empty()) {
        cout << "错误：找不到 data/Station.csv 文件！" << endl;
        return;
    }

    int count = 0;
    for (size_t i = 1; i < allLines.size(); i++) {
        stringstream ss(allLines[i]);
        string id, name, lineName, status;
        getline(ss, id, ',');
        getline(ss, name, ',');
        getline(ss, lineName, ',');
        getline(ss, status, ',');

        if (status == "关闭") {
            cout << "  " << name << " (" << lineName << ")" << endl;
            count++;
        }
    }

    if (count == 0) {
        cout << "  当前没有关闭的站点（全部为开启状态）" << endl;
    }
    else {
        cout << "-----------------------------------" << endl;
        cout << "共 " << count << " 个站点关闭" << endl;
    }
}

// ============================================
// 功能4：恢复初始状态
// ============================================
void resetStationStatus() {
    cout << "\n【恢复初始状态】" << endl;

    vector<string> allLines = readAllLines("data/Station_init.csv");
    if (allLines.empty()) {
        cout << "错误：找不到 data/Station_init.csv！" << endl;
        return;
    }

    if (writeAllLines("data/Station.csv", allLines)) {
        cout << "已恢复所有站点到初始状态！" << endl;

        // ========== 同步更新 stationMap ==========
        ifstream syncFile("data/Station.csv");
        if (syncFile.is_open()) {
            string syncLine;
            getline(syncFile, syncLine);
            while (getline(syncFile, syncLine)) {
                if (!syncLine.empty() && syncLine.back() == '\r') syncLine.pop_back();
                if (syncLine.empty()) continue;

                stringstream ss(syncLine);
                string idStr, name, lineName, statusStr;
                getline(ss, idStr, ',');
                getline(ss, name, ',');
                getline(ss, lineName, ',');
                getline(ss, statusStr, ',');

                int id = stoi(idStr);
                int status = (statusStr == "开启") ? 1 : 0;

                if (stationMap.find(id) != stationMap.end()) {
                    stationMap[id].status = status;
                }
            }
            syncFile.close();
        }
        // =====================================
    }
    else {
        cout << "错误：写入 data/Station.csv 失败！" << endl;
    }
}

// ============================================
// 功能5：受影响区域分析
// ============================================
void showAffectedArea() {
    cout << "\n【受影响区域分析】" << endl;
    cout << "-----------------------------------" << endl;

    vector<string> allLines = readAllLines("data/Station.csv");
    if (allLines.empty()) {
        cout << "错误：找不到 data/Station.csv 文件！" << endl;
        return;
    }

    vector<pair<string, string>> closedStations;
    for (size_t i = 1; i < allLines.size(); i++) {
        stringstream ss(allLines[i]);
        string id, name, lineName, status;
        getline(ss, id, ',');
        getline(ss, name, ',');
        getline(ss, lineName, ',');
        getline(ss, status, ',');

        if (status == "关闭") {
            closedStations.push_back(make_pair(name, lineName));
        }
    }

    if (closedStations.empty()) {
        cout << "当前没有关闭的站点，全网运营正常。" << endl;
        return;
    }

    cout << "关闭站点及受影响线路：" << endl;
    for (size_t i = 0; i < closedStations.size(); i++) {
        cout << "  " << closedStations[i].first
            << " -> " << closedStations[i].second << " 运营受影响" << endl;
    }

    cout << "-----------------------------------" << endl;
    cout << "提示：路径规划模块会自动规避关闭站点。" << endl;
}

// ============================================
// 功能6：显示指定线路站点信息（包含换乘站标记）
// ============================================
void showInfoByLine(StationManager* sm) {
    cout << "\n【显示指定线路站点信息】" << endl;

    string targetLine;
    cout << "请输入线路名称（如：1号线、2号线、浦江线）：";
    cin >> targetLine;

    vector<string> allLines = readAllLines("data/Station.csv");
    if (allLines.empty()) {
        cout << "错误：找不到 data/Station.csv 文件！" << endl;
        return;
    }

    // 先检查该线路是否存在
    bool lineExists = false;
    for (size_t i = 1; i < allLines.size(); i++) {
        stringstream ss(allLines[i]);
        string id, name, lineName, status;
        getline(ss, id, ',');
        getline(ss, name, ',');
        getline(ss, lineName, ',');
        getline(ss, status, ',');
        if (lineName == targetLine) {
            lineExists = true;
            break;
        }
    }

    if (!lineExists) {
        cout << "  未找到该线路，请检查输入！" << endl;
        cout << "  常见格式：1号线、2号线、浦江线、市域机场线" << endl;
        return;
    }

    cout << "-----------------------------------" << endl;
    cout << "【" << targetLine << "】站点列表：" << endl;
    cout << "-----------------------------------" << endl;

    int count = 0;
    for (size_t i = 1; i < allLines.size(); i++) {
        stringstream ss(allLines[i]);
        string id, name, lineName, status;
        getline(ss, id, ',');
        getline(ss, name, ',');
        getline(ss, lineName, ',');
        getline(ss, status, ',');

        if (lineName == targetLine) {
            cout << "  " << id << ". " << name;
            // 判断是否为换乘站
            if (sm->isTransferStation(name)) {
                cout << " [换乘站]";
            }
            if (status == "关闭") {
                cout << " [已关闭]";
            }
            cout << endl;
            count++;
        }
    }

    cout << "-----------------------------------" << endl;
    cout << "共 " << count << " 个站点" << endl;
}