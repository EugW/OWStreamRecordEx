//
// Created by Evgeny on 8/17/2019.
//

#ifndef OWSTREAMRECORDEX_VISIONWORKER_H
#define OWSTREAMRECORDEX_VISIONWORKER_H

#include "MFindWindow.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <QObject>
#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>

class VisionWorker : public QObject {
    Q_OBJECT
private:
    int width;
    int height;
    int capture_mode;
    BITMAPFILEHEADER   bmfHeader{};
    BITMAPINFOHEADER   bi{};
    HANDLE hMapFile;
    HWND targetHWND;
    HDC hdcWindow;
    HDC hdcMemDC;
    HBITMAP hbmScreen;
    BITMAP bmpScreen{};
    int size;
    uint32_t * pBuf;
    uint32_t * scrData;
    Pix *img;
    void SHMEM2PIX();
    void SCR2PIX();
    void analyze();
    Box* srBoxes[3]{boxCreate(622, 594, 64, 38), boxCreate(941, 594, 64, 38), boxCreate(1257, 594, 64, 38)};
    Box* roleBoxes[3]{boxCreate(596, 510, 93, 40), boxCreate(885, 510, 149, 40), boxCreate(1204, 510, 147, 40)};
    tesseract::TessBaseAPI * numApi;
    tesseract::TessBaseAPI * textApi;
    std::string names[3]{"tank", "damage", "support"};
    struct stats {
        int sr;
        int wins;
        int losses;
    };
    stats tnk{0, 0, 0};
    stats dmg{0, 0, 0};
    stats sup{0, 0, 0};
    std::ofstream out;
public:
    explicit VisionWorker(int capture_mode);
    ~VisionWorker() override;
    bool working;

public slots:
    void process();

signals:
    //void updLog();
    void updPreview(Pix *pix);
    void updTank(int sr);
    void updDPS(int sr);
    void updSupport(int sr);
    void finished();
};


#endif //OWSTREAMRECORDEX_VISIONWORKER_H
