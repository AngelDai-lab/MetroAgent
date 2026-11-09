#include "pathfinder.h"
#include <algorithm>
#include<iostream>
#include <fstream>   
#include <sstream>   
using namespace std;

// 全局变量定义（不带extern，只写在cpp中）
vector<vector<Edge>> adj;
unordered_map<int, Station> stationMap;

// ============================================================
// 辅助函数：根据站名搜索匹配的站点（支持模糊匹配）
// ============================================================
static vector<pair<int, string>> searchStationsByName(StationManager* sm, const string& keyword) {
    vector<pair<int, string>> results;
    const auto& allStations = sm->getAllStations();

    for (const auto& s : allStations) {
        if (s.station_name.find(keyword) != string::npos) {
            string displayName = s.station_name + "（" + s.line_name + "）";
            results.push_back({ s.station_id, displayName });
        }
    }
    return results;
}

// ============================================================
// 辅助函数：让用户选择站点（如果有多个匹配）
// ============================================================
static int selectStation(StationManager* sm, const string& prompt) {
    string keyword;
    vector<pair<int, string>> matches;

    while (true) {
        cout << prompt;
        cin >> keyword;

        matches = searchStationsByName(sm, keyword);

        if (matches.empty()) {
            cout << "未找到匹配的站点，请重新输入（如：人民广场、徐家汇）：" << endl;
            continue;
        }

        if (matches.size() == 1) {
            cout << "已匹配：" << matches[0].second << endl;
            return matches[0].first;
        }

        cout << "找到多个匹配的站点，请选择：" << endl;
        for (size_t i = 0; i < matches.size(); i++) {
            cout << "  " << i + 1 << ". " << matches[i].second << endl;
        }
        cout << "请输入编号（1-" << matches.size() << "）：";

        int choice;
        cin >> choice;
        if (choice >= 1 && choice <= (int)matches.size()) {
            return matches[choice - 1].first;
        }
        cout << "无效编号，请重新输入。" << endl;
    }
}

// 校验站点是否开放
bool isStationValid(int sid)
{
    if (stationMap.find(sid) == stationMap.end())
        return false;
    return stationMap[sid].status == 1;
}

void calcPathTimeTransfer(PathInfo& path)
{
    path.totalTime = 0;
    path.transferCnt = 0;

    // ⭐ 确保 directionSeq 大小与 lineSeq 一致
    if (path.directionSeq.size() < path.lineSeq.size()) {
        path.directionSeq.resize(path.lineSeq.size(), 0);
    }

    // 1. 累加所有边的通行时间（含行车边+虚拟换乘边）
    for (size_t i = 0; i < path.lineSeq.size(); i++)
    {
        string curLine = path.lineSeq[i];
        int u = path.stationSeq[i];
        int v = path.stationSeq[i + 1];

        for (Edge& e : adj[u])
        {
            if (e.to == v && e.line == curLine)
            {
                path.totalTime += e.cost;
                break;
            }
        }
    }

    // 2. 过滤掉虚拟的"换乘"线路，只保留真实运营线路
    vector<string> realLines;
    for (const string& s : path.lineSeq)
    {
        if (s != "换乘")
        {
            realLines.push_back(s);
        }
    }

    // 3. 合并连续重复的真实线路
    vector<string> compLine;
    for (const string& s : realLines)
    {
        if (compLine.empty() || compLine.back() != s)
        {
            compLine.push_back(s);
        }
    }

    // 4. 真实换乘次数 = 压缩后线路数 - 1
    path.transferCnt = compLine.size() - 1;

    // ⚠️ 换乘时间已经在 Edge.csv 的换乘边中记录了（5分钟）
    // 这里不再重复添加，否则换乘时间会被算两次
    // path.totalTime += path.transferCnt * 5;
}

