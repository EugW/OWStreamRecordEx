//
// Created by Evgeny on 8/17/2019.
//

#ifndef OWSTREAMRECORDEX_VISIONWORKER_H
#define OWSTREAMRECORDEX_VISIONWORKER_H

#include "SHMEMReader.h"
#include "ScreenShooter.h"
#include <QObject>

class VisionWorker : public QObject {
    Q_OBJECT
private:
    int capture_mode;
    SHMEMReader *shmemReader;
    ScreenShooter *screenShooter;
    Pix *img;
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
