
#include "delta.h"
#include <tinyxml2.h>

namespace uniter::contract::pdm {


void Delta::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    ResourceAbstract::to_xml(dest);

    putUInt64(dest, "snapshot_id",      snapshot_id);
    putUInt64(dest, "next_snapshot_id", next_snapshot_id);
    putUInt32(dest, "changes_count",    static_cast<uint32_t>(changes.size()));

    auto* doc = dest->GetDocument();
    if (!doc) return;

    tinyxml2::XMLElement* changesEl = doc->NewElement("Changes");
    for (const auto& ch : changes) {
        tinyxml2::XMLElement* c = doc->NewElement("Change");
        putUInt64(c, "id",             ch.id);
        putString(c, "designation",    ch.designation);
        putInt   (c, "element_type",   static_cast<int>(ch.element_type));
        putInt   (c, "change_type",    static_cast<int>(ch.change_type));
        putString(c, "changed_fields", ch.changed_fields.join(","));

        tinyxml2::XMLElement* filesEl = doc->NewElement("FileChanges");
        for (const auto& fc : ch.file_changes) {
            tinyxml2::XMLElement* f = doc->NewElement("FileChange");
            putUInt64(f, "id",             fc.id);
            putInt   (f, "file_type",      static_cast<int>(fc.file_type));
            putString(f, "old_object_key", fc.old_object_key);
            putString(f, "old_sha256",     fc.old_sha256);
            putString(f, "new_object_key", fc.new_object_key);
            putString(f, "new_sha256",     fc.new_sha256);
            filesEl->InsertEndChild(f);
        }
        c->InsertEndChild(filesEl);

        changesEl->InsertEndChild(c);
    }
    dest->InsertEndChild(changesEl);
}

void Delta::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    ResourceAbstract::from_xml(source);

    snapshot_id      = getUInt64(source, "snapshot_id");
    next_snapshot_id = getUInt64(source, "next_snapshot_id");
    changes_count    = getUInt32(source, "changes_count");

    changes.clear();
    if (auto* changesEl = source->FirstChildElement("Changes")) {
        for (auto* c = changesEl->FirstChildElement("Change");
             c; c = c->NextSiblingElement("Change")) {
            DeltaChange ch;
            ch.id           = getUInt64(c, "id");
            ch.designation  = getString(c, "designation");
            ch.element_type = static_cast<DeltaElementType>(getInt(c, "element_type",
                                                                   static_cast<int>(DeltaElementType::PART)));
            ch.change_type  = static_cast<DeltaChangeType>(getInt(c, "change_type",
                                                                  static_cast<int>(DeltaChangeType::MODIFY)));
            const QString fields = getString(c, "changed_fields");
            if (!fields.isEmpty())
                ch.changed_fields = fields.split(',', Qt::SkipEmptyParts);

            if (auto* filesEl = c->FirstChildElement("FileChanges")) {
                for (auto* f = filesEl->FirstChildElement("FileChange");
                     f; f = f->NextSiblingElement("FileChange")) {
                    DeltaFileChange fc;
                    fc.id             = getUInt64(f, "id");
                    fc.file_type      = static_cast<documents::DocumentType>(
                        getInt(f, "file_type", static_cast<int>(documents::DocumentType::UNKNOWN)));
                    fc.old_object_key = getString(f, "old_object_key");
                    fc.old_sha256     = getString(f, "old_sha256");
                    fc.new_object_key = getString(f, "new_object_key");
                    fc.new_sha256     = getString(f, "new_sha256");
                    ch.file_changes.push_back(std::move(fc));
                }
            }

            changes.push_back(std::move(ch));
        }
    }
    // Если поле не было записано явно — корректируем по факту
    if (changes_count == 0)
        changes_count = static_cast<uint32_t>(changes.size());
}


} // namespace uniter::contract::pdm
