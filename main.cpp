#include "stdafx.h"

#include "MainWindow.hpp"
#include "qt_helper.hpp"

#include <qfont.h>
#include <qsvggenerator.h>


int main(int argc, char *argv[])
{
    // application settings
    QCoreApplication::setOrganizationName("visionS");
    QCoreApplication::setOrganizationDomain("visions.work");
    QCoreApplication::setApplicationName("rakuten");

    QApplication app(argc, argv);
	MainWindow w;
	w.show();

	//classfs();

    return app.exec();
}
