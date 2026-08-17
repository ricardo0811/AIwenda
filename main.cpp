#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName("AIwenda");
    a.setApplicationVersion("1.0.0");
    a.setOrganizationName("AIwenda");

    MainWindow w;
    w.show();

    return a.exec();
}
