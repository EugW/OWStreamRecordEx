#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include "VisionWorker.h"
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QPushButton>
#include <QLabel>
#include <leptonica/allheaders.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    QImage image;
    u_char *imgData;
    Ui::MainWindow *ui;
    VisionWorker *worker{};
    QThread *thread{};
private slots:
    void on_pushButtonSHMEM_clicked();
    void on_pushButtonDest_clicked();

public slots:
    void updImage(Pix *pix);
    void workerDeath();
private:
    QGraphicsScene *graphicsScene;
    QGraphicsPixmapItem *pixmap;
};

#endif // MAINWINDOW{}_H
