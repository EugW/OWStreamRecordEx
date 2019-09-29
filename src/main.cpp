#include "mainwindow.h"
#include "ConfigController.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    auto inst = ConfigController::getInstance();
    inst->loadConfig();
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowFlags(Qt::MSWindowsFixedSizeDialogHint);
    w.show();
    return QApplication::exec();
}
