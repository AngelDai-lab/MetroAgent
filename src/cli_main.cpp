#ifndef _HAS_STD_BYTE
#define _HAS_STD_BYTE 0
#endif
#include "cli_main.h"
#include "station.h"
#include "graph.h"
#include "pathfinder.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <cstdlib>
#include <windows.h>
using namespace std;

// 编码转换：GBK → UTF-8
// 用途：Windows 窄字符 argv 是 ANSI(GBK) 编码，需转成 UTF-8 与程序内部一致
static string gbkToUtf8(const string& gbk) {
    if (gbk.empty()) return "";
    int wlen = MultiByteToWideChar(936, 0, gbk.c_str(), -1, NULL, 0);
    if (wlen <= 0) return gbk;
    wstring wstr(wlen, L'\0');
    MultiByteToWideChar(936, 0, gbk.c_str(), -1, &wstr[0], wlen);
    int ulen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (ulen <= 0) return gbk;
    string ustr(ulen, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &ustr[0], ulen, NULL, NULL);
    if (!ustr.empty() && ustr.back() == '\0') ustr.pop_back();
    return ustr;
}

// ============================================================
// 轻量级 JSON 输出工具（不引入第三方库）
// ============================================================
static string jsonEscape(const string& s) {
    string out;
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

static string jsonStr(const string& s) {
    return "\"" + jsonEscape(s) + "\"";
}

// ============================================================
// 数据加载（复用 main.cpp 的逻辑）
// ============================================================
static bool loadData(StationManager& sm, Graph& graph) {
    if (!sm.loadStationsFromCSV("data/Station.csv")) {
        cerr << "[CLI] 站点数据加载失败" << endl;
        return false;
    }
    if (!graph.loadEdgesFromCSV("data/Edge.csv")) {
        cerr << "[CLI] 边数据加载失败" << endl;
        return false;
    }
    graph.buildPathfinderAdj(adj, stationMap);
    for (auto& s : sm.getAllStations()) {
        stationMap[s.station_id] = s;
    }
    return true;
}

// ============================================================
// 模糊搜索站点（基于 stationMap）
// ============================================================
static vector<pair<int, Station>> searchStations(const string& keyword) {
    vector<pair<int, Station>> results;
    for (const auto& kv : stationMap) {
        if (kv.second.station_name.find(keyword) != string::npos) {
            results.push_back(kv);
        }
    }
    // 按 ID 排序，保证输出稳定
    sort(results.begin(), results.end(), [](const pair<int, Station>& a, const pair<int, Station>& b) {
        return a.first < b.first;
    });
    return results;
}

// 获取站名（从已转码的 stationMap 取）
static string getStationName(int id) {
    auto it = stationMap.find(id);
    if (it != stationMap.end()) return it->second.station_name;
    return "未知站点";
}

// 判断是否为换乘站（同名站点出现在多条线路）
static bool isTransferStation(const string& name) {
    int cnt = 0;
    for (const auto& kv : stationMap) {
        if (kv.second.station_name == name) cnt++;
    }
    return cnt > 1;
}

// 精确匹配站点（站名完全相等，可能返回多个换乘站条目）
static vector<pair<int, Station>> exactStations(const string& name) {
    vector<pair<int, Station>> results;
    for (const auto& kv : stationMap) {
        if (kv.second.station_name == name) {
            results.push_back(kv);
        }
    }
    sort(results.begin(), results.end(), [](const pair<int, Station>& a, const pair<int, Station>& b) {
        return a.first < b.first;
    });
    return results;
}

// ============================================================
// 把 PathInfo 序列化为 JSON 字符串
// 复用 printPathWithNumber 的分段逻辑
// ============================================================
static string pathToJson(const PathInfo& path) {
    stringstream ss;
    ss << "{";
    ss << "\"total_time\":" << path.totalTime << ",";
    ss << "\"transfer_count\":" << path.transferCnt << ",";
    ss << "\"station_count\":" << path.stationSeq.size() << ",";

    // 分段
    vector<vector<int>> segments;
    vector<string> segmentLines;
    vector<int> segmentDirs;

    if (!path.stationSeq.empty()) {
        segments.push_back({ path.stationSeq[0] });
        string currentLine = "";
        int currentDir = 0;
        for (size_t i = 0; i < path.lineSeq.size(); i++) {
            if (path.lineSeq[i] != "换乘") {
                currentLine = path.lineSeq[i];
                currentDir = (i < path.directionSeq.size()) ? path.directionSeq[i] : 0;
                break;
            }
        }
        if (currentLine.empty()) currentLine = "未知";
        segmentLines.push_back(currentLine);
        segmentDirs.push_back(currentDir);

        for (size_t i = 0; i < path.lineSeq.size(); i++) {
            string edgeLine = path.lineSeq[i];
            if (edgeLine == "换乘") continue;
            if (edgeLine != currentLine) {
                segments.push_back({ path.stationSeq[i + 1] });
                currentLine = edgeLine;
                currentDir = (i < path.directionSeq.size()) ? path.directionSeq[i] : 0;
                segmentLines.push_back(currentLine);
                segmentDirs.push_back(currentDir);
            } else {
                segments.back().push_back(path.stationSeq[i + 1]);
            }
        }
    }

    ss << "\"segments\":[";
    for (size_t segIdx = 0; segIdx < segments.size(); segIdx++) {
        if (segIdx > 0) ss << ",";
        ss << "{";
        string lineDisplay = segmentLines[segIdx];
        int dir = segmentDirs[segIdx];
        if (lineDisplay == "4号线") {
            if (dir == 1) lineDisplay += "（内圈）";
            else if (dir == 2) lineDisplay += "（外圈）";
        }
        ss << "\"line\":" << jsonStr(lineDisplay) << ",";
        ss << "\"stations\":[";
        for (size_t idx = 0; idx < segments[segIdx].size(); idx++) {
            if (idx > 0) ss << ",";
            ss << jsonStr(getStationName(segments[segIdx][idx]));
        }
        ss << "]";
        ss << "}";
    }
    ss << "]";

    // 完整站点序列（便于 LLM 理解）
    ss << ",\"station_seq\":[";
    for (size_t i = 0; i < path.stationSeq.size(); i++) {
        if (i > 0) ss << ",";
        ss << jsonStr(getStationName(path.stationSeq[i]));
    }
    ss << "]";

    ss << "}";
    return ss.str();
}

// ============================================================
// 通用：解析起终点（支持模糊搜索，多个匹配时返回第一个并提示）
// ============================================================
static bool resolveStation(const string& keyword, int& outId, string& outName, string& outLine) {
    auto matches = searchStations(keyword);
    if (matches.empty()) return false;
    outId = matches[0].first;
    outName = matches[0].second.station_name;
    outLine = matches[0].second.line_name;
    return true;
}

// ============================================================
// 各子命令实现
// ============================================================

// search <keyword>
static void cmdSearch(const string& keyword) {
    auto matches = searchStations(keyword);
    stringstream ss;
    ss << "{\"success\":true,\"count\":" << matches.size() << ",\"stations\":[";
    for (size_t i = 0; i < matches.size(); i++) {
        if (i > 0) ss << ",";
        const auto& s = matches[i].second;
        ss << "{\"id\":" << s.station_id
           << ",\"name\":" << jsonStr(s.station_name)
           << ",\"line\":" << jsonStr(s.line_name)
           << ",\"status\":" << (s.status == 1 ? "\"open\"" : "\"closed\"")
           << "}";
    }
    ss << "]}";
    cout << ss.str() << endl;
}

// shortest_time <from> <to>
static void cmdShortestTime(StationManager& sm, const string& fromKw, const string& toKw) {
    int fromId, toId; string fromName, toName, fromLine, toLine;
    if (!resolveStation(fromKw, fromId, fromName, fromLine)) {
        cout << "{\"success\":false,\"error\":\"起点站未找到\",\"keyword\":" << jsonStr(fromKw) << "}" << endl;
        return;
    }
    if (!resolveStation(toKw, toId, toName, toLine)) {
        cout << "{\"success\":false,\"error\":\"终点站未找到\",\"keyword\":" << jsonStr(toKw) << "}" << endl;
        return;
    }
    if (fromId == toId) {
        cout << "{\"success\":false,\"error\":\"起点和终点相同\"}" << endl;
        return;
    }
    PathInfo result = getSingleShortestTime(fromId, toId);
    if (result.stationSeq.empty()) {
        cout << "{\"success\":false,\"error\":\"未找到路径\"}" << endl;
        return;
    }
    cout << "{\"success\":true,\"from\":" << jsonStr(fromName)
         << ",\"to\":" << jsonStr(toName)
         << ",\"path\":" << pathToJson(result) << "}" << endl;
}

// min_transfer <from> <to>
static void cmdMinTransfer(StationManager& sm, const string& fromKw, const string& toKw) {
    int fromId, toId; string fromName, toName, fromLine, toLine;
    if (!resolveStation(fromKw, fromId, fromName, fromLine)) {
        cout << "{\"success\":false,\"error\":\"起点站未找到\",\"keyword\":" << jsonStr(fromKw) << "}" << endl;
        return;
    }
    if (!resolveStation(toKw, toId, toName, toLine)) {
        cout << "{\"success\":false,\"error\":\"终点站未找到\",\"keyword\":" << jsonStr(toKw) << "}" << endl;
        return;
    }
    if (fromId == toId) {
        cout << "{\"success\":false,\"error\":\"起点和终点相同\"}" << endl;
        return;
    }
    PathInfo result = getSingleMinTransfer(fromId, toId);
    if (result.stationSeq.empty()) {
        cout << "{\"success\":false,\"error\":\"未找到路径\"}" << endl;
        return;
    }
    cout << "{\"success\":true,\"from\":" << jsonStr(fromName)
         << ",\"to\":" << jsonStr(toName)
         << ",\"path\":" << pathToJson(result) << "}" << endl;
}

// k_shortest_time <from> <to> <k>
static void cmdKShortestTime(StationManager& sm, const string& fromKw, const string& toKw, int k) {
    int fromId, toId; string fromName, toName, fromLine, toLine;
    if (!resolveStation(fromKw, fromId, fromName, fromLine)) {
        cout << "{\"success\":false,\"error\":\"起点站未找到\",\"keyword\":" << jsonStr(fromKw) << "}" << endl;
        return;
    }
    if (!resolveStation(toKw, toId, toName, toLine)) {
        cout << "{\"success\":false,\"error\":\"终点站未找到\",\"keyword\":" << jsonStr(toKw) << "}" << endl;
        return;
    }
    if (fromId == toId) {
        cout << "{\"success\":false,\"error\":\"起点和终点相同\"}" << endl;
        return;
    }
    vector<PathInfo> results = getKShortestTime(fromId, toId, k);
    if (results.empty()) {
        cout << "{\"success\":false,\"error\":\"未找到路径\"}" << endl;
        return;
    }
    stringstream ss;
    ss << "{\"success\":true,\"from\":" << jsonStr(fromName)
       << ",\"to\":" << jsonStr(toName)
       << ",\"count\":" << results.size()
       << ",\"paths\":[";
    for (size_t i = 0; i < results.size(); i++) {
        if (i > 0) ss << ",";
        ss << pathToJson(results[i]);
    }
    ss << "]}";
    cout << ss.str() << endl;
}

// k_min_transfer <from> <to> <k>
static void cmdKMinTransfer(StationManager& sm, const string& fromKw, const string& toKw, int k) {
    int fromId, toId; string fromName, toName, fromLine, toLine;
    if (!resolveStation(fromKw, fromId, fromName, fromLine)) {
        cout << "{\"success\":false,\"error\":\"起点站未找到\",\"keyword\":" << jsonStr(fromKw) << "}" << endl;
        return;
    }
    if (!resolveStation(toKw, toId, toName, toLine)) {
        cout << "{\"success\":false,\"error\":\"终点站未找到\",\"keyword\":" << jsonStr(toKw) << "}" << endl;
        return;
    }
    if (fromId == toId) {
        cout << "{\"success\":false,\"error\":\"起点和终点相同\"}" << endl;
        return;
    }
    vector<PathInfo> results = getKMinTransfer(fromId, toId, k);
    if (results.empty()) {
        cout << "{\"success\":false,\"error\":\"未找到路径\"}" << endl;
        return;
    }
    stringstream ss;
    ss << "{\"success\":true,\"from\":" << jsonStr(fromName)
       << ",\"to\":" << jsonStr(toName)
       << ",\"count\":" << results.size()
       << ",\"paths\":[";
    for (size_t i = 0; i < results.size(); i++) {
        if (i > 0) ss << ",";
        ss << pathToJson(results[i]);
    }
    ss << "]}";
    cout << ss.str() << endl;
}

// station_status <keyword>
static void cmdStationStatus(const string& keyword) {
    auto matches = searchStations(keyword);
    if (matches.empty()) {
        cout << "{\"success\":false,\"error\":\"未找到站点\"}" << endl;
        return;
    }
    stringstream ss;
    ss << "{\"success\":true,\"count\":" << matches.size() << ",\"stations\":[";
    for (size_t i = 0; i < matches.size(); i++) {
        if (i > 0) ss << ",";
        const auto& s = matches[i].second;
        ss << "{\"name\":" << jsonStr(s.station_name)
           << ",\"line\":" << jsonStr(s.line_name)
           << ",\"status\":" << (s.status == 1 ? "\"open\"" : "\"closed\"")
           << "}";
    }
    ss << "]}";
    cout << ss.str() << endl;
}

// set_status <station_name> <open|close>
// 重写一份非交互版的 CSV 状态更新
static bool setStationStatusCsv(const string& targetName, const string& newStatus) {
    ifstream inFile("data/Station.csv");
    if (!inFile) return false;
    vector<string> allLines;
    string line;
    while (getline(inFile, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        allLines.push_back(line);
    }
    inFile.close();

    int changed = 0;
    for (size_t i = 1; i < allLines.size(); i++) {
        stringstream ss(allLines[i]);
        string id, name, lineName, status;
        getline(ss, id, ',');
        getline(ss, name, ',');
        getline(ss, lineName, ',');
        getline(ss, status, ',');
        if (name == targetName) {
            allLines[i] = id + "," + name + "," + lineName + "," + newStatus;
            changed++;
        }
    }

    ofstream outFile("data/Station.csv");
    if (!outFile) return false;
    for (size_t i = 0; i < allLines.size(); i++) {
        outFile << allLines[i] << endl;
    }
    outFile.close();

    // 同步 stationMap
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
    return changed > 0;
}

static void cmdSetStatus(const string& stationName, const string& statusStr) {
    string csvStatus = (statusStr == "open" || statusStr == "开启") ? "开启" : "关闭";
    bool ok = setStationStatusCsv(stationName, csvStatus);
    if (!ok) {
        cout << "{\"success\":false,\"error\":\"未找到站点或写入失败\"}" << endl;
        return;
    }
    cout << "{\"success\":true,\"station\":" << jsonStr(stationName)
         << ",\"status\":" << jsonStr(csvStatus == "开启" ? "open" : "closed") << "}" << endl;
}

// closed_stations
static void cmdClosedStations() {
    vector<pair<string, string>> closed;
    for (const auto& kv : stationMap) {
        if (kv.second.status == 0) {
            closed.push_back({ kv.second.station_name, kv.second.line_name });
        }
    }
    sort(closed.begin(), closed.end());
    stringstream ss;
    ss << "{\"success\":true,\"count\":" << closed.size() << ",\"stations\":[";
    for (size_t i = 0; i < closed.size(); i++) {
        if (i > 0) ss << ",";
        ss << "{\"name\":" << jsonStr(closed[i].first)
           << ",\"line\":" << jsonStr(closed[i].second) << "}";
    }
    ss << "]}";
    cout << ss.str() << endl;
}

// line_info <line_name>
static void cmdLineInfo(StationManager& sm, const string& lineName) {
    vector<Station> lineStations;
    for (const auto& kv : stationMap) {
        if (kv.second.line_name == lineName) {
            lineStations.push_back(kv.second);
        }
    }
    sort(lineStations.begin(), lineStations.end(), [](const Station& a, const Station& b) {
        return a.station_id < b.station_id;
    });
    stringstream ss;
    ss << "{\"success\":true,\"line\":" << jsonStr(lineName)
       << ",\"count\":" << lineStations.size() << ",\"stations\":[";
    for (size_t i = 0; i < lineStations.size(); i++) {
        if (i > 0) ss << ",";
        const auto& s = lineStations[i];
        bool isTransfer = isTransferStation(s.station_name);
        ss << "{\"id\":" << s.station_id
           << ",\"name\":" << jsonStr(s.station_name)
           << ",\"is_transfer\":" << (isTransfer ? "true" : "false")
           << ",\"status\":" << (s.status == 1 ? "\"open\"" : "\"closed\"")
           << "}";
    }
    ss << "]}";
    cout << ss.str() << endl;
}

// network_stats
static void cmdNetworkStats() {
    int total = 0, openCount = 0, closeCount = 0;
    unordered_map<string, int> lineCount;
    unordered_map<string, int> nameCount;
    for (const auto& kv : stationMap) {
        total++;
        if (kv.second.status == 1) openCount++; else closeCount++;
        lineCount[kv.second.line_name]++;
        nameCount[kv.second.station_name]++;
    }
    int transferCount = 0;
    for (const auto& p : nameCount) {
        if (p.second > 1) transferCount++;
    }
    int edgeCount = 0;
    for (const auto& row : adj) edgeCount += (int)row.size();

    stringstream ss;
    ss << "{\"success\":true"
       << ",\"total_stations\":" << total
       << ",\"open_stations\":" << openCount
       << ",\"closed_stations\":" << closeCount
       << ",\"transfer_stations\":" << transferCount
       << ",\"line_count\":" << lineCount.size()
       << ",\"edge_count\":" << edgeCount
       << ",\"lines\":[";
    vector<string> lines;
    for (const auto& p : lineCount) lines.push_back(p.first);
    sort(lines.begin(), lines.end());
    for (size_t i = 0; i < lines.size(); i++) {
        if (i > 0) ss << ",";
        ss << "{\"name\":" << jsonStr(lines[i]) << ",\"count\":" << lineCount[lines[i]] << "}";
    }
    ss << "]}";
    cout << ss.str() << endl;
}

// reset_status
static void cmdResetStatus() {
    ifstream inFile("data/Station_init.csv");
    if (!inFile) {
        cout << "{\"success\":false,\"error\":\"找不到初始数据文件\"}" << endl;
        return;
    }
    vector<string> allLines;
    string line;
    while (getline(inFile, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        allLines.push_back(line);
    }
    inFile.close();

    ofstream outFile("data/Station.csv");
    if (!outFile) {
        cout << "{\"success\":false,\"error\":\"写入失败\"}" << endl;
        return;
    }
    for (size_t i = 0; i < allLines.size(); i++) {
        outFile << allLines[i] << endl;
    }
    outFile.close();

    // 同步 stationMap
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
    cout << "{\"success\":true,\"message\":\"已恢复所有站点初始状态\"}" << endl;
}

// ============================================================
// CLI 主入口
// ============================================================
int runCli(int argc, char** argv) {
    if (argc < 3) {
        cout << "{\"success\":false,\"error\":\"缺少子命令\"}" << endl;
        return 1;
    }

    // 命令行参数是 ANSI(GBK) 编码，统一转成 UTF-8
    vector<string> args;
    for (int i = 0; i < argc; i++) {
        args.push_back(gbkToUtf8(argv[i]));
    }

    string command = args[2];
    StationManager sm;
    Graph graph(&sm);

    // set_status 和 reset_status 也需要加载数据来同步 stationMap
    if (!loadData(sm, graph)) {
        cout << "{\"success\":false,\"error\":\"数据加载失败\"}" << endl;
        return 1;
    }

    try {
        if (command == "search" && argc >= 4) {
            cmdSearch(args[3]);
        }
        else if (command == "shortest_time" && argc >= 5) {
            cmdShortestTime(sm, args[3], args[4]);
        }
        else if (command == "min_transfer" && argc >= 5) {
            cmdMinTransfer(sm, args[3], args[4]);
        }
        else if (command == "k_shortest_time" && argc >= 6) {
            cmdKShortestTime(sm, args[3], args[4], atoi(args[5].c_str()));
        }
        else if (command == "k_min_transfer" && argc >= 6) {
            cmdKMinTransfer(sm, args[3], args[4], atoi(args[5].c_str()));
        }
        else if (command == "station_status" && argc >= 4) {
            cmdStationStatus(args[3]);
        }
        else if (command == "set_status" && argc >= 5) {
            cmdSetStatus(args[3], args[4]);
        }
        else if (command == "closed_stations") {
            cmdClosedStations();
        }
        else if (command == "line_info" && argc >= 4) {
            cmdLineInfo(sm, args[3]);
        }
        else if (command == "network_stats") {
            cmdNetworkStats();
        }
        else if (command == "reset_status") {
            cmdResetStatus();
        }
        else {
            cout << "{\"success\":false,\"error\":\"未知或不完整的命令: " << jsonEscape(command) << "\"}" << endl;
            return 1;
        }
    }
    catch (const exception& e) {
        cout << "{\"success\":false,\"error\":\"" << jsonEscape(e.what()) << "\"}" << endl;
        return 1;
    }
    return 0;
}