// ============================================================
// 带节点封锁的 Dijkstra 最短路径算法
// ============================================================
PathInfo dijkstraBlockNode(int start, int end, set<int>& blockNodes)
{
    PathInfo res;

    if (!isStationValid(start) || !isStationValid(end))
        return res;

    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> q;
    vector<int> dist(10000, INT_MAX);
    vector<int> pre(10000, -1);
    vector<string> preLine(10000);

    dist[start] = 0;
    q.push({ 0, start });

    while (!q.empty())
    {
        auto [curCost, u] = q.top();//取出距离源点最近的节点
        q.pop();
        if (u == end) break;//到达终点直接退出
        if (curCost > dist[u]) continue;

        //遍历u在邻接表中的所有邻居
        for (Edge& e : adj[u])
        {
            int v = e.to;

            //跳过封锁节点或关闭节点
            if (blockNodes.count(v) || !isStationValid(v))
                continue;
            int newCost = curCost + e.cost;
            if (newCost < dist[v])//如果找到更短路径，更新
            {
                dist[v] = newCost;
                pre[v] = u;
                preLine[v] = e.line;
                q.push({ newCost, v });
            }
        }
    }

    if (dist[end] == INT_MAX)
        return res;

    // 回溯站点
    int cur = end;
    while (cur != -1)
    {
        res.stationSeq.insert(res.stationSeq.begin(), cur);
        cur = pre[cur];
    }

    // 重建原始线路序列（不做任何修改，保证打印下标对应）
    res.lineSeq.clear();
    res.directionSeq.clear();  // 清空方向序列
    for (size_t i = 1; i < res.stationSeq.size(); i++)
    {
        int v = res.stationSeq[i];
        res.lineSeq.push_back(preLine[v]);

        // ⭐ 记录方向：从邻接表中查找
        int u = res.stationSeq[i - 1];
        int dir = 0;
        for (Edge& e : adj[u]) {
            if (e.to == v && e.line == preLine[v]) {
                dir = e.direction;
                break;
            }
        }
        res.directionSeq.push_back(dir);
    }

    calcPathTimeTransfer(res);
    return res;
}

// 1. 单条最短时间路径
PathInfo getSingleShortestTime(int startId, int endId)
{
    set<int> emptyBlock;
    return dijkstraBlockNode(startId, endId, emptyBlock);
}

// 2. Yen算法求前3条最短时间路径
vector<PathInfo> getKShortestTime(int startId, int endId, int k)
{
    vector<PathInfo> kPaths;
    vector<PathInfo> candidate;
    set<vector<int>> existPath;

    PathInfo firstPath = getSingleShortestTime(startId, endId);
    if (firstPath.stationSeq.empty())
        return kPaths;
    kPaths.push_back(firstPath);
    existPath.insert(firstPath.stationSeq);

    for (int i = 1; i < k; i++)
    {
        PathInfo lastBest = kPaths.back();
        int pathLen = lastBest.stationSeq.size();
        bool hasNew = false;

        for (int spurIdx = 0; spurIdx < pathLen - 1; spurIdx++)
        {
            int spurNode = lastBest.stationSeq[spurIdx];
            set<int> block;

            for (auto& p : kPaths)
            {
                if (p.stationSeq.size() > spurIdx && p.stationSeq[spurIdx] == spurNode)
                    block.insert(p.stationSeq[spurIdx + 1]);
            }
            for (int j = 0; j < spurIdx; j++)
                block.insert(lastBest.stationSeq[j]);

            PathInfo spurPath = dijkstraBlockNode(spurNode, endId, block);
            if (spurPath.stationSeq.empty()) continue;

            PathInfo newPath;
            newPath.stationSeq.assign(lastBest.stationSeq.begin(), lastBest.stationSeq.begin() + spurIdx + 1);
            for (int s : spurPath.stationSeq)
            {
                if (s != spurNode)
                    newPath.stationSeq.push_back(s);
            }

            // 原样拼接线路，不做修改，统一交给calc处理
            newPath.lineSeq.clear();
            for (int j = 0; j < spurIdx; j++)
                newPath.lineSeq.push_back(lastBest.lineSeq[j]);
            for (string l : spurPath.lineSeq)
                newPath.lineSeq.push_back(l);

            calcPathTimeTransfer(newPath);

            if (!existPath.count(newPath.stationSeq))
            {
                candidate.push_back(newPath);
                existPath.insert(newPath.stationSeq);
                hasNew = true;
            }
        }
        if (!hasNew || candidate.empty()) break;

        sort(candidate.begin(), candidate.end());
        kPaths.push_back(candidate[0]);
        candidate.erase(candidate.begin());
    }
    return kPaths;
}

