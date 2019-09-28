#include "mainwindow.h"
#include "VisionWorker.h"

#include <iostream>
#include <QMainWindow>
#include <QThread>

MainWindow::MainWindow(QWidget *parent) :QMainWindow(parent),ui(new Ui::MainWindow) {
    ui->setupUi(this);
    graphicsScene = new QGraphicsScene;
    ui->graphicsViewPreview->setScene(graphicsScene);
    image = QImage(320, 180, QImage::Format_RGB32);
    imgData = image.bits();
    pixmap = graphicsScene->addPixmap(QPixmap::fromImage(image));
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_pushButtonSHMEM_clicked() {
    worker = new VisionWorker(ui->checkBoxOBS->checkState() == Qt::Checked ? 1 : 0);
    thread = new QThread;
    worker->moveToThread(thread);
    connect(thread, SIGNAL(started()), worker, SLOT(process()));
    connect(worker, SIGNAL(updPreview(Pix * )), this, SLOT(updImage(Pix * )));
    connect(worker, SIGNAL(updTank(int)), this, SLOT(updTnk(int)));
    connect(worker, SIGNAL(updDPS(int)), this, SLOT(updDmg(int)));
    connect(worker, SIGNAL(updSupport(int)), this, SLOT(updSup(int)));
    connect(worker, SIGNAL (finished()), thread, SLOT (quit()));
    connect(worker, SIGNAL (finished()), worker, SLOT (deleteLater()));
    connect(thread, SIGNAL (finished()), thread, SLOT (deleteLater()));
    connect(worker, SIGNAL(finished()), this, SLOT(workerDeath()));
    thread->start();
}

void MainWindow::on_pushButtonDest_clicked() {
    worker->working=false;
}


void MainWindow::updImage(Pix *pix) {
    auto swpPx = pixEndianByteSwapNew(pix);
    memmove(imgData, pixGetData(swpPx), pixGetWidth(pix) * pixGetHeight(pix) * pixGetDepth(pix)/8);
    pixmap->setPixmap(QPixmap::fromImage(image.rgbSwapped()));
    pixDestroy(&swpPx);
    pixDestroy(&pix);
}

void MainWindow::workerDeath() {
}

void MainWindow::updTnk(int sr) {
    ui->labelTankSRVal->setText(QString::number(sr));
}

void MainWindow::updDmg(int sr) {
    ui->labelDPSSRVal->setText(QString::number(sr));
}

void MainWindow::updSup(int sr) {
    ui->labelSupportSRVal->setText(QString::number(sr));
}
