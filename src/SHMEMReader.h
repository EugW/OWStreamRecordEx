#ifndef OWSTREAMRECORDEX_SHMEMREADER_H
#define OWSTREAMRECORDEX_SHMEMREADER_H


#include <cstdint>
#include <leptonica/allheaders.h>
#include <Windows.h>

class SHMEMReader {
private:
    HANDLE hMapFile;
    uint32_t * pBuf;
    uint32_t * width;
    uint32_t * height;
    uint32_t size;
    uint8_t * data;
    BITMAPFILEHEADER   bmfHeader{};
    BITMAPINFOHEADER   bi{} ;
public:
    SHMEMReader();
    ~SHMEMReader();
    void * read();
};


#endif //OWSTREAMRECORDEX_SHMEMREADER_H
