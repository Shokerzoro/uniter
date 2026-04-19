// Round-trip тесты сериализации/десериализации всех ресурсов contract/.
//
// Для каждого ресурса создаётся объект со значимыми полями, сериализуется
// в tinyxml2::XMLElement через to_xml(), затем новый пустой объект восстанавливается
// из того же элемента через from_xml() и сравнивается с исходным через operator==.
//
// Паттерн operator== для наследников ResourceAbstract:
//   static_cast<const ResourceAbstract&>(a) == static_cast<const ResourceAbstract&>(b)
//   плюс пополевное сравнение собственных полей (см. resourceabstract.h).

#include "../src/uniter/contract/resourceabstract.h"

#include "../src/uniter/contract/employee/employee.h"
#include "../src/uniter/contract/employee/permissions.h"

#include "../src/uniter/contract/design/designtypes.h"
#include "../src/uniter/contract/design/assembly.h"
#include "../src/uniter/contract/design/part.h"
#include "../src/uniter/contract/design/project.h"

#include "../src/uniter/contract/material/segment.h"
#include "../src/uniter/contract/material/templatebase.h"
#include "../src/uniter/contract/material/templatesimple.h"
#include "../src/uniter/contract/material/templatecomposite.h"

#include "../src/uniter/contract/instance/instancebase.h"
#include "../src/uniter/contract/instance/instancesimple.h"
#include "../src/uniter/contract/instance/instancecomposite.h"

#include "../src/uniter/contract/documents/doctypes.h"
#include "../src/uniter/contract/documents/doc.h"
#include "../src/uniter/contract/documents/doclink.h"

#include "../src/uniter/contract/pdm/pdmtypes.h"
#include "../src/uniter/contract/pdm/snapshot.h"
#include "../src/uniter/contract/pdm/delta.h"

#include "../src/uniter/contract/plant/plant.h"
#include "../src/uniter/contract/plant/task.h"

#include "../src/uniter/contract/production/productionstock.h"

#include "../src/uniter/contract/supply/purchase.h"
#include "../src/uniter/contract/supply/purchasecomplex.h"

#include <tinyxml2.h>
#include <gtest/gtest.h>
#include <QDateTime>
#include <QString>
#include <memory>
#include <optional>

using namespace uniter::contract;

