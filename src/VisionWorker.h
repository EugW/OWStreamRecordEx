//
// Created by Evgeny on 8/17/2019.
//

#ifndef OWSTREAMRECORDEX_VISIONWORKER_H
#define OWSTREAMRECORDEX_VISIONWORKER_H

#include "MFindWindow.h"
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
    Box* boxes[3]{};
    tesseract::TessBaseAPI * api;
    const char * names[3]{"tank", "dps", "support"};
public:
    explicit VisionWorker(int capture_mode);
    ~VisionWorker() override;
    bool working;

public slots:
    void process();

signals:
    void updPreview(Pix *pix);
    void finished();
};


#endif //OWSTREAMRECORDEX_VISIONWORKER_H
