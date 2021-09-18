#include "VisionWorker.h"
#include <QImage>
#include <Psapi.h>

BOOL CALLBACK VisionWorker::enumWindowsProc(__in HWND hWnd, __in LPARAM lParam) {
    int length = GetWindowTextLength(hWnd);
    if (length == 0)
        return true;
    TCHAR buffer[512];
    memset( buffer, 0, ( 512 ) * sizeof( TCHAR ) );
    GetWindowText( hWnd, buffer, 512 );
    auto windowTitleTmp = std::wstring(buffer);
    auto windowTitle = std::string(windowTitleTmp.begin(), windowTitleTmp.end());
    DWORD proc = NULL;
    GetWindowThreadProcessId(hWnd, &proc);
    GetModuleFileNameEx(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, proc), nullptr, buffer, 512);
    auto filePathTmp = std::wstring(buffer);
    auto filePath = std::string(filePathTmp.begin(), filePathTmp.end());
    auto pos = filePath.find_last_of('\\');
    auto fileName = filePath.substr(pos + 1);
    if (windowTitle == "Overwatch" && fileName == "Overwatch.exe") {
        ((VisionWorker*)lParam)->targetHWND = hWnd;
        return false;
    }
    return true;
}

void VisionWorker::DATA2PIX() {
    if (img)
        pixDestroy(&img);
    size_t mlcSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + size;
    auto tmp = (uint8_t*)malloc(mlcSize);
    if (tmp == nullptr || mlcSize < 14) {
        printf("Couldn't alloc memory (DATA2PIX)");
        return;
    }
    memcpy(tmp, &bmfHeader, sizeof(BITMAPFILEHEADER));
    memcpy(tmp + sizeof(BITMAPFILEHEADER), &bi, sizeof(BITMAPINFOHEADER));
    memcpy(tmp + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), scrData, size);
    img = pixReadMemBmp(tmp, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + size);
    free(tmp);
}

void VisionWorker::analyze() {
    out.open(ctrl->config.path, std::ios::trunc | std::ios::out);
    for (int i = 0; i < 3; i++) {
        auto targetPix = pixClipRectangle(img, srBoxes[i], nullptr);
        /*if (i == 0)
            pixWritePng("tank.png", targetPix, 0.0);
        if (i == 1)
            pixWritePng("dps.png", targetPix, 0.0);
        if (i == 2)
            pixWritePng("support.png", targetPix, 0.0);*/
            numApi->SetImage(targetPix);
            char* lll = numApi->GetUTF8Text();
            int res = 0;
            bool nan = false;
            numApi->Clear();
            for (int j = 0; j < 4; j++) {
                if (lll[j] >= '0' && lll[j] <= '9') {
                    res += pow(10, 3 - j) * ((long long)lll[j] - '0');
                }
                else {
                    nan = true;
                    break;
                }
            }
            if (nan)
                continue;
            free(lll);
        switch (i) {
            case 0:
                if (tnk.sr == 0) {
                    tnk.sr = res;
                    updTank(res);
                }
                if (res != tnk.sr) {
                    updTank(res);
                    if (res > tnk.sr) {
                        tnk.wins++;
                    } else if (res < tnk.sr) {
                        tnk.losses++;
                    }
                }
                break;
            case 1:
                if (dmg.sr == 0) {
                    dmg.sr = res;
                    updDPS(res);
                }
                if (res != dmg.sr) {
                    updDPS(res);
                    if (res > dmg.sr) {
                        dmg.wins++;
                    } else if (res < dmg.sr) {
                        dmg.losses++;
                    }
                }
                break;
            case 2:
                if (sup.sr == 0) {
                    sup.sr = res;
                    updSupport(res);
                }
                if (res != sup.sr) {
                    updSupport(res);
                    if (res > sup.sr) {
                        sup.wins++;
                    } else if (res < sup.sr) {
                        sup.losses++;
                    }
                }
                break;
            default:;
        }
        pixDestroy(&targetPix);
    }
    auto str = ctrl->config.placeholder;
    replace(str, "%tW%", to_string(tnk.wins));
    replace(str, "%tL%", to_string(tnk.losses));
    replace(str, "%tSR%", to_string(tnk.sr));
    replace(str, "%dW%", to_string(dmg.wins));
    replace(str, "%dL%", to_string(dmg.losses));
    replace(str, "%dSR%", to_string(dmg.sr));
    replace(str, "%sW%", to_string(sup.wins));
    replace(str, "%sL%", to_string(sup.losses));
    replace(str, "%sSR%", to_string(sup.sr));
    out << str;
    out.close();
}

void VisionWorker::replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return;
    str.replace(start_pos, from.length(), to);
}

void VisionWorker::process() {
    while (working) {
        switch (mode) {
        case 0: {
            DATA2PIX();
            break;
        }
        case 1: {
            BitBlt(hdcMemDC, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY);
            GetDIBits(hdcWindow, hbmScreen, 0, bmpScreen.bmHeight, scrData, reinterpret_cast<LPBITMAPINFO>(&bi), DIB_RGB_COLORS);
            DATA2PIX();
            break;
        }
        case 2: {
            while (mPic->startupWait) {
                Sleep(0);
            }
            scrData = (BYTE*)mPic->rsc.pData;
            DATA2PIX();
            mPic->wait = false;
            break;
        }
        }
        if (ctrl->config.preview)
            updPreview(pixScaleToSize(img, 320, 180));
        analyze();
        pixDestroy(&img);
        Sleep(ctrl->config.delay);
    }
    finished();
}

VisionWorker::VisionWorker() {
    ctrl = ConfigController::getInstance();
    mode = ctrl->config.mode;
    working = false;
    numApi = new tesseract::TessBaseAPI();
    numApi->Init(nullptr, "eng");
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 70 * 39.37;
    bi.biYPelsPerMeter = 70 * 39.37;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHeader.bfType = 0x4D42;
    switch (mode) {
    case 0: {
        hMapFile = OpenFileMapping(FILE_MAP_READ, FALSE, L"OWStreamRecordExRec:SHMEM");
        if (hMapFile == nullptr) {
            printf("Could not open file mapping object.\n");
            return;
        }
        pBuf = (uint32_t*)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
        if (pBuf == nullptr) {
            printf("Could not map view of file.\n");
            CloseHandle(hMapFile);
            return;
        }
        width = pBuf[0];
        height = pBuf[1];
        bi.biWidth = width;
        bi.biHeight = -height;
        size = width * height * 3;
        scrData = (BYTE*)pBuf + 2;
        break;
    }
    case 1: {
        EnumWindows(enumWindowsProc, (LPARAM)this);
        if (targetHWND == nullptr) {
            printf("Could not find window.\n");
            return;
        }
        hdcWindow = GetDC(targetHWND);
        hdcMemDC = CreateCompatibleDC(hdcWindow);
        RECT rcClient;
        GetClientRect(targetHWND, &rcClient);
        hbmScreen = CreateCompatibleBitmap(hdcWindow, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
        SelectObject(hdcMemDC, hbmScreen);
        GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);
        width = bmpScreen.bmWidth;
        height = bmpScreen.bmHeight;
        bi.biWidth = width;
        bi.biHeight = height;
        size = width * height * 3;
        scrData = (BYTE*)malloc(size);
        break;
    }
    case 2: {
        RECT desktop;
        const HWND hDesktop = GetDesktopWindow();
        GetWindowRect(hDesktop, &desktop);
        width = desktop.right;
        height = desktop.bottom;
        bi.biBitCount = 32;
        bi.biWidth = width;
        bi.biHeight = -height;
        size = width * height * 4;
        mPic = (MPIC*)malloc(sizeof(MPIC));
        if (mPic == nullptr) {
            printf("Couldn't alloc memory (INIT)\n");
            break;
        }
        mPic->startupWait = true;
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)startDup, (LPVOID)mPic, 0, nullptr);
        break;
    }
    }
    working = true;
}

VisionWorker::~VisionWorker() {
    switch (mode) {
    case 0: {
        if (pBuf != nullptr)
            UnmapViewOfFile(pBuf);
        if (hMapFile != nullptr)
            CloseHandle(hMapFile);
        break;
    }
    case 1: {
        if (hbmScreen != nullptr)
            DeleteObject(hbmScreen);
        if (hdcMemDC != nullptr)
            DeleteObject(hdcMemDC);
        if (targetHWND != nullptr && hdcWindow != nullptr)
            ReleaseDC(targetHWND, hdcWindow);
        if (scrData != nullptr)
            free(scrData);
        break;
    }
    case 2: {
        if (mPic != nullptr) 
            free(mPic);
        break;
    }
    }
    if (img)
        pixDestroy(&img);
    boxDestroy(&srBoxes[0]);
    boxDestroy(&srBoxes[1]);
    boxDestroy(&srBoxes[2]);
    numApi->End();
}