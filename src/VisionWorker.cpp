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
    if (scrData == nullptr)
        return;
    memcpy(tmpBuf, &bmfHeader, sizeof(BITMAPFILEHEADER));
    memcpy(tmpBuf + sizeof(BITMAPFILEHEADER), &bi, sizeof(BITMAPINFOHEADER));
    memcpy(tmpBuf + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), scrData, size);
    img = pixReadMemBmp(tmpBuf, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + size);
}

void VisionWorker::analyze() {
    if (!img)
        return;
    out.open(ctrl->config.path, std::ios::trunc | std::ios::out);
    PIX* pixoq = pixClipRectangle(img, oqDetection, nullptr);
    numApi->SetImage(pixoq);
    char* test1 = numApi->GetUTF8Text();
    pixDestroy(&pixoq);
    numApi->Clear();
    char ref[33] = "NO ROLE LIMITS ON HERO SELECTION";
    if (memcmp(test1, &ref, 32) == 0) {
        //OPEN Q DETECTED
        PIX* oqq = pixClipRectangle(img, oqSR, nullptr);
        //pixWritePng("oqq.png", oqq, 0.0);
        numApi->SetImage(oqq);
        char* txt = numApi->GetUTF8Text();
        pixDestroy(&oqq);
        numApi->Clear();
        int res = std::atoi(txt);
        updOpenQ(res);
        if (oq.sr == 0) {
            oq.sr = res;
        }
        if (res != oq.sr) {
            if (res > oq.sr) {
                oq.wins++;
            }
            else {
                oq.losses++;
            }
        }
    }
    else {
        //OPEN Q NOT DETECTED
        PIX* pixrq = pixClipRectangle(img, rqDetection, nullptr);
        numApi->SetImage(pixrq);
        test1 = numApi->GetUTF8Text();
        pixDestroy(&pixrq);
        numApi->Clear();
        char ref2[12] = "ROLE SELECT";
        if (memcmp(test1, &ref2, 11) == 0) {
            //ROLE Q DETECTED
            for (int i = 0; i < 3; i++) {
                PIX* targetPix = pixClipRectangle(img, srBoxes[i], nullptr);
                numApi->SetImage(targetPix);
                char* txt = numApi->GetUTF8Text();
                pixDestroy(&targetPix);
                numApi->Clear();
                int res = std::atoi(txt);
                switch (i) {
                case 0: {
                    updTank(res);
                    if (tnk.sr == 0) {
                        tnk.sr = res;
                    }
                    if (res != tnk.sr) {
                        if (res > tnk.sr) {
                            tnk.wins++;
                        }
                        else {
                            tnk.losses++;
                        }
                    }
                    break;
                }
                case 1: {
                    updDPS(res);
                    if (dmg.sr == 0) {
                        dmg.sr = res;
                    }
                    if (res != dmg.sr) {
                        if (res > dmg.sr) {
                            dmg.wins++;
                        }
                        else {
                            dmg.losses++;
                        }
                    }
                    break;
                }
                case 2: {
                    updSupport(res);
                    if (sup.sr == 0) {
                        sup.sr = res;
                    }
                    if (res != sup.sr) {
                        if (res > sup.sr) {
                            sup.wins++;
                        }
                        else {
                            sup.losses++;
                        }
                    }
                    break;
                }
                }
            }
        }
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
    replace(str, "%oW%", to_string(oq.wins));
    replace(str, "%oL%", to_string(oq.losses));
    replace(str, "%oSR%", to_string(oq.sr));
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
    while (true) {
        //if (targetHWND != GetForegroundWindow()) {
        //    continue;
        //}
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
            scrData = mPic->data;
            DATA2PIX();
            mPic->wait = false;
            break;
        }
        }
        if (ctrl->config.preview && img)
            updPreview(pixScaleToSize(img, 320, 180));
        analyze();
        pixDestroy(&img);
        Sleep(ctrl->config.delay);
    }
}

VisionWorker::VisionWorker() {
    ctrl = ConfigController::getInstance();
    mode = ctrl->config.mode;
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
        scrData = &pBuf[2];
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
        scrData = malloc(size);
        break;
    }
    case 2: {
        EnumWindows(enumWindowsProc, (LPARAM)this);
        if (targetHWND == nullptr) {
            printf("Could not find window.\n");
            return;
        }
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
        mPic->data = nullptr;
        mPic->height = 0;
        mPic->wait = true;
        mPic->width = 0;
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)startDup, (LPVOID)mPic, 0, nullptr);
        break;
    }
    }
    tmpBuf = (uint8_t*)malloc(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + size);
    if (tmpBuf == nullptr) {
        printf("Couldn't alloc memory (TMPBUF)\n");
        return;
    }
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
    if (tmpBuf != nullptr)
        free(tmpBuf);
    boxDestroy(&srBoxes[0]);
    boxDestroy(&srBoxes[1]);
    boxDestroy(&srBoxes[2]);
    boxDestroy(&oqDetection);
    boxDestroy(&oqSR);
    numApi->End();
}