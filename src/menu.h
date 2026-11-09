#ifndef MENU_H
#define MENU_H

#include "graph.h"
#include "station.h"

// 显示主菜单
void showMainMenu(Graph* graph, StationManager* sm);

// 时间最优路径二级菜单
void showTimeMenu(Graph* graph, StationManager* sm);

// 换乘最优路径二级菜单
void showTransferMenu(Graph* graph, StationManager* sm);

// 显示"线路站点信息/运营状态管理"二级菜单（组员D负责）
void showStationManageMenu(StationManager* sm);  


#endif

