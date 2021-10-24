#include "stdafx.h"

#include "MainWindow.hpp"

int main(int argc, char *argv[])
{
    // application settings
    QCoreApplication::setOrganizationName("visionS");
    QCoreApplication::setOrganizationDomain("visions.work");
    QCoreApplication::setApplicationName("rakuten");

    QApplication app(argc, argv);
	MainWindow w;
	w.show();

    return app.exec();
}