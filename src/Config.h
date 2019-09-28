//
// Created by Evgeny on 28-Sep-19.
//

#ifndef OWSTREAMRECORDEX_CONFIG_H
#define OWSTREAMRECORDEX_CONFIG_H

#include <nlohmann/json.hpp>

using namespace nlohmann;

class Config {
private:
    json conf;
public:
    void saveConf();
    void saveDefaultConf();
    void loadConf();
    json getConf();

};


#endif //OWSTREAMRECORDEX_CONFIG_H
