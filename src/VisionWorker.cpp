#include "VisionWorker.h"
#include <QImage>

void VisionWorker::process() {
    while (working) {
        img = static_cast<Pix *>(shmemReader->read());
        updPreview(pixScaleToSize(img, 320, 180));
        pixDestroy(&img);
        //printf("work\n");
        Sleep(100);
    }
    finished();
}

VisionWorker::VisionWorker(int capture_mode) {
    this->img = nullptr;
    this->working = true;
    this->capture_mode = capture_mode;
    this->shmemReader = new SHMEMReader;
    this->screenShooter = new ScreenShooter;
    printf("init\n");
}

VisionWorker::~VisionWorker() {
    delete this->shmemReader;
    delete this->screenShooter;
    pixDestroy(&img);
}