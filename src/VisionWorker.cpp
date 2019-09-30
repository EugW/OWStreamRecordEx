#include "VisionWorker.h"
#include <QImage>
#include <Psapi.h>

BOOL CALLBACK VisionWorker::enumWindowsProc(__in HWND hWnd, __in LPARAM lParam) {
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
        reinterpret_cast<VisionWorker *>(lParam)->targetHWND = hWnd;
        return false;
    }
    return true;
}

void VisionWorker::SHMEM2PIX() {
    if (img)
        pixDestroy(&img);
    auto tmp = (uint8_t *)malloc(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + size);
    memcpy(tmp, &bmfHeader, sizeof(BITMAPFILEHEADER));
    memcpy(&tmp[sizeof(BITMAPFILEHEADER)], &bi, sizeof(BITMAPINFOHEADER));
    memcpy(&tmp[sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)], &pBuf[2], size);
    img = pixReadMemBmp(tmp, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + size);
    free(tmp);
}

void VisionWorker::SCR2PIX() {
    if (img)
        pixDestroy(&img);
    auto tmp = (uint8_t *)malloc(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + size);
    BitBlt(hdcMemDC,0,0,width, height,hdcWindow,0,0,SRCCOPY);
    GetDIBits(hdcWindow, hbmScreen, 0, bmpScreen.bmHeight, scrData, reinterpret_cast<LPBITMAPINFO>(&bi), DIB_RGB_COLORS);
    memcpy(tmp, &bmfHeader, sizeof(BITMAPFILEHEADER));
    memcpy(&tmp[sizeof(BITMAPFILEHEADER)], &bi, sizeof(BITMAPINFOHEADER));
    memcpy(&tmp[sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)], scrData, size);
    img = pixReadMemBmp(tmp, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + size);
    free(tmp);
}

void VisionWorker::analyze() {
    out.open(ctrl->config.path, std::ios::trunc | std::ios::out);
    for (int i = 0; i < 3; i++) {
        auto preTargetPix = pixClipRectangle(img, roleBoxes[i], nullptr);
        textApi->SetImage(preTargetPix);
        std::string roleNM = ((std::string)textApi->GetUTF8Text()).substr(0, names[i].size());
        std::transform(roleNM.begin(), roleNM.end(), roleNM.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        if (roleNM == names[i]) {
            auto targetPix = pixClipRectangle(img, srBoxes[i], nullptr);
            numApi->SetImage(targetPix);
            int res = std::stoi(numApi->GetUTF8Text());
            //
            //printf("%s: %d\n", names[i].c_str(), res);
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
                    //pixWritePng("tank.png", preTargetPix, 0.0);
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
                    //pixWritePng("dps.png", preTargetPix, 0.0);
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
                    //pixWritePng("support.png", preTargetPix, 0.0);
                    break;
                default:;
            }
            pixDestroy(&targetPix);
        }
        pixDestroy(&preTargetPix);
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
    int i = 0;
    while (working) {
        if (obs_mode) {
            SHMEM2PIX();
        } else {
            SCR2PIX();
        }
        if (ctrl->config.preview)
            updPreview(pixScaleToSize(img, 320, 180));
        analyze();
        pixDestroy(&img);
        Sleep(ctrl->config.delay);
        i++;
    }
    finished();
}

VisionWorker::VisionWorker() {
    ctrl = ConfigController::getInstance();
    obs_mode = ctrl->config.obs;
    working = false;
    numApi = new tesseract::TessBaseAPI();
    numApi->Init(nullptr, "BigNoodleTooOblique");
    textApi = new tesseract::TessBaseAPI();
    textApi->Init(nullptr, "eng");
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
    if (obs_mode) {
        hMapFile = OpenFileMapping(FILE_MAP_READ, FALSE, "OWStreamRecordExRec:SHMEM");
        if (hMapFile == nullptr) {
            printf("Could not open file mapping object.\n");
            return;
        }
        pBuf = (uint32_t *)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
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
    } else {
        EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(this));
        if (targetHWND == nullptr) {
            printf("Could not find window.\n");
            return;
        }
        hdcWindow = GetDC(targetHWND);
        hdcMemDC = CreateCompatibleDC(hdcWindow);
        RECT rcClient;
        GetClientRect(targetHWND, &rcClient);
        hbmScreen = CreateCompatibleBitmap(hdcWindow, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top);
        SelectObject(hdcMemDC,hbmScreen);
        GetObject(hbmScreen,sizeof(BITMAP),&bmpScreen);
        width = bmpScreen.bmWidth;
        height = bmpScreen.bmHeight;
        bi.biWidth = width;
        bi.biHeight = height;
        size = width * height * 3;
        scrData = (uint32_t *)malloc(size);
        //printf("SZ:%ld,%ld\n", bmpScreen.bmWidth, bmpScreen.bmHeight);
    }
    working = true;
    //printf("init\n");
}

VisionWorker::~VisionWorker() {
    if (obs_mode) {
        if (pBuf != nullptr)
            UnmapViewOfFile(pBuf);
        if (hMapFile != nullptr)
            CloseHandle(hMapFile);
    } else {
        if (hbmScreen != nullptr)
            DeleteObject(hbmScreen);
        if (hdcMemDC != nullptr)
            DeleteObject(hdcMemDC);
        if (targetHWND != nullptr && hdcWindow != nullptr)
            ReleaseDC(targetHWND,hdcWindow);
        if (scrData != nullptr)
            free(scrData);
    }
    if (img != nullptr)
        pixDestroy(&img);
    boxDestroy(&srBoxes[0]);
    boxDestroy(&srBoxes[1]);
    boxDestroy(&srBoxes[2]);
    boxDestroy(&roleBoxes[0]);
    boxDestroy(&roleBoxes[1]);
    boxDestroy(&roleBoxes[2]);
    numApi->End();
    textApi->End();
}