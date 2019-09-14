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

MainWindow::~MainWindow()
{
    delete ui;
}

//void MainWindow::on_pushButton1_clicked()
//{
//    auto *api = new tesseract::TessBaseAPI();
//    if (api->Init(NULL, "eng")) {
//            fprintf(stderr, "Could not initialize tesseract.\n");
//            exit(1);
//        }
//        // Open input image with leptonica library
//        Pix *image = pixRead("test.png");
//        api->SetImage(image);
//        // Get OCR result
//        char *outText;
//        outText = api->GetUTF8Text();
//        std::cout<<outText<<std::endl;
//        //ui->label1->setText(outText);
//        // Destroy used object and release memory
//        api->End();
//        delete [] outText;
//        pixDestroy(&image);
//}


void MainWindow::on_pushButtonSHMEM_clicked() {
    worker = new VisionWorker(ui->checkBoxOBS->checkState() == Qt::Checked ? 1 : 0);
    thread = new QThread;
    worker->moveToThread(thread);
    connect(thread, SIGNAL(started()), worker, SLOT(process()));
    connect(worker, SIGNAL(updPreview(Pix * )), this, SLOT(updImage(Pix * )));
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
