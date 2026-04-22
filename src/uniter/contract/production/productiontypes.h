#ifndef PRODUCTIONTYPES_H
#define PRODUCTIONTYPES_H

#include <cstdint>

namespace uniter::contract::production {

/**
 * @brief Статус производственного задания.
 *
 * Прикладной атрибут ProductionTask. В структурной схеме БД
 * (PRODUCTION.pdf) статус не указан — он не участвует в ключевых связях,
 * но нужен для ERP-аналитики и UI.
 */
enum class TaskStatus : uint8_t {
    PLANNED     = 0, // Запланировано
    IN_PROGRESS = 1, // В работе
    PAUSED      = 2, // Приостановлено
    COMPLETED   = 3, // Завершено
    CANCELLED   = 4  // Отменено
};

} // namespace uniter::contract::production

#endif // PRODUCTIONTYPES_H
