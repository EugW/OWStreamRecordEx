#include "VisionWorker.h"
#include <QImage>

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
    out.open("outfile.txt", std::ios::trunc | std::ios::out);
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
                    }
                    if (res != tnk.sr) {
                        if (res > tnk.sr) {
                            tnk.wins++;
                        } else if (res < tnk.sr) {
                            tnk.losses++;
                        }
                    }
                    //pixWritePng("tank.png", preTargetPix, 0.0);
                    updTank(res);
                    break;
                case 1:
                    if (dmg.sr == 0) {
                        dmg.sr = res;
                    }
                    if (res != dmg.sr) {
                        if (res > dmg.sr) {
                            dmg.wins++;
                        } else if (res < dmg.sr) {
                            dmg.losses++;
                        }
                    }
                    //pixWritePng("dps.png", preTargetPix, 0.0);
                    updDPS(res);
                    break;
                case 2:
                    if (sup.sr == 0) {
                        sup.sr = res;
                    }
                    if (res != sup.sr) {
                        if (res > sup.sr) {
                            sup.wins++;
                        } else if (res < sup.sr) {
                            sup.losses++;
                        }
                    }
                    //pixWritePng("support.png", preTargetPix, 0.0);
                    updSupport(res);
                    break;
                default:;
            }
            //updateStats
            pixDestroy(&targetPix);
        }
        pixDestroy(&preTargetPix);
    }
    out << "TANK: " << tnk.wins << "/" << tnk.losses << " SR: " << tnk.sr << "\n"
        << "DPS: " << dmg.wins << "/" << dmg.losses << " SR: " << dmg.sr << "\n"
        << "SUPPORT: " << sup.wins << "/" << sup.losses << " SR: " << sup.sr << "\n";
    out.close();
}

void VisionWorker::process() {
    int i = 0;
    while (working) {
        if (capture_mode == 1) {
            SHMEM2PIX();
        } else {
            SCR2PIX();
        }
        updPreview(pixScaleToSize(img, 320, 180));
        analyze();
        pixDestroy(&img);
        Sleep(100);
        if (i == 5) {
            working = false;
        }
        i++;
    }
    finished();
}

VisionWorker::VisionWorker(int capture_mode) {
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
    if (capture_mode == 1) {
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
        targetHWND = MFindWindow::getWindow();
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
        printf("SZ:%ld,%ld\n", bmpScreen.bmWidth, bmpScreen.bmHeight);
    }
    this->img = nullptr;
    this->working = true;
    this->capture_mode = capture_mode;
    printf("init\n");
}

VisionWorker::~VisionWorker() {
    if (capture_mode == 1) {
        UnmapViewOfFile(pBuf);
        CloseHandle(hMapFile);
    } else {
        DeleteObject(hbmScreen);
        DeleteObject(hdcMemDC);
        ReleaseDC(targetHWND,hdcWindow);
        free(scrData);
    }
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