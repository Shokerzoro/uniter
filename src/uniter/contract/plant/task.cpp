
#include "task.h"
#include <tinyxml2.h>

namespace uniter::contract::plant {

namespace {

// ---------------- Хелперы сериализации узлов дерева ----------------
// Узлы — не ResourceAbstract, поэтому хелперы put*/get* вызываем через
// квалифицированное имя класса (они статические).

using R = uniter::contract::ResourceAbstract;

void writePartNode(tinyxml2::XMLElement* p, const ProductionPartNode& n) {
    R::putUInt64     (p, "assembly_id",        n.assembly_id);
    R::putUInt64     (p, "project_id",         n.project_id);
    R::putUInt64     (p, "part_id",            n.part_id);
    R::putInt        (p, "status",             static_cast<int>(n.status));
    R::putUInt64     (p, "quantity_planned",   n.quantity_planned);
    R::putUInt64     (p, "quantity_completed", n.quantity_completed);
    R::putDateTime   (p, "planned_start",      n.planned_start);
    R::putDateTime   (p, "planned_end",        n.planned_end);
    R::putOptDateTime(p, "actual_start",       n.actual_start);
    R::putOptDateTime(p, "actual_end",         n.actual_end);
    R::putDateTime   (p, "updated_at",         n.updated_at);
}

void readPartNode(const tinyxml2::XMLElement* p, ProductionPartNode& n) {
    n.assembly_id        = R::getUInt64     (p, "assembly_id");
    n.project_id         = R::getUInt64     (p, "project_id");
    n.part_id            = R::getUInt64     (p, "part_id");
    n.status             = static_cast<TaskStatus>(
                            R::getInt(p, "status", static_cast<int>(TaskStatus::PLANNED)));
    n.quantity_planned   = R::getUInt64     (p, "quantity_planned");
    n.quantity_completed = R::getUInt64     (p, "quantity_completed");
    n.planned_start      = R::getDateTime   (p, "planned_start");
    n.planned_end        = R::getDateTime   (p, "planned_end");
    n.actual_start       = R::getOptDateTime(p, "actual_start");
    n.actual_end         = R::getOptDateTime(p, "actual_end");
    n.updated_at         = R::getDateTime   (p, "updated_at");
}

// Forward-decl: assembly-узлы рекурсивны.
void writeAssemblyNode(tinyxml2::XMLElement* a, const ProductionAssemblyNode& n);
void readAssemblyNode (const tinyxml2::XMLElement* a, ProductionAssemblyNode& n);

void writeAssemblyNode(tinyxml2::XMLElement* a, const ProductionAssemblyNode& n) {
    R::putUInt64     (a, "assembly_id",        n.assembly_id);
    R::putUInt64     (a, "project_id",         n.project_id);
    R::putInt        (a, "status",             static_cast<int>(n.status));
    R::putUInt64     (a, "quantity_planned",   n.quantity_planned);
    R::putUInt64     (a, "quantity_completed", n.quantity_completed);
    R::putDateTime   (a, "planned_start",      n.planned_start);
    R::putDateTime   (a, "planned_end",        n.planned_end);
    R::putOptDateTime(a, "actual_start",       n.actual_start);
    R::putOptDateTime(a, "actual_end",         n.actual_end);
    R::putDateTime   (a, "updated_at",         n.updated_at);

    auto* doc = a->GetDocument();
    if (!doc) return;

    // Дочерние сборки
    auto* childAsmEl = doc->NewElement("ChildAssemblies");
    for (const auto& child : n.child_assemblies) {
        auto* c = doc->NewElement("AssemblyNode");
        writeAssemblyNode(c, child);
        childAsmEl->InsertEndChild(c);
    }
    a->InsertEndChild(childAsmEl);

    // Входящие детали
    auto* partsEl = doc->NewElement("Parts");
    for (const auto& p : n.parts) {
        auto* pe = doc->NewElement("PartNode");
        writePartNode(pe, p);
        partsEl->InsertEndChild(pe);
    }
    a->InsertEndChild(partsEl);
}

void readAssemblyNode(const tinyxml2::XMLElement* a, ProductionAssemblyNode& n) {
    n.assembly_id        = R::getUInt64     (a, "assembly_id");
    n.project_id         = R::getUInt64     (a, "project_id");
    n.status             = static_cast<TaskStatus>(
                            R::getInt(a, "status", static_cast<int>(TaskStatus::PLANNED)));
    n.quantity_planned   = R::getUInt64     (a, "quantity_planned");
    n.quantity_completed = R::getUInt64     (a, "quantity_completed");
    n.planned_start      = R::getDateTime   (a, "planned_start");
    n.planned_end        = R::getDateTime   (a, "planned_end");
    n.actual_start       = R::getOptDateTime(a, "actual_start");
    n.actual_end         = R::getOptDateTime(a, "actual_end");
    n.updated_at         = R::getDateTime   (a, "updated_at");

    n.child_assemblies.clear();
    if (auto* childAsmEl = a->FirstChildElement("ChildAssemblies")) {
        for (auto* c = childAsmEl->FirstChildElement("AssemblyNode");
             c; c = c->NextSiblingElement("AssemblyNode")) {
            ProductionAssemblyNode child;
            readAssemblyNode(c, child);
            n.child_assemblies.push_back(std::move(child));
        }
    }

    n.parts.clear();
    if (auto* partsEl = a->FirstChildElement("Parts")) {
        for (auto* pe = partsEl->FirstChildElement("PartNode");
             pe; pe = pe->NextSiblingElement("PartNode")) {
            ProductionPartNode p;
            readPartNode(pe, p);
            n.parts.push_back(std::move(p));
        }
    }
}

} // anonymous

// ---------------- Task: каскадная сериализация ----------------

void Task::to_xml(tinyxml2::XMLElement* dest) {
    if (!dest) return;
    ResourceAbstract::to_xml(dest);

    putString     (dest, "code",           code);
    putString     (dest, "name",           name);
    putUInt64     (dest, "project_id",     project_id);
    putUInt64     (dest, "snapshot_id",    snapshot_id);
    putUInt64     (dest, "plant_id",       plant_id);
    putUInt64     (dest, "quantity",       quantity);
    putInt        (dest, "status",         static_cast<int>(status));
    putDateTime   (dest, "planned_start",  planned_start);
    putDateTime   (dest, "planned_end",    planned_end);
    putOptDateTime(dest, "actual_start",   actual_start);
    putOptDateTime(dest, "actual_end",     actual_end);

    auto* doc = dest->GetDocument();
    if (!doc) return;

    auto* rootsEl = doc->NewElement("RootAssemblies");
    for (const auto& ra : root_assemblies) {
        auto* a = doc->NewElement("AssemblyNode");
        writeAssemblyNode(a, ra);
        rootsEl->InsertEndChild(a);
    }
    dest->InsertEndChild(rootsEl);
}

void Task::from_xml(tinyxml2::XMLElement* source) {
    if (!source) return;
    ResourceAbstract::from_xml(source);

    code          = getString     (source, "code");
    name          = getString     (source, "name");
    project_id    = getUInt64     (source, "project_id");
    snapshot_id   = getUInt64     (source, "snapshot_id");
    plant_id      = getUInt64     (source, "plant_id");
    quantity      = getUInt64     (source, "quantity");
    status        = static_cast<TaskStatus>(
                        getInt(source, "status", static_cast<int>(TaskStatus::PLANNED)));
    planned_start = getDateTime   (source, "planned_start");
    planned_end   = getDateTime   (source, "planned_end");
    actual_start  = getOptDateTime(source, "actual_start");
    actual_end    = getOptDateTime(source, "actual_end");

    root_assemblies.clear();
    if (auto* rootsEl = source->FirstChildElement("RootAssemblies")) {
        for (auto* a = rootsEl->FirstChildElement("AssemblyNode");
             a; a = a->NextSiblingElement("AssemblyNode")) {
            ProductionAssemblyNode n;
            readAssemblyNode(a, n);
            root_assemblies.push_back(std::move(n));
        }
    }
}


} // namespace uniter::contract::plant
