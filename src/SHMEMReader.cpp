#include "SHMEMReader.h"
#include <Windows.h>
#include <cstdio>
#include <iostream>
#include <leptonica/allheaders.h>
#include <vector>

void convertRGBA2BGR(const uint8_t *src, uint8_t  *dest, int w, int h) {
    for (int i = 0; i < w * h * 4; i+=4) {
        dest[i-i/4] = src[i+2];
        dest[i+1-i/4] = src[i+1];
        dest[i+2-i/4] = src[i];
        //data[i+3] = src[i+3];
        if (i == 0) {
            printf("d:%u|s:%u", dest[i], src[i+2]);
        }
    }
}

void *SHMEMReader::read() {
    HANDLE hMapFile;
    hMapFile = OpenFileMapping(FILE_MAP_READ, FALSE, "OWStreamRecordExRec:SHMEM");
    if (hMapFile == NULL) {
        printf(TEXT("Could not open file mapping object (%d).\n"), GetLastError());
        return nullptr;
    }
    auto pBuf = (uint32_t *) (MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0));
    if (pBuf == NULL) {
        printf(TEXT("Could not map view of file (%d).\n"), GetLastError());
        CloseHandle(hMapFile);
        return nullptr;
    }
    auto w = pBuf[0];
    auto h = pBuf[1];
    if (firstRun) {
        data = (uint8_t *) malloc(w * h *3 );
        firstRun = false;
    }
    auto ls = pBuf[2];
    auto i = pBuf[3];
    std::cout << "W:" << w << std::endl << "H:" << h << std::endl << "LS:" << ls << std::endl << "I:" << i << std::endl
              << "SZ:" << " sizeof(data)" << std::endl;
    BITMAPFILEHEADER   bmfHeader;
    BITMAPINFOHEADER   bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = w;
    bi.biHeight = -h;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
    DWORD dwBmpSize = (w * bi.biBitCount + 31) /32 * h * 4;
    bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
    bmfHeader.bfType = 0x4D42; //BM
    HANDLE hFile = CreateFile("lossless.bmp",
                              GENERIC_WRITE,
                              0,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD dwBytesWritten = 0;
    convertRGBA2BGR(reinterpret_cast<uint8_t *>(&pBuf[4]), data, w, h);
    std::cout<<std::endl;
    printf("b:%u|g:%u|r:%u", data[0], data[1], data[2]);
    std::cout<<std::endl;
    WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, data, dwBmpSize, &dwBytesWritten, NULL);
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    CloseHandle(hFile);
    std::vector<unsigned char> buffer(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize);
    std::copy(reinterpret_cast<unsigned char*>(&bmfHeader), reinterpret_cast<unsigned char*>(&bmfHeader) + sizeof(BITMAPFILEHEADER), buffer.begin());
    std::copy(reinterpret_cast<unsigned char*>(&bi), reinterpret_cast<unsigned char*>(&bi) + sizeof(BITMAPINFOHEADER), buffer.begin() + sizeof(BITMAPFILEHEADER));
    std::copy(data, data + dwBmpSize, buffer.begin() + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
    //free(dt);
    auto pic = pixReadMemBmp(&buffer[0], buffer.size());
    buffer.clear();
    buffer.shrink_to_fit();
    return pic;
}

SHMEMReader::SHMEMReader() {
    firstRun = true;
}
