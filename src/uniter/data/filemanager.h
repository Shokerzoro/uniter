//
// Created by ACA on 14.05.2026.
//

#ifndef UNITER_FILEMANAGER_H
#define UNITER_FILEMANAGER_H

#include <QObject>

namespace data {

class FileManager : public QObject {
    Q_OBJECT
public:
    // singleton
    static FileManager& Instance() {
        static FileManager instance;
        return instance;
    }


private:
    // singleton
    FileManager() = default;

public slots:

signals:
};

}// namespace data

#endif //UNITER_FILEMANAGER_H