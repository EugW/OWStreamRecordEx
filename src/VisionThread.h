#ifndef OWSTREAMRECORDEX_VISIONTHREAD_H
#define OWSTREAMRECORDEX_VISIONTHREAD_H

#include "mainwindow.h"
#include <Windows.h>
#include <thread>

class VisionThread {

private:
    bool running;
    int capture_type;
    std::thread* thread{};
    void ThreadFunc();
public:
    VisionThread() = default;
    explicit VisionThread(int capture_type, QMainWindow ui) {
        this->capture_type = capture_type;
    };
    bool start();
    bool stop();
signals:
        void updatePreview(QImage img);
};


#endif //OWSTREAMRECORDEX_VISIONTHREAD_H
