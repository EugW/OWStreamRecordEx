#include "MFindWindow.h"
#include <string>
#include <Psapi.h>

HWND tHWND;

BOOL CALLBACK MFindWindow::enumWindowsProc(__in HWND hWnd, __in LPARAM lParam) {
    int length = GetWindowTextLength(hWnd);
    if (length == 0)
        return true;
    auto buffer = new TCHAR[512];
    memset( buffer, 0, ( 512 ) * sizeof( TCHAR ) );
    GetWindowText( hWnd, buffer, 512 );
    auto windowTitle = std::string(buffer);
    DWORD proc = NULL;
    GetWindowThreadProcessId(hWnd, &proc);
    GetModuleFileNameEx(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, proc), nullptr, buffer, 512);
    auto filePath = std::string(buffer);
    auto pos = filePath.find_last_of('\\');
    auto fileName = filePath.substr(pos + 1);
    delete[] buffer;
    if (windowTitle == "Overwatch" && fileName == "Overwatch.exe") {
        tHWND = hWnd;
        return false;
    }
    return true;
}

HWND MFindWindow::getWindow() {
    EnumWindows(enumWindowsProc, NULL);
    return tHWND;
}