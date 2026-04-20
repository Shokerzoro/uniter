#ifndef INTEGRATIONTYPES_H
#define INTEGRATIONTYPES_H

#include <cstdint>

namespace uniter::contract::integration {

/**
 * @brief Статус задачи на интеграцию (передачу данных внешнему партнёру).
 *
 * Независимый от Integration статус: интеграция может быть ACTIVE, а конкретная
 * IntegrationTask внутри неё — ещё PENDING или уже COMPLETED.
 */
enum class IntegrationTaskStatus : uint8_t {
    PENDING     = 0, // Ожидает отправки
    IN_PROGRESS = 1, // В процессе передачи
    COMPLETED   = 2, // Успешно передано
    FAILED      = 3, // Не удалось (ошибка канала / отказ партнёра)
    CANCELLED   = 4  // Отменено пользователем
};

} // namespace uniter::contract::integration

#endif // INTEGRATIONTYPES_H
