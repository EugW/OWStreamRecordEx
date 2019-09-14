#ifndef OWSTREAMRECORDEX_MFINDWINDOW_H
#define OWSTREAMRECORDEX_MFINDWINDOW_H

#include <Windows.h>

class MFindWindow {
private:
    static BOOL CALLBACK enumWindowsProc(__in HWND hWnd, __in LPARAM lParam);
public:
    static HWND getWindow();
};


#endif //OWSTREAMRECORDEX_MFINDWINDOW_H
