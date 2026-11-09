#ifndef STATION_H
#define STATION_H

#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

struct Station {
    int station_id;
    string station_name;
    string line_name;
    int status;  // 0:关闭, 1:开启
    Station(int id = 0, string name = "", string line = "", int st = 0)
        : station_id(id), station_name(name), line_name(line), status(st) {
    }
};

class StationManager {
private:
    vector<Station> stations;
    unordered_map<int, int> id_to_index;
    unordered_map<string, vector<int>> name_to_indexs;
public:
    bool loadStationsFromCSV(const string& filename);
    bool checkStation(int id) const;
    string getStationName(int id) const;
    const vector<Station>& getAllStations() const { return stations; }

    // 判断某个站名是否为换乘站
    bool isTransferStation(const string& stationName) const;
};

#endif