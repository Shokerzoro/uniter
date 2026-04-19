
#include "snapshot.h"
#include <tinyxml2.h>

namespace uniter::contract::pdm {


void Snapshot::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    ResourceAbstract::to_xml(dest);

    putUInt64     (dest, "project_id",           project_id);
    putUInt32     (dest, "version",              version);
    putOptUInt64  (dest, "previous_snapshot_id", previous_snapshot_id);
    putString     (dest, "xml_object_key",       xml_object_key);
    putString     (dest, "xml_sha256",           xml_sha256);
    putInt        (dest, "status",               static_cast<int>(status));
    putOptUInt64  (dest, "approved_by",          approved_by);
    putOptDateTime(dest, "approved_at",          approved_at);
    putUInt32     (dest, "error_count",          error_count);
    putUInt32     (dest, "warning_count",        warning_count);
}

void Snapshot::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    ResourceAbstract::from_xml(source);

    project_id           = getUInt64     (source, "project_id");
    version              = getUInt32     (source, "version");
    previous_snapshot_id = getOptUInt64  (source, "previous_snapshot_id");
    xml_object_key       = getString     (source, "xml_object_key");
    xml_sha256           = getString     (source, "xml_sha256");
    status               = static_cast<SnapshotStatus>(getInt(source, "status",
                                                              static_cast<int>(SnapshotStatus::DRAFT)));
    approved_by          = getOptUInt64  (source, "approved_by");
    approved_at          = getOptDateTime(source, "approved_at");
    error_count          = getUInt32     (source, "error_count");
    warning_count        = getUInt32     (source, "warning_count");
}


} // namespace uniter::contract::pdm
