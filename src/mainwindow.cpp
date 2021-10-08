#include "mainwindow.h"
#include <iostream>
#include <QMainWindow>
#include <QThread>
#include "MDXGI.h"

MainWindow::MainWindow(QWidget *parent) :QMainWindow(parent),ui(new Ui::MainWindow) {
    ui->setupUi(this);
    graphicsScene = new QGraphicsScene;
    ui->graphicsViewPreview->setScene(graphicsScene);
    image = QImage(320, 180, QImage::Format_RGB32);
    imgData = image.bits();
    pixmap = graphicsScene->addPixmap(QPixmap::fromImage(image));
    auto controller = ConfigController::getInstance();
    ui->lineEditDelay->setText(QString::number(controller->config.delay));
    ui->comboBoxMode->addItems(QStringList() << "OBS" << "GDI+" << "DXGI");
    ui->comboBoxMode->setCurrentIndex(controller->config.mode);
    ui->checkBoxPreview->setChecked(controller->config.preview);
    ui->lineEditPath->setText(QString::fromStdString(controller->config.path));
    ui->textEditPlaceholder->setText(QString::fromStdString(controller->config.placeholder));
    startService();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_pushButtonApplySettings_clicked() {
    auto ctrl = ConfigController::getInstance();
    ctrl->config.delay = ui->lineEditDelay->text().toInt();
    ctrl->config.mode = ui->comboBoxMode->currentIndex();
    ctrl->config.preview = ui->checkBoxPreview->isChecked();
    ctrl->config.path = ui->lineEditPath->text().toStdString();
    ctrl->config.placeholder = ui->textEditPlaceholder->toPlainText().toStdString();
    ctrl->saveConfig();
}

void MainWindow::updImage(Pix *pix) {
    auto swpPx = pixEndianByteSwapNew(pix);
    memmove(imgData, pixGetData(swpPx), pixGetWidth(pix) * pixGetHeight(pix) * pixGetDepth(pix)/8);
    pixmap->setPixmap(QPixmap::fromImage(image.rgbSwapped()));
    pixDestroy(&swpPx);
    pixDestroy(&pix);
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

void MainWindow::updOQ(int sr) {
    ui->labelOpenQSRVal->setText(QString::number(sr));
}

void MainWindow::startService() {
    if (running)
        return;
    worker = new VisionWorker;
    thread = new QThread;
    worker->moveToThread(thread);
    connect(thread, SIGNAL(started()), worker, SLOT(process()));
    connect(worker, SIGNAL(updPreview(Pix * )), this, SLOT(updImage(Pix * )));
    connect(worker, SIGNAL(updTank(int)), this, SLOT(updTnk(int)));
    connect(worker, SIGNAL(updDPS(int)), this, SLOT(updDmg(int)));
    connect(worker, SIGNAL(updSupport(int)), this, SLOT(updSup(int)));
    connect(worker, SIGNAL(updOpenQ(int)), this, SLOT(updOQ(int)));
    thread->start();
    running = true;
}