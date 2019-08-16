#ifndef OWSTREAMRECORDEX_SHMEMREADER_H
#define OWSTREAMRECORDEX_SHMEMREADER_H


#include <cstdint>

class SHMEMReader {
private:
    bool firstRun;
public:
    SHMEMReader();
    uint8_t *data{};
    void *read();
};


#endif //OWSTREAMRECORDEX_SHMEMREADER_H
