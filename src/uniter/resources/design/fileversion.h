#ifndef FILEVERSION_H
#define FILEVERSION_H

#include <QString>
#include <QDateTime>
#include <cstdint>
#include <memory>

namespace uniter::resources::design {

enum class DocumentType : uint8_t {
    ASSEMBLY_DRAWING    = 0,   // Сборочный чертёж
    MOUNTING_DRAWING    = 1,   // Монтажный чертёж
    MODEL_3D            = 2,           // 3D модель
    PART_DRAWING        = 3,       // Чертёж детали
    SPECIFICATION       = 4,      // Спецификация
    UNKNOWN             = 5
};

class FileVersion {
public:
    QString relative_path;
    DocumentType DocType;
    bool actual = true;

    uint64_t proposed_by_user_id;
    QDateTime proposed_at;
    QString change_comment;

    bool is_approved = false;
    uint64_t approved_by_user_id = 0;
    QDateTime approved_at;

    std::shared_ptr<FileVersion> prev_version;
};

} // design

#endif // FILEVERSION_H
