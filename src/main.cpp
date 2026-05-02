#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Quin Sandbox");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Quin");

    MainWindow w;
    w.show();

    return app.exec();
}
