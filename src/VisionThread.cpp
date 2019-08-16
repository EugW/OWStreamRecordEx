#include "VisionThread.h"
#include <iostream>

void VisionThread::updatePreview(QImage img) {

}

void VisionThread::ThreadFunc() {

    while (running) {
        std::cout<<"Thread running"<<std::endl;
        Sleep(500);
    }
    std::cout<<"Thread stopped"<<std::endl;
}

bool VisionThread::start() {
    running = true;
    thread = new std::thread(&VisionThread::ThreadFunc, this);
    if (thread) {
        std::cout<<"Thread started"<<std::endl;
        return true;
    } else {
        std::cout<<"Thread NOT started"<<std::endl;
        thread = nullptr;
        return false;
    }
}

bool VisionThread::stop() {
    running = false;
    std::cout<<"Thread stop signal sent"<<std::endl;
    return true;
}
