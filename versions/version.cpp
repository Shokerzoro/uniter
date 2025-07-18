
#include <QApplication>

void embed_meta(QApplication & app)
{
    app.setApplicationName(PROJECT_NAME);
    app.setApplicationVersion(PROJECT_VERSION);
    app.setOrganizationName(AUTHOR);
}

