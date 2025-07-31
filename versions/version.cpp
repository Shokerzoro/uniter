
#include <QApplication>

//!!!Не работает, нужно брать из реестра версию, и менять ее при запуске
void embed_meta(void)
{
    QCoreApplication::setApplicationName(PROJECT_NAME);
    QCoreApplication::setApplicationVersion(PROJECT_VERSION);
    QCoreApplication::setOrganizationName(AUTHOR);
}

