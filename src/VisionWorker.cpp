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
    int j = 0;
    for (auto & i : boxes) {
        auto targetPix = pixClipRectangle(img, i, nullptr);
        api->SetImage(targetPix);
        printf("%s: %s",names[j] , api->GetUTF8Text());
        switch (j) {
            case 0: pixWritePng("tank.png", targetPix, 0.0);
            case 1: pixWritePng("dps.png", targetPix, 0.0);
            case 2: pixWritePng("support.png", targetPix, 0.0);
            default:;
        }
        j++;
    }
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
    auto tank = boxCreate(622, 594, 64, 38);
    auto dps = boxCreate(941, 594, 64, 38);
    auto support = boxCreate(1257, 594, 64, 38);
    boxes[0] = tank;
    boxes[1] = dps;
    boxes[2] = support;
    api = new tesseract::TessBaseAPI();
    api->Init(nullptr, "BigNoodleTooOblique");
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
    if (capture_mode == 1) {
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
    boxDestroy(&boxes[0]);
    boxDestroy(&boxes[1]);
    boxDestroy(&boxes[2]);
    api->End();
}