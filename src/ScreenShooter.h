#ifndef OWSTREAMRECORDEX_SCREENSHOOTER_H
#define OWSTREAMRECORDEX_SCREENSHOOTER_H

#include <Windows.h>
#include <leptonica/allheaders.h>
#include <cstdint>

class ScreenShooter {
private:
    static BOOL CALLBACK enumWindowsProc(__in HWND hWnd, __in LPARAM lParam);
public:
    ScreenShooter();
    uint8_t *data{};
    void* take();
};

#endif //OWSTREAMRECORDEX_SCREENSHOOTER_H
