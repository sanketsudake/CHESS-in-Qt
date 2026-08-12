#include "MainWindow.hpp"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    QCoreApplication::setApplicationName(QStringLiteral("CINES"));
    QCoreApplication::setOrganizationName(QStringLiteral("CINES"));
    QCoreApplication::setApplicationVersion(QStringLiteral(CINES_VERSION_STRING));

    cines::MainWindow window;
    window.show();

    return QApplication::exec();
}
