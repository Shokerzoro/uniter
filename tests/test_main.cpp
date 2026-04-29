#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QDir>
#include <QLibraryInfo>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    app.addLibraryPath(QLibraryInfo::path(QLibraryInfo::PluginsPath));

    const QString devkitPluginPath = QStringLiteral("C:/DevKit/Qt/6.9.0/shared/plugins");
    if (QDir(devkitPluginPath).exists()) {
        app.addLibraryPath(devkitPluginPath);
    }

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
