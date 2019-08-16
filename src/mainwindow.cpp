#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tesseract/baseapi.h"
#include "leptonica/allheaders.h"

#include "ScreenShooter.h"
#include "SHMEMReader.h"
#include "VisionThread.h"

#include <iostream>
#include <QMainWindow>

VisionThread th;
SHMEMReader shmemReader;

MainWindow::MainWindow(QWidget *parent) :QMainWindow(parent),ui(new Ui::MainWindow) {
    ui->setupUi(this);
    shmemReader = SHMEMReader();
    auto img = new QImage();
    //img->fromData()
    //QGraphicsPixmapItem *item = new QGraphicsPixmapItem(QPixmap::fromImage())
    //graphicsViewPreview->scene()->addItem();
    //th = VisionThread(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton1_clicked()
{
    auto *api = new tesseract::TessBaseAPI();
    if (api->Init(NULL, "eng")) {
            fprintf(stderr, "Could not initialize tesseract.\n");
            exit(1);
        }
        // Open input image with leptonica library
        Pix *image = pixRead("test.png");
        api->SetImage(image);
        // Get OCR result
        char *outText;
        outText = api->GetUTF8Text();
        std::cout<<outText<<std::endl;
        //ui->label1->setText(outText);
        // Destroy used object and release memory
        api->End();
        delete [] outText;
        pixDestroy(&image);
}

void MainWindow::on_pushButton2_clicked() {
    //0 - GDI
    //1 - OBS
    th.start();
}

void MainWindow::on_pushButtonStop_clicked() {
    //0 - GDI
    //1 - OBS
    th.stop();
}

void MainWindow::on_pushButtonSHMEM_clicked() {
    pixWritePng("finalTest.png", (Pix *)(shmemReader.read()),0);
}

void MainWindow::on_pushButtonSCR_clicked() {
    //ScreenShooter::take();
    //pixWritePng("rwout.png", pixRead("lossless.bmp"), 0);
}