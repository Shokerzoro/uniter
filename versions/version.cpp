#include <QCoreApplication>
#include <QSettings>
#include <QString>

void embed_meta()
{
    QCoreApplication::setApplicationName(PROJECT_NAME);
    QCoreApplication::setOrganizationName(AUTHOR);

#ifndef RELEASE
    QCoreApplication::setApplicationVersion(PROJECT_VERSION);
#else
    QSettings settings("HKEY_CURRENT_USER\\Software\\" + QCoreApplication::applicationName(), QSettings::NativeFormat);
    QString version = settings.value("Version").toString();
    QCoreApplication::setApplicationVersion(version);
#endif
}
