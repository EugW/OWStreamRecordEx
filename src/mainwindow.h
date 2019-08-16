#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QPushButton>
#include <QLabel>

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
    Ui::MainWindow *ui;
private slots:
    void on_pushButton1_clicked();
    void on_pushButton2_clicked();
    void on_pushButtonStop_clicked();
    void on_pushButtonSHMEM_clicked();
    void on_pushButtonSCR_clicked();
private:
    QGraphicsView *graphicsViewPreview;
    //QPushButton *pushButton1;
    //QPushButton *pushButton2;
    //QPushButton *pushButtonStop;
    QPushButton *pushButtonSHMEM;
    //QPushButton *pushButtonSCR;
    //QLabel *label1;
};

#endif // MAINWINDOW{}_H