// 3. 单条最少换乘路径 BFS
PathInfo getSingleMinTransfer(int startId, int endId) {
    PathInfo res;
    if (!isStationValid(startId) || !isStationValid(endId))
        return res;

    queue<pair<pair<int, string>, PathInfo>> q;
    set<pair<int, string>> visited;

    PathInfo initPath;
    initPath.stationSeq.push_back(startId);
    // directionSeq 默认为空
    q.push({ {startId, ""}, initPath });
    visited.insert({ startId, "" });

    while (!q.empty()) {
        auto frontData = q.front();
        q.pop();
        int curStation = frontData.first.first;
        string curLine = frontData.first.second;
        PathInfo curPath = frontData.second;

        if (curStation == endId) {
            calcPathTimeTransfer(curPath);
            return curPath;
        }

        for (Edge& e : adj[curStation]) {
            int nextStation = e.to;
            if (!isStationValid(nextStation)) continue;

            pair<int, string> state = { nextStation, e.line };
            if (visited.count(state)) continue;

            PathInfo newPath = curPath;
            newPath.stationSeq.push_back(nextStation);
            newPath.lineSeq.push_back(e.line);
            newPath.directionSeq.push_back(e.direction);  // ⭐ 添加方向

            visited.insert(state);
            q.push({ {nextStation, e.line}, newPath });
        }
    }
    return res;
}

// 4. 前3条最少换乘路径
vector<PathInfo> getKMinTransfer(int startId, int endId, int k)
{
    vector<PathInfo> rawPaths = getKShortestTime(startId, endId, k * 3);
    sort(rawPaths.begin(), rawPaths.end(), [](const PathInfo& a, const PathInfo& b)
        {
            return a.cmpTransfer(b);
        });

    vector<PathInfo> result;
    set<vector<int>> seenPath;
    for (auto& p : rawPaths)
    {
        if (!seenPath.count(p.stationSeq))
        {
            result.push_back(p);
            seenPath.insert(p.stationSeq);
        }
        if (result.size() >= k) break;
    }
    return result;
}

// ============================================
// 路径输出与去重模块
// ============================================

static string joinSegments(const vector<string>& parts, const string& delimiter) {
    string result;
    for (size_t i = 0; i < parts.size(); i++) {
        result += parts[i];
        if (i != parts.size() - 1) {
            result += delimiter;
        }
    }
    return result;
}
void printPath(const PathInfo& path, const StationManager& sm) {
    printPathWithNumber(path, sm, 1);
}

