#ifndef OWSTREAMRECORDEX_VISIONWORKER_H
#define OWSTREAMRECORDEX_VISIONWORKER_H

#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <QObject>
#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>
#include "ConfigController.h"
#include "MDXGI.h"
#include "DXGI/CommonTypes.h"

class VisionWorker : public QObject {
    Q_OBJECT
private:
    static BOOL CALLBACK enumWindowsProc(__in HWND hWnd, __in LPARAM lParam);
    static void replace(std::string& str, const std::string& from, const std::string& to);
    ConfigController *ctrl;
    int width;
    int height;
    int mode;
    BITMAPFILEHEADER   bmfHeader{};
    BITMAPINFOHEADER   bi{};
    HANDLE hMapFile = nullptr;
    HWND targetHWND = nullptr;
    HDC hdcWindow = nullptr;
    HDC hdcMemDC = nullptr;
    HBITMAP hbmScreen = nullptr;
    BITMAP bmpScreen{};
    int size;
    uint32_t *pBuf = nullptr;
    Pix *img = nullptr;
    void DATA2PIX();
    void analyze();
    Box* srBoxes[3]{boxCreate(492, 542, 51, 36), boxCreate(772, 542, 51, 36), boxCreate(1052, 542, 51, 36)};
    tesseract::TessBaseAPI * numApi;
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
    explicit VisionWorker();
    ~VisionWorker() override;
    bool working;
    MPIC* mPic;
    BYTE* scrData = nullptr;

public slots:
    void process();

signals:
    void updPreview(Pix *pix);
    void updTank(int sr);
    void updDPS(int sr);
    void updSupport(int sr);
    void finished();
};


#endif //OWSTREAMRECORDEX_VISIONWORKER_H
