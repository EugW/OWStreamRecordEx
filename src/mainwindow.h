#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include "VisionWorker.h"
#include "ConfigController.h"
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QPushButton>
#include <QLabel>
#include <leptonica/allheaders.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    QImage image;
    u_char *imgData;
    Ui::MainWindow *ui;
    VisionWorker *worker;
    QThread *thread;
    void startService();
    bool running = false;
private slots:
    void on_pushButtonApplySettings_clicked();
public slots:
    void updImage(Pix *pix);
    void updTnk(int sr);
    void updDmg(int sr);
    void updSup(int sr);
    void updOQ(int sr);
private:
    QGraphicsScene *graphicsScene;
    QGraphicsPixmapItem *pixmap;
};

#endif // MAINWINDOW{}_H