void printPathWithNumber(const PathInfo& path, const StationManager& sm, int pathNumber) {
    // ===== 1. 标题行 =====
    cout << "耗时最短路径 " << pathNumber << ": （耗时 " << path.totalTime
        << " 分钟，换乘 " << path.transferCnt
        << " 次，途径站台数（含起点）" << path.stationSeq.size() << " 个）" << endl;

    if (path.stationSeq.empty()) return;

    // ===== 2. 构建分段：按线路变化换行 =====
    vector<vector<int>> segments;       // 每一段的站点ID列表
    vector<string> segmentLines;        // 每一段的线路名

    // 第一段：从第0个站点开始
    segments.push_back({ path.stationSeq[0] });
    string currentLine = "";

    // 确定第一段的线路名
    for (size_t i = 0; i < path.lineSeq.size(); i++) {
        if (path.lineSeq[i] != "换乘") {
            currentLine = path.lineSeq[i];
            break;
        }
    }
    if (currentLine.empty()) currentLine = "未知";
    segmentLines.push_back(currentLine);

    // 遍历边，检测线路变化
    for (size_t i = 0; i < path.lineSeq.size(); i++) {
        string edgeLine = path.lineSeq[i];
        if (edgeLine == "换乘") continue;

        if (edgeLine != currentLine) {
            segments.push_back({ path.stationSeq[i + 1] });
            currentLine = edgeLine;
            segmentLines.push_back(currentLine);
        }
        else {
            segments.back().push_back(path.stationSeq[i + 1]);
        }
    }

    // ===== 3. 输出 =====
    for (size_t segIdx = 0; segIdx < segments.size(); segIdx++) {
        string lineStr;

        // 计算当前段第一个站点在 path.stationSeq 中的索引
        int startIdx = -1;
        for (size_t i = 0; i < path.stationSeq.size(); i++) {
            if (path.stationSeq[i] == segments[segIdx][0]) {
                startIdx = (int)i;
                break;
            }
        }

        for (size_t idx = 0; idx < segments[segIdx].size(); idx++) {
            int id = segments[segIdx][idx];
            string name = sm.getStationName(id);
            string lineDisplay = segmentLines[segIdx];

            // 判断是否为4号线，显示内圈/外圈
            if (lineDisplay == "4号线") {
                int dirIdx = startIdx + (int)idx - 1;
                if (dirIdx >= 0 && dirIdx < (int)path.directionSeq.size()) {
                    if (path.directionSeq[dirIdx] == 1) {
                        lineDisplay += "（内圈）";
                    }
                    else if (path.directionSeq[dirIdx] == 2) {
                        lineDisplay += "（外圈）";
                    }
                }
            }

            lineStr += name + " (" + lineDisplay + ")";
            if (idx != segments[segIdx].size() - 1) {
                lineStr += " -> ";
            }
        }

        if (segIdx == 0) {
            cout << "起点: " << lineStr << endl;
        }
        else {
            int transferStationId = segments[segIdx - 1].back();
            string transferName = sm.getStationName(transferStationId);
            cout << "    " << transferName << " 换乘至 [" << segmentLines[segIdx] << "]==>" << endl;
            cout << "    " << lineStr << endl;
        }
    }
}

void printKPaths(const vector<PathInfo>& paths, const StationManager& sm) {
    for (size_t i = 0; i < paths.size(); i++) {
        cout << "========== 路径 " << i + 1 << " ==========" << endl;
        printPathWithNumber(paths[i], sm, i + 1);
        cout << endl;
    }
}

bool isPathDuplicate(const PathInfo& a, const PathInfo& b) {
    if (a.stationSeq.size() != b.stationSeq.size()) {
        return false;
    }
    for (size_t i = 0; i < a.stationSeq.size(); i++) {
        if (a.stationSeq[i] != b.stationSeq[i]) {
            return false;
        }
    }
    return true;
}

vector<PathInfo> removeDuplicatePaths(const vector<PathInfo>& paths) {
    vector<PathInfo> uniquePaths;
    for (const auto& p : paths) {
        bool isDup = false;
        for (const auto& q : uniquePaths) {
            if (isPathDuplicate(p, q)) {
                isDup = true;
                break;
            }
        }
        if (!isDup) {
            uniquePaths.push_back(p);
        }
    }
    return uniquePaths;
}

// ============================================
// 菜单调用的接口函数
// ============================================

void shortestTimePath(Graph* graph, StationManager* sm) {
    cout << "\n【时间最优路径查询】" << endl;

    int startId = selectStation(sm, "请输入起点站名称（如：人民广场）：");
    int endId = selectStation(sm, "请输入终点站名称（如：虹桥火车站）：");

    if (startId == endId) {
        cout << "起点和终点相同，无需查询！" << endl;
        return;
    }

    PathInfo result = getSingleShortestTime(startId, endId);
    if (result.stationSeq.empty()) {
        cout << "未找到路径！" << endl;
    }
    else {
        printPath(result, *sm);
    }
}

void minTransferPath(Graph* graph, StationManager* sm) {
    cout << "\n【换乘最优路径查询】" << endl;

    int startId = selectStation(sm, "请输入起点站名称（如：人民广场）：");
    int endId = selectStation(sm, "请输入终点站名称（如：虹桥火车站）：");

    if (startId == endId) {
        cout << "起点和终点相同，无需查询！" << endl;
        return;
    }

    PathInfo result = getSingleMinTransfer(startId, endId);
    if (result.stationSeq.empty()) {
        cout << "未找到路径！" << endl;
    }
    else {
        printPath(result, *sm);
    }
}

void kShortestTimePaths(Graph* graph, StationManager* sm) {
    cout << "\n【K条时间最优路径查询】" << endl;

    int startId = selectStation(sm, "请输入起点站名称（如：人民广场）：");
    int endId = selectStation(sm, "请输入终点站名称（如：虹桥火车站）：");

    if (startId == endId) {
        cout << "起点和终点相同，无需查询！" << endl;
        return;
    }

    int k = 3;
    vector<PathInfo> results = getKShortestTime(startId, endId, k);
    if (results.empty()) {
        cout << "未找到路径！" << endl;
    }
    else {
        printKPaths(results, *sm);
    }
}

void kMinTransferPaths(Graph* graph, StationManager* sm) {
    cout << "\n【K条换乘最优路径查询】" << endl;

    int startId = selectStation(sm, "请输入起点站名称（如：人民广场）：");
    int endId = selectStation(sm, "请输入终点站名称（如：虹桥火车站）：");

    if (startId == endId) {
        cout << "起点和终点相同，无需查询！" << endl;
        return;
    }

    int k = 3;
    vector<PathInfo> results = getKMinTransfer(startId, endId, k);
    if (results.empty()) {
        cout << "未找到路径！" << endl;
    }
    else {
        printKPaths(results, *sm);
    }
}

void showNetworkAnalysis(Graph* graph, StationManager* sm) {
    cout << "\n【地铁网络基本信息统计】" << endl;
    cout << "=====================================" << endl;

    ifstream file("data/Station.csv");
    if (!file.is_open()) {
        cout << "错误：无法打开 data/Station.csv 文件！" << endl;
        return;
    }

    string line;
    vector<vector<string>> allRows;
    getline(file, line);

    while (getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        stringstream ss(line);
        string id, name, lineName, status;
        getline(ss, id, ',');
        getline(ss, name, ',');
        getline(ss, lineName, ',');
        getline(ss, status, ',');
        allRows.push_back({ id, name, lineName, status });
    }
    file.close();

    cout << "【站点统计】" << endl;
    cout << "  总站点数（含换乘站重复计算）: " << allRows.size() << " 个" << endl;

    unordered_map<string, int> stationNameCount;
    for (const auto& row : allRows) {
        stationNameCount[row[1]]++;
    }
    int transferStationCount = 0;
    for (const auto& pair : stationNameCount) {
        if (pair.second > 1) transferStationCount++;
    }
    cout << "  换乘站数量: " << transferStationCount << " 个" << endl;

    unordered_map<string, int> lineCount;
    for (const auto& row : allRows) {
        lineCount[row[2]]++;
    }
    cout << "  运营线路总数: " << lineCount.size() << " 条" << endl;

    int openCount = 0, closeCount = 0;
    for (const auto& row : allRows) {
        if (row[3] == "开启") openCount++;
        else if (row[3] == "关闭") closeCount++;
    }
    cout << "  开启站点: " << openCount << " 个" << endl;
    cout << "  关闭站点: " << closeCount << " 个" << endl;

    cout << "\n【线路列表】" << endl;
    vector<string> lineNames;
    for (const auto& pair : lineCount) {
        lineNames.push_back(pair.first);
    }
    sort(lineNames.begin(), lineNames.end());
    for (const auto& name : lineNames) {
        cout << "  " << name << " (" << lineCount[name] << " 个站点)" << endl;
    }

    cout << "=====================================" << endl;
}