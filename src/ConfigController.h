#ifndef OWSTREAMRECORDEX_CONFIGCONTROLLER_H
#define OWSTREAMRECORDEX_CONFIGCONTROLLER_H

#include <string>
#include <Windows.h>
#include <ShlObj_core.h>
#include <Shlwapi.h>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace std;
using namespace nlohmann;

class ConfigController {
public:
    struct Config {
        int mode;
        string path;
        int delay;
        bool preview;
        string placeholder;
    };
    Config config;
    void loadConfig();
    void saveConfig();
    static ConfigController* getInstance();
private:
    static ConfigController* instance;
    ConfigController();
};


#endif //OWSTREAMRECORDEX_CONFIGCONTROLLER_H
