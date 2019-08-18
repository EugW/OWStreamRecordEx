#include "ScreenShooter.h"
#include <cstdio>
#include <iostream>
#include <vector>
#include <Psapi.h>

HWND targetHWND;

BOOL CALLBACK ScreenShooter::enumWindowsProc(__in HWND hWnd, __in LPARAM lParam) {
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
        targetHWND = hWnd;
        return false;
    }
    //std::cout << Title: " << windowTitle << " |  Filename: " << fileName << std::endl;
    return true;
}

void* ScreenShooter::take() {
    std::cout << EnumWindows(enumWindowsProc, NULL) << std::endl;
    if (targetHWND == nullptr)
        return nullptr;
    HDC hdcWindow;
    HDC hdcMemDC = nullptr;
    HBITMAP hbmScreen = nullptr;
    BITMAP bmpScreen;
    hdcWindow = GetDC(targetHWND);
    hdcMemDC = CreateCompatibleDC(hdcWindow);
    RECT rcClient;
    GetClientRect(targetHWND, &rcClient);
    hbmScreen = CreateCompatibleBitmap(hdcWindow, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top);
    SelectObject(hdcMemDC,hbmScreen);
    BitBlt(hdcMemDC,
           0,0,
           rcClient.right-rcClient.left, rcClient.bottom-rcClient.top,
           hdcWindow,
           0,0,
           SRCCOPY);
    GetObject(hbmScreen,sizeof(BITMAP),&bmpScreen);
    BITMAPFILEHEADER   bmfHeader;
    BITMAPINFOHEADER   bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmpScreen.bmWidth;
    bi.biHeight = bmpScreen.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;
    DWORD dwBmpSize = bi.biWidth * bi.biHeight * 3;
    if (firstRun) {
        data = (uint8_t *) malloc(dwBmpSize);
        firstRun = false;
    }
    GetDIBits(hdcWindow, hbmScreen, 0,
              bmpScreen.bmHeight,
              data,
              reinterpret_cast<LPBITMAPINFO>(&bi), DIB_RGB_COLORS);
    bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
    bmfHeader.bfType = 0x4D42;


    HANDLE hFile = CreateFile("losslessSCR.bmp",
                              GENERIC_WRITE,
                              0,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD dwBytesWritten = 0;
    WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, data, dwBmpSize, &dwBytesWritten, NULL);
    CloseHandle(hFile);
    std::vector<unsigned char> buffer(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize);
    std::copy(reinterpret_cast<unsigned char*>(&bmfHeader), reinterpret_cast<unsigned char*>(&bmfHeader) + sizeof(BITMAPFILEHEADER), buffer.begin());
    std::copy(reinterpret_cast<unsigned char*>(&bi), reinterpret_cast<unsigned char*>(&bi) + sizeof(BITMAPINFOHEADER), buffer.begin() + sizeof(BITMAPFILEHEADER));
    std::copy(data, data + dwBmpSize, buffer.begin() + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
    //free(lpbitmap);
    DeleteObject(hbmScreen);
    DeleteObject(hdcMemDC);
    ReleaseDC(targetHWND,hdcWindow);
    pixWritePng("SCR.png", pixReadMemBmp(&buffer[0], buffer.size()), 0);
    //return ;
}

ScreenShooter::ScreenShooter() = default;

