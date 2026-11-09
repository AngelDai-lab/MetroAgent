#include "menu.h"
#include "station.h"
#include "graph.h"
#include "pathfinder.h"
#include <iostream>
#include <string>
using namespace std;

// ============================================================
// 运营管理模块函数声明（定义在 manager.cpp 中）
// ============================================================
void updateStationCsvStatus();
void manualUpdateStation();
void showClosedStations();
void resetStationStatus();
void showAffectedArea();
void showInfoByLine(StationManager* sm);

// ============================================================
// 主菜单
// ============================================================
void showMainMenu(Graph* graph, StationManager* sm) {
    int choice;
    do {
        cout << "\n=====================================" << endl;
        cout << "      上海地铁线路查询系统" << endl;
        cout << "=====================================" << endl;
        cout << "  1. 时间最优路径查询" << endl;
        cout << "  2. 换乘最优路径查询" << endl;
        cout << "  3. 线路站点信息/运营状态管理" << endl;
        cout << "  4. 网络分析" << endl;
        cout << "  0. 退出系统" << endl;
        cout << "=====================================" << endl;
        cout << "请输入选项：";

        cin >> choice;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "无效选项，请重新输入！" << endl;
            continue;
        }

        switch (choice) {
        case 1:
            showTimeMenu(graph, sm);
            break;
        case 2:
            showTransferMenu(graph, sm);
            break;
        case 3:
            showStationManageMenu(sm);  // ✅ 传 sm
            break;
        case 4:
            showNetworkAnalysis(graph, sm);
            break;
        case 0:
            cout << "感谢使用，再见！" << endl;
            break;
        default:
            cout << "无效选项，请重新输入！" << endl;
            break;
        }
    } while (choice != 0);
}

// ============================================================
// 时间最优路径二级菜单
// ============================================================
void showTimeMenu(Graph* graph, StationManager* sm) {
    int choice;
    do {
        cout << "\n=====================================" << endl;
        cout << "       时间最优路径查询" << endl;
        cout << "=====================================" << endl;
        cout << "  1. 单条最短时间路径" << endl;
        cout << "  2. 3条最短时间路径" << endl;
        cout << "  0. 返回上级菜单" << endl;
        cout << "=====================================" << endl;
        cout << "请输入选项：";
        cin >> choice;

        switch (choice) {
        case 1:
            shortestTimePath(graph, sm);
            break;
        case 2:
            kShortestTimePaths(graph, sm);
            break;
        case 0:
            cout << "返回上级菜单..." << endl;
            break;
        default:
            cout << "无效选项，请重新输入！" << endl;
        }
    } while (choice != 0);
}

// ============================================================
// 换乘最优路径二级菜单
// ============================================================
void showTransferMenu(Graph* graph, StationManager* sm) {
    int choice;
    do {
        cout << "\n=====================================" << endl;
        cout << "       换乘最优路径查询" << endl;
        cout << "=====================================" << endl;
        cout << "  1. 单条最少换乘路径" << endl;
        cout << "  2. 3条最少换乘路径" << endl;
        cout << "  0. 返回上级菜单" << endl;
        cout << "=====================================" << endl;
        cout << "请输入选项：";
        cin >> choice;

        switch (choice) {
        case 1:
            minTransferPath(graph, sm);
            break;
        case 2:
            kMinTransferPaths(graph, sm);
            break;
        case 0:
            cout << "返回上级菜单..." << endl;
            break;
        default:
            cout << "无效选项，请重新输入！" << endl;
        }
    } while (choice != 0);
}

// ============================================================
// 二级菜单：线路站点信息/运营状态管理
// ============================================================
void showStationManageMenu(StationManager* sm) {  // ✅ 加参数
    int choice;
    do {
        cout << "\n=====================================" << endl;
        cout << "    线路站点信息/运营状态管理" << endl;
        cout << "=====================================" << endl;
        cout << "  1. CSV批量更新站点状态" << endl;
        cout << "  2. 手工更新站点状态" << endl;
        cout << "  3. 显示当前关闭站点" << endl;
        cout << "  4. 恢复初始状态" << endl;
        cout << "  5. 受影响区域分析" << endl;
        cout << "  6. 显示指定线路站点信息" << endl;
        cout << "  0. 返回主菜单" << endl;
        cout << "=====================================" << endl;
        cout << "请输入选项：";
        cin >> choice;

        switch (choice) {
        case 1:
            updateStationCsvStatus();
            break;
        case 2:
            manualUpdateStation();
            break;
        case 3:
            showClosedStations();
            break;
        case 4:
            resetStationStatus();
            break;
        case 5:
            showAffectedArea();
            break;
        case 6:
            showInfoByLine(sm); 
            break;
        case 0:
            cout << "返回主菜单..." << endl;
            break;
        default:
            cout << "无效选项，请重新输入！" << endl;
        }
        cout << endl;
    } while (choice != 0);
}