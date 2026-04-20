#ifndef MANAGERTYPES_H
#define MANAGERTYPES_H

#include <cstdint>

namespace uniter::contract::manager {

/**
 * @brief Тип производственной площадки (Plant).
 *
 * Перенесён из contract/plant/plant.h. Оставлен в отдельном заголовке, чтобы
 * остальные ресурсы подсистемы (и внешние подсистемы, например supply.Purchase
 * с FK `plant_id`) могли импортировать только enum, не таща весь ресурс Plant.
 */
enum class PlantType : uint8_t {
    PLANT     = 0, // Завод
    WAREHOUSE = 1, // Склад
    VIRTUAL   = 2  // Виртуальное (группа)
};

/**
 * @brief Статус интеграции с внешней компанией-партнёром.
 *
 * TODO(refactor): возможно, стоит вынести в отдельный ресурс
 *                 IntegrationContract и ссылаться на него из Integration.
 *                 Пока embedded enum — достаточно для базового CRUD.
 */
enum class IntegrationStatus : uint8_t {
    DRAFT   = 0, // Черновик — запись создана, партнёр ещё не подтвердил
    PENDING = 1, // Ожидает подтверждения / согласования
    ACTIVE  = 2, // Активна — можно создавать IntegrationTask
    REVOKED = 3  // Отозвана — новые задачи создавать нельзя
};

} // namespace uniter::contract::manager

#endif // MANAGERTYPES_H
