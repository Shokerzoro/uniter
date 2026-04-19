
#include "resourceabstract.h"
#include <tinyxml2.h>
#include <QByteArray>

namespace uniter::contract {

// -------------------- Каскадная сериализация общих полей --------------------

void ResourceAbstract::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    putUInt64  (dest, "id",         id);
    putBool    (dest, "actual",     is_actual);
    putDateTime(dest, "created_at", created_at);
    putDateTime(dest, "updated_at", updated_at);
    putUInt64  (dest, "created_by", created_by);
    putUInt64  (dest, "updated_by", updated_by);
}

void ResourceAbstract::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    id         = getUInt64  (source, "id");
    is_actual  = getBool    (source, "actual", true);
    created_at = getDateTime(source, "created_at");
    updated_at = getDateTime(source, "updated_at");
    created_by = getUInt64  (source, "created_by");
    updated_by = getUInt64  (source, "updated_by");
}

// -------------------- Хелперы: write --------------------

void ResourceAbstract::putString(tinyxml2::XMLElement* el, const char* name, const QString& v) {
    if (!el || !name) return;
    el->SetAttribute(name, v.toUtf8().constData());
}

void ResourceAbstract::putUInt64(tinyxml2::XMLElement* el, const char* name, uint64_t v) {
    if (!el || !name) return;
    el->SetAttribute(name, static_cast<uint64_t>(v));
}

void ResourceAbstract::putUInt32(tinyxml2::XMLElement* el, const char* name, uint32_t v) {
    if (!el || !name) return;
    el->SetAttribute(name, static_cast<unsigned int>(v));
}

void ResourceAbstract::putInt(tinyxml2::XMLElement* el, const char* name, int v) {
    if (!el || !name) return;
    el->SetAttribute(name, v);
}

void ResourceAbstract::putBool(tinyxml2::XMLElement* el, const char* name, bool v) {
    if (!el || !name) return;
    el->SetAttribute(name, v);
}

void ResourceAbstract::putDouble(tinyxml2::XMLElement* el, const char* name, double v) {
    if (!el || !name) return;
    el->SetAttribute(name, v);
}

void ResourceAbstract::putDateTime(tinyxml2::XMLElement* el, const char* name, const QDateTime& v) {
    if (!el || !name) return;
    if (!v.isValid()) { el->SetAttribute(name, ""); return; }
    const QByteArray s = v.toUTC().toString(Qt::ISODateWithMs).toUtf8();
    el->SetAttribute(name, s.constData());
}

void ResourceAbstract::putOptUInt64(tinyxml2::XMLElement* el, const char* name, const std::optional<uint64_t>& v) {
    if (!el || !name) return;
    if (v.has_value())
        el->SetAttribute(name, static_cast<uint64_t>(*v));
    // Если nullopt — атрибут не выставляем (отсутствие == NULL)
}

void ResourceAbstract::putOptDateTime(tinyxml2::XMLElement* el, const char* name, const std::optional<QDateTime>& v) {
    if (!el || !name) return;
    if (v.has_value() && v->isValid())
        putDateTime(el, name, *v);
}

// -------------------- Хелперы: read --------------------

QString ResourceAbstract::getString(const tinyxml2::XMLElement* el, const char* name, const QString& def) {
    if (!el || !name) return def;
    const char* v = el->Attribute(name);
    return v ? QString::fromUtf8(v) : def;
}

uint64_t ResourceAbstract::getUInt64(const tinyxml2::XMLElement* el, const char* name, uint64_t def) {
    if (!el || !name) return def;
    uint64_t out = def;
    return el->QueryUnsigned64Attribute(name, &out) == tinyxml2::XML_SUCCESS ? out : def;
}

uint32_t ResourceAbstract::getUInt32(const tinyxml2::XMLElement* el, const char* name, uint32_t def) {
    if (!el || !name) return def;
    unsigned out = def;
    return el->QueryUnsignedAttribute(name, &out) == tinyxml2::XML_SUCCESS
               ? static_cast<uint32_t>(out) : def;
}

int ResourceAbstract::getInt(const tinyxml2::XMLElement* el, const char* name, int def) {
    if (!el || !name) return def;
    int out = def;
    return el->QueryIntAttribute(name, &out) == tinyxml2::XML_SUCCESS ? out : def;
}

bool ResourceAbstract::getBool(const tinyxml2::XMLElement* el, const char* name, bool def) {
    if (!el || !name) return def;
    bool out = def;
    return el->QueryBoolAttribute(name, &out) == tinyxml2::XML_SUCCESS ? out : def;
}

double ResourceAbstract::getDouble(const tinyxml2::XMLElement* el, const char* name, double def) {
    if (!el || !name) return def;
    double out = def;
    return el->QueryDoubleAttribute(name, &out) == tinyxml2::XML_SUCCESS ? out : def;
}

QDateTime ResourceAbstract::getDateTime(const tinyxml2::XMLElement* el, const char* name) {
    if (!el || !name) return QDateTime();
    const char* v = el->Attribute(name);
    if (!v || !*v) return QDateTime();
    return QDateTime::fromString(QString::fromUtf8(v), Qt::ISODateWithMs);
}

std::optional<uint64_t> ResourceAbstract::getOptUInt64(const tinyxml2::XMLElement* el, const char* name) {
    if (!el || !name) return std::nullopt;
    if (!el->Attribute(name)) return std::nullopt;
    uint64_t out = 0;
    if (el->QueryUnsigned64Attribute(name, &out) == tinyxml2::XML_SUCCESS)
        return out;
    return std::nullopt;
}

std::optional<QDateTime> ResourceAbstract::getOptDateTime(const tinyxml2::XMLElement* el, const char* name) {
    if (!el || !name) return std::nullopt;
    const char* v = el->Attribute(name);
    if (!v || !*v) return std::nullopt;
    const QDateTime dt = QDateTime::fromString(QString::fromUtf8(v), Qt::ISODateWithMs);
    return dt.isValid() ? std::optional<QDateTime>(dt) : std::nullopt;
}

} // namespace uniter::contract