namespace resourcestests {

// -----------------------------------------------------------------------------
// Вспомогательные константы и хелперы
// -----------------------------------------------------------------------------

// Фиксированное время для детерминизма (UTC ISO 8601).
static QDateTime fixedDateTime(const QString& iso) {
    QDateTime dt = QDateTime::fromString(iso, Qt::ISODate);
    dt.setTimeSpec(Qt::UTC);
    return dt;
}

static const QDateTime kCreatedAt = fixedDateTime("2025-01-15T10:30:00");
static const QDateTime kUpdatedAt = fixedDateTime("2025-02-20T14:45:30");

// Round-trip: сериализовать obj в свежий XMLElement, десериализовать во второй
// объект, вернуть его. Сам obj не модифицируется.
template <typename T>
static T roundTrip(T& obj) {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement* root = doc.NewElement("Root");
    doc.InsertFirstChild(root);

    obj.to_xml(root);

    T restored;
    restored.from_xml(root);
    return restored;
}


// -----------------------------------------------------------------------------
// EMPLOYEE
// -----------------------------------------------------------------------------

TEST(ResourcesSerialization, Employee) {
    employees::EmployeeAssignment a1;
    a1.subsystem    = Subsystem::DESIGN;
    a1.genSubsystem = GenSubsystemType::NOTGEN;
    a1.genId        = std::nullopt;
    a1.permissions  = { 0, 1, 3 };

    employees::EmployeeAssignment a2;
    a2.subsystem    = Subsystem::GENERATIVE;
    a2.genSubsystem = GenSubsystemType::PRODUCTION;
    a2.genId        = 777;
    a2.permissions  = { 2, 4 };

    employees::Employee emp(
        42,                          // id
        true,                        // actual
        kCreatedAt,
        kUpdatedAt,
        1,                           // created_by
        2,                           // updated_by
        QStringLiteral("Иван"),
        QStringLiteral("Иванов"),
        QStringLiteral("Иванович"),
        QStringLiteral("ivan@example.ru"),
        { a1, a2 }
    );

    employees::Employee restored = roundTrip(emp);
    EXPECT_EQ(emp, restored);
}


// -----------------------------------------------------------------------------
// DESIGN: Project / Assembly / Part
// -----------------------------------------------------------------------------

TEST(ResourcesSerialization, Project) {
    design::Project proj(
        100,
        true,
        kCreatedAt,
        kUpdatedAt,
        1, 2,
        QStringLiteral("Редуктор РМ-250"),
        QStringLiteral("Проект редуктора серийной партии"),
        QStringLiteral("РМ.250.000"),
        500,                      // root_assembly_id
        std::optional<uint64_t>{999}  // active_snapshot_id
    );

    design::Project restored = roundTrip(proj);
    EXPECT_EQ(proj, restored);
}

TEST(ResourcesSerialization, Project_NullActiveSnapshot) {
    design::Project proj(
        101,
        true,
        kCreatedAt,
        kUpdatedAt,
        1, 1,
        QStringLiteral("Новый проект"),
        QStringLiteral("Пока без snapshot"),
        QStringLiteral("НП.001.000"),
        501,
        std::nullopt
    );

    design::Project restored = roundTrip(proj);
    EXPECT_EQ(proj, restored);
}

TEST(ResourcesSerialization, Assembly) {
    design::Assembly asm_(
        500,
        true,
        kCreatedAt,
        kUpdatedAt,
        1, 2,
        100,                              // project_id
        std::optional<uint64_t>{499},     // parent_assembly_id
        QStringLiteral("СБ-001"),
        QStringLiteral("Корпус редуктора"),
        QStringLiteral("Корпус в сборе"),
        design::AssemblyType::REAL
    );
    asm_.child_assemblies = {
        { /*child_assembly_id*/ 501, /*quantity*/ 1, /*config*/ QStringLiteral("01") },
        { 502, 2, QString() }
    };
    asm_.parts = {
        { /*part_id*/ 801, 4, QStringLiteral("01") },
        { 802, 1, QString() }
    };

    design::Assembly restored = roundTrip(asm_);
    EXPECT_EQ(asm_, restored);
}

TEST(ResourcesSerialization, Part) {
    design::Part part(
        801,
        true,
        kCreatedAt,
        kUpdatedAt,
        1, 2,
        100,    // project_id
        500,    // assembly_id
        QStringLiteral("СБ-001-01"),
        QStringLiteral("Вал выходной"),
        QStringLiteral("Сталь 45, закалка")
    );
    part.litera               = QStringLiteral("О");
    part.organization         = QStringLiteral("ОАО Завод");
    part.material_instance_id = 7777;
    part.configs = {
        { QStringLiteral("01"), 250.0, 40.0, 40.0, 2.45 },
        { QStringLiteral("02"), 300.0, 40.0, 40.0, 2.90 }
    };
    part.signatures = {
        { QStringLiteral("Разработал"), QStringLiteral("Петров"), kCreatedAt },
        { QStringLiteral("Проверил"),   QStringLiteral("Сидоров"), kUpdatedAt }
    };

    design::Part restored = roundTrip(part);
    EXPECT_EQ(part, restored);
}


// -----------------------------------------------------------------------------
// MATERIALS: TemplateSimple / TemplateComposite
// -----------------------------------------------------------------------------

TEST(ResourcesSerialization, TemplateSimple_Standalone) {
    materials::SegmentDefinition seg1{
        /*id*/ 1,
        QStringLiteral("gost_number"),
        QStringLiteral("Номер ГОСТа"),
        QStringLiteral("Обозначение стандарта"),
        materials::SegmentValueType::STRING,
        {},
        true
    };

    materials::TemplateSimple tpl(
        200,
        QStringLiteral("Болт М10"),
        QStringLiteral("Болт по ГОСТ 7798-70"),
        materials::DimensionType::PIECE,
        materials::GostSource::BUILT_IN,
        materials::StandartType::STANDALONE,
        materials::GostStandardType::GOST,
        QStringLiteral("7798-70"),
        /*prefix_segments*/ { { 1, seg1 } },
        /*prefix_order*/ { 1 },
        /*suffix_segments*/ {},
        /*suffix_order*/ {},
        /*compatible_template_ids*/ {},
        /*is_active*/ true,
        kCreatedAt,
        kUpdatedAt,
        /*version*/ 1,
        /*created_by*/ 1,
        /*updated_by*/ 1
    );
    tpl.year = QStringLiteral("70");

    materials::TemplateSimple restored = roundTrip(tpl);
    EXPECT_EQ(tpl, restored);
}

TEST(ResourcesSerialization, TemplateSimple_Assortment) {
    materials::SegmentDefinition seg_profile{
        2,
        QStringLiteral("profile"),
        QStringLiteral("Профиль сечения"),
        QStringLiteral("Профиль проката"),
        materials::SegmentValueType::ENUM,
        { { 1, "Лист" }, { 2, "Круг" }, { 3, "Швеллер" } },
        true
    };

    materials::TemplateSimple tpl(
        201,
        QStringLiteral("Швеллер стальной"),
        QStringLiteral("Швеллер по ГОСТ 8240-89"),
        materials::DimensionType::LINEAR,
        materials::GostSource::BUILT_IN,
        materials::StandartType::ASSORTMENT,
        materials::GostStandardType::GOST,
        QStringLiteral("8240-89"),
        { { 2, seg_profile } },
        { 2 },
        {},
        {},
        /*compatible_template_ids*/ { 210, 211, 212 },
        true,
        kCreatedAt,
        kUpdatedAt,
        2,
        1,
        1
    );
    tpl.year = QStringLiteral("89");

    materials::TemplateSimple restored = roundTrip(tpl);
    EXPECT_EQ(tpl, restored);
}

TEST(ResourcesSerialization, TemplateComposite) {
    materials::TemplateComposite tpl(
        300,
        QStringLiteral("Швеллер 20П ГОСТ 8240-89 / Сталь 09Г2С ГОСТ 19281-89"),
        QStringLiteral("Сортамент + марка стали"),
        materials::DimensionType::LINEAR,
        materials::GostSource::BUILT_IN,
        /*top_template_id*/    201,
        /*bottom_template_id*/ 210,
        true,
        kCreatedAt,
        kUpdatedAt,
        1,
        1,
        1
    );
    tpl.PrefName = QStringLiteral("Швеллер");

    materials::TemplateComposite restored = roundTrip(tpl);
    EXPECT_EQ(tpl, restored);
}


// -----------------------------------------------------------------------------
// INSTANCES: InstanceSimple / InstanceComposite
// -----------------------------------------------------------------------------

TEST(ResourcesSerialization, InstanceSimple) {
    InstanceSimple inst;
    // Поля ResourceAbstract заполняем напрямую (у InstanceSimple только =default ctor)
    inst.id         = 7001;
    inst.is_actual  = true;
    inst.created_at = kCreatedAt;
    inst.updated_at = kUpdatedAt;
    inst.created_by = 1;
    inst.updated_by = 2;
    // Поля InstanceBase
    inst.template_id    = 200;
    inst.name           = QStringLiteral("Болт М10х30");
    inst.description    = QStringLiteral("Специализация болта М10");
    inst.dimension_type = materials::DimensionType::PIECE;
    inst.quantity.items = 42;
    // Собственные поля InstanceSimple
    inst.prefix_values = { { 1, "М10х30" } };
    inst.suffix_values = { { 2, "оц" } };

    InstanceSimple restored = roundTrip(inst);
    EXPECT_EQ(inst, restored);
}

TEST(ResourcesSerialization, InstanceComposite) {
    InstanceComposite inst;
    inst.id         = 7002;
    inst.is_actual  = true;
    inst.created_at = kCreatedAt;
    inst.updated_at = kUpdatedAt;
    inst.created_by = 1;
    inst.updated_by = 2;
    inst.template_id    = 300;
    inst.name           = QStringLiteral("Швеллер 20П / Сталь 09Г2С");
    inst.description    = QStringLiteral("Составной экземпляр");
    inst.dimension_type = materials::DimensionType::LINEAR;
    inst.quantity.length = 12.5;
    inst.top_values    = { { 1, "20П" } };
    inst.bottom_values = { { 1, "09Г2С" } };

    InstanceComposite restored = roundTrip(inst);
    EXPECT_EQ(inst, restored);
}

TEST(ResourcesSerialization, InstanceSimple_AreaDimension) {
    InstanceSimple inst;
    inst.id         = 7003;
    inst.is_actual  = true;
    inst.created_at = kCreatedAt;
    inst.updated_at = kUpdatedAt;
    inst.created_by = 1;
    inst.updated_by = 1;
    inst.template_id    = 202;
    inst.name           = QStringLiteral("Лист 2 мм");
    inst.description    = QStringLiteral("Стальной лист");
    inst.dimension_type = materials::DimensionType::AREA;
    figure f;
    f.area = 3.75;
    inst.quantity.fig = f;
    inst.prefix_values = { { 1, "Лист 2" } };

    InstanceSimple restored = roundTrip(inst);
    EXPECT_EQ(inst, restored);
}


// -----------------------------------------------------------------------------
// DOCUMENTS: Doc / DocLink
// -----------------------------------------------------------------------------

TEST(ResourcesSerialization, Doc) {
    documents::Doc doc(
        900,
        true,
        kCreatedAt,
        kUpdatedAt,
        1, 2,
        documents::DocumentType::PART_DRAWING,
        QStringLiteral("Чертёж вала"),
        QStringLiteral("projects/100/parts/801/drawing.pdf"),
        QStringLiteral("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"),
        /*size_bytes*/ 524288,
        QStringLiteral("application/pdf"),
        QStringLiteral("Чертёж детали выходного вала, ред. 1")
    );

    documents::Doc restored = roundTrip(doc);
    EXPECT_EQ(doc, restored);
}

TEST(ResourcesSerialization, DocLink) {
    documents::DocLink link(
        901,
        true,
        kCreatedAt,
        kUpdatedAt,
        1, 2,
        /*doc_id*/ 900,
        documents::DocLinkTargetType::PART,
        /*target_id*/ 801,
        QStringLiteral("primary"),
        /*position*/ 0
    );

    documents::DocLink restored = roundTrip(link);
    EXPECT_EQ(link, restored);
}


// -----------------------------------------------------------------------------
// PDM: Snapshot / Delta
// -----------------------------------------------------------------------------

TEST(ResourcesSerialization, Snapshot) {
    pdm::Snapshot snap(
        1001,
        true,
        kCreatedAt,
        kUpdatedAt,
        1, 2,
        /*project_id*/ 100,
        /*version*/ 3,
        /*previous_snapshot_id*/ std::optional<uint64_t>{1000},
        QStringLiteral("snapshots/100/v3.xml"),
        QStringLiteral("1234abcd5678ef901234abcd5678ef901234abcd5678ef901234abcd5678ef90"),
        pdm::SnapshotStatus::APPROVED
    );
    snap.approved_by   = 42;
    snap.approved_at   = kUpdatedAt;
    snap.error_count   = 2;
    snap.warning_count = 7;

    pdm::Snapshot restored = roundTrip(snap);
    EXPECT_EQ(snap, restored);
}

TEST(ResourcesSerialization, Delta) {
    pdm::DeltaFileChange fc1;
    fc1.id             = 1;
    fc1.file_type      = documents::DocumentType::PART_DRAWING;
    fc1.old_object_key = QStringLiteral("old/key.pdf");
    fc1.old_sha256     = QStringLiteral("aaaa");
    fc1.new_object_key = QStringLiteral("new/key.pdf");
    fc1.new_sha256     = QStringLiteral("bbbb");

    pdm::DeltaChange ch1;
    ch1.id             = 10;
    ch1.designation    = QStringLiteral("СБ-001-01");
    ch1.element_type   = pdm::DeltaElementType::PART;
    ch1.change_type    = pdm::DeltaChangeType::MODIFY;
    ch1.changed_fields = { QStringLiteral("drawing"), QStringLiteral("litera") };
    ch1.file_changes   = { fc1 };

    pdm::DeltaChange ch2;
    ch2.id             = 11;
    ch2.designation    = QStringLiteral("СБ-002");
    ch2.element_type   = pdm::DeltaElementType::ASSEMBLY;
    ch2.change_type    = pdm::DeltaChangeType::ADD;
    ch2.changed_fields = {};
    ch2.file_changes   = {};

    pdm::Delta delta(
        2000,
        true,
        kCreatedAt,
        kUpdatedAt,
        1, 2,
        /*snapshot_id*/      1000,
        /*next_snapshot_id*/ 1001
    );
    delta.changes       = { ch1, ch2 };
    delta.changes_count = 2;

    pdm::Delta restored = roundTrip(delta);
    EXPECT_EQ(delta, restored);
}


// -----------------------------------------------------------------------------
// PLANT: Plant / Task
// -----------------------------------------------------------------------------

TEST(ResourcesSerialization, Plant) {
    plant::Plant p(
        3000,
        true,
        kCreatedAt,
        kUpdatedAt,
        1, 2,
        QStringLiteral("Цех №1"),
        QStringLiteral("Основной цех"),
        plant::PlantType::PLANT,
        QStringLiteral("Москва, ул. Промышленная, 10")
    );

    plant::Plant restored = roundTrip(p);
    EXPECT_EQ(p, restored);
}

TEST(ResourcesSerialization, Task) {
    plant::ProductionPartNode pnode;
    pnode.assembly_id        = 500;
    pnode.project_id         = 100;
    pnode.part_id            = 801;
    pnode.status             = plant::TaskStatus::PLANNED;
    pnode.quantity_planned   = 10;
    pnode.quantity_completed = 0;
    pnode.planned_start      = kCreatedAt;
    pnode.planned_end        = kUpdatedAt;
    pnode.actual_start       = std::nullopt;
    pnode.actual_end         = std::nullopt;
    pnode.updated_at         = kUpdatedAt;

    plant::ProductionAssemblyNode anode;
    anode.assembly_id        = 500;
    anode.project_id         = 100;
    anode.status             = plant::TaskStatus::IN_PROGRESS;
    anode.quantity_planned   = 5;
    anode.quantity_completed = 2;
    anode.planned_start      = kCreatedAt;
    anode.planned_end        = kUpdatedAt;
    anode.actual_start       = kCreatedAt;
    anode.actual_end         = std::nullopt;
    anode.child_assemblies   = {};
    anode.parts              = { pnode };
    anode.updated_at         = kUpdatedAt;

    plant::Task t(
        4000,
        true,
        kCreatedAt,
        kUpdatedAt,
        1, 2,
        QStringLiteral("T-2025-001"),
        QStringLiteral("Серийная партия редукторов"),
        /*project_id*/  100,
        /*snapshot_id*/ 1001,
        /*quantity*/    5,
        plant::TaskStatus::IN_PROGRESS,
        /*plant_id*/    3000
    );
    t.root_assemblies = { anode };
    t.planned_start   = kCreatedAt;
    t.planned_end     = kUpdatedAt;
    t.actual_start    = kCreatedAt;
    t.actual_end      = std::nullopt;

    plant::Task restored = roundTrip(t);
    EXPECT_EQ(t, restored);
}


// -----------------------------------------------------------------------------
// PRODUCTION: ProductionStock
// -----------------------------------------------------------------------------

TEST(ResourcesSerialization, ProductionStock) {
    production::ProductionStock stock(
        5000,
        true,
        kCreatedAt,
        kUpdatedAt,
        1, 2,
        /*material_instance_id*/ 7001,
        /*plant_id*/             3000,
        /*quantity_items*/  100,
        /*quantity_length*/ 0.0,
        /*quantity_area*/   0.0
    );

    production::ProductionStock restored = roundTrip(stock);
    EXPECT_EQ(stock, restored);
}


// -----------------------------------------------------------------------------
// SUPPLY: Purchase / PurchaseComplex
// -----------------------------------------------------------------------------

TEST(ResourcesSerialization, Purchase) {
    supply::Purchase pur(
        6000,
        true,
        kCreatedAt,
        kUpdatedAt,
        1, 2,
        QStringLiteral("Закупка стали 10т"),
        QStringLiteral("Сталь для проекта #100"),
        supply::PurchStatus::PLACED,
        std::optional<uint64_t>{7001}
    );

    supply::Purchase restored = roundTrip(pur);
    EXPECT_EQ(pur, restored);
}

TEST(ResourcesSerialization, Purchase_NullInstance) {
    supply::Purchase pur(
        6001,
        true,
        kCreatedAt,
        kUpdatedAt,
        1, 2,
        QStringLiteral("Черновик заявки"),
        QStringLiteral("Материал ещё не назначен"),
        supply::PurchStatus::DRAFT,
        std::nullopt
    );

    supply::Purchase restored = roundTrip(pur);
    EXPECT_EQ(pur, restored);
}

TEST(ResourcesSerialization, PurchaseComplex) {
    supply::PurchaseComplex grp(
        6100,
        true,
        kCreatedAt,
        kUpdatedAt,
        1, 2,
        QStringLiteral("Комплексная закупка"),
        QStringLiteral("Все материалы для проекта #100"),
        supply::PurchStatus::PLACED,
        std::vector<uint64_t>{ 6000, 6001, 6002 }
    );

    supply::PurchaseComplex restored = roundTrip(grp);
    EXPECT_EQ(grp, restored);
}


} // namespace resourcestests
