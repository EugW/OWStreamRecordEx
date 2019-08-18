#include "SHMEMReader.h"
#include <Windows.h>
#include <cstdio>
#include <vector>

void * SHMEMReader::read() {
    memcpy(data, &bmfHeader, sizeof(bmfHeader));
    memcpy(&data[sizeof(bmfHeader)], &bi, sizeof(bi));
    memcpy(&data[sizeof(bmfHeader) + sizeof(bi)], &pBuf[2], size);
    //printf("b:%u|g:%u|r:%u\n", data[0], data[1], data[2]);
    return pixReadMemBmp(data, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + size);
}

SHMEMReader::SHMEMReader() {
    hMapFile = OpenFileMapping(FILE_MAP_READ, FALSE, "OWStreamRecordExRec:SHMEM");
    if (hMapFile == nullptr) {
        printf(TEXT("Could not open file mapping object (%d).\n"), GetLastError());
        return;
    }
    pBuf = (uint32_t *)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
    if (pBuf == nullptr) {
        printf(TEXT("Could not map view of file (%d).\n"), GetLastError());
        CloseHandle(hMapFile);
        return;
    }
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHeader.bfType = 0x4D42;
    width = &pBuf[0];
    height = &pBuf[1];
    bi.biWidth = *width;
    bi.biHeight = -*height;
    size = *width * *height * 3;
    data = (uint8_t *) malloc(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + size);
}

SHMEMReader::~SHMEMReader() {
    delete data;
}
