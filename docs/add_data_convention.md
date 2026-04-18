# Конвенция add_data в UniterMessage

`UniterMessage::add_data` (`std::map<QString, QString>`) — унифицированный механизм
передачи параметров протокольных операций. Намеренно не типизирован жёстко,
чтобы не плодить специализированные поля. В будущем планируется добавить
вспомогательный интерфейс (адаптер/builder) для типобезопасной работы с этим полем.

> **Важно:** Ключи ниже — единственный источник истины. Любой компонент,
> формирующий или читающий UniterMessage, обязан использовать строго эти ключи.

---

## Subsystem::PROTOCOL

### AUTH

| Направление | Ключ | Значение | Обязателен |
|-------------|------|----------|-----------|
| REQUEST | `"login"` | Логин пользователя | Да |
| REQUEST | `"password_hash"` | SHA-256 от пароля (hex) | Да |
| RESPONSE | — | Данные пользователя передаются в `resource` (Employee) | — |

---

### GET_KAFKA_CREDENTIALS

| Направление | Ключ | Значение | Обязателен |
|-------------|------|----------|-----------|
| REQUEST | — | Нет дополнительных параметров | — |
| RESPONSE | `"bootstrap_servers"` | `"host:port"` или `"host1:port1,host2:port2"` | Да |
| RESPONSE | `"topic"` | Название топика компании | Да |
| RESPONSE | `"username"` | SASL username | Да |
| RESPONSE | `"password"` | SASL password | Да |
| RESPONSE | `"group_id"` | Consumer group ID (привязан к пользователю) | Да |

---

### GET_MINIO_PRESIGNED_URL

| Направление | Ключ | Значение | Обязателен |
|-------------|------|----------|-----------|
| REQUEST | `"object_key"` | Путь объекта в MinIO bucket, напр. `"drawings/SB-001/Part-42.pdf"` | Да |
| REQUEST | `"minio_operation"` | `"GET"` или `"PUT"` | Да |
| RESPONSE | `"object_key"` | Эхо ключа из запроса (для сопоставления ответа с запросом) | Да |
| RESPONSE | `"presigned_url"` | Временный URL с подписью | Да |
| RESPONSE | `"url_expires_at"` | ISO 8601 UTC, напр. `"2026-04-18T20:00:00Z"` | Да |

---

### GET_MINIO_FILE

| Направление | Ключ | Значение | Обязателен |
|-------------|------|----------|-----------|
| REQUEST | `"presigned_url"` | URL, полученный из GET_MINIO_PRESIGNED_URL RESPONSE | Да |
| REQUEST | `"object_key"` | Ключ объекта (для идентификации в callback) | Да |
| REQUEST | `"expected_sha256"` | SHA-256 ожидаемого файла (hex), для верификации после скачивания | Нет |
| RESPONSE | `"object_key"` | Эхо ключа | Да |
| RESPONSE | `"local_path"` | Путь к скачанному файлу на диске | Да |
| ERROR | `"object_key"` | Эхо ключа | Да |
| ERROR | `"reason"` | Текстовое описание ошибки | Нет |

---

### FULL_SYNC

| Направление | Ключ | Значение | Обязателен |
|-------------|------|----------|-----------|
| REQUEST | — | Нет параметров | — |
| SUCCESS | — | Маркер завершения: `status=SUCCESS`, нет ключей в add_data | — |

> Тело FULL_SYNC — поток CRUD NOTIFICATION-сообщений через Kafka.
> Сам FULL_SYNC REQUEST/SUCCESS идёт через ServerConnector (TCP).

---

### UPDATE_CHECK

| Направление | Ключ | Значение | Обязателен |
|-------------|------|----------|-----------|
| REQUEST | `"current_version"` | Текущая версия приложения, напр. `"1.2.3"` | Да |
| RESPONSE (обновление есть) | `"update_available"` | `"true"` | Да |
| RESPONSE (обновление есть) | `"new_version"` | Версия обновления, напр. `"1.3.0"` | Да |
| RESPONSE (обновление есть) | `"release_notes"` | Краткое описание изменений | Нет |
| RESPONSE (нет обновления) | `"update_available"` | `"false"` | Да |

---

### UPDATE_CONSENT

| Направление | Ключ | Значение | Обязателен |
|-------------|------|----------|-----------|
| (клиентское действие) | `"accepted"` | `"true"` или `"false"` | Да |

> UPDATE_CONSENT — **чисто клиентское действие** (согласие пользователя на установку).
> На сервер не отправляется. Это сигнал от UI → UpdaterWorker через AppManager.
> Если пользователь согласился — UpdaterWorker инициирует UPDATE_DOWNLOAD REQUEST.

---

### UPDATE_DOWNLOAD

Протокол передачи файлов обновления:

**Инициация (клиент → сервер):**
```
subsystem: PROTOCOL
protact:   UPDATE_DOWNLOAD
status:    REQUEST
add_data:  { "version": "1.3.0" }   // версия, которую хотим скачать
```

**Сервер присылает каждый файл обновления отдельным сообщением:**
```
subsystem: PROTOCOL
protact:   UPDATE_DOWNLOAD
status:    NOTIFICATION
add_data:
  "file_path":   "uniter_setup_1.3.0.exe"   // имя файла с расширением (полное, как сохранять)
  "presigned_url": "https://minio.../..."   // временный URL для скачивания
  "file_size":   "15728640"                 // размер в байтах (для прогресс-бара)
  "sha256":      "a3f1..."                  // SHA-256 для верификации после скачивания
  "file_index":  "1"                        // порядковый номер файла в пакете обновления
  "files_total": "3"                        // всего файлов в пакете
```

**После отправки всех файлов сервер закрывает поток:**
```
subsystem: PROTOCOL
protact:   UPDATE_DOWNLOAD
status:    SUCCESS
add_data:
  "version":     "1.3.0"
  "files_total": "3"                        // итоговое число (для сверки с полученным)
```

**Клиент скачивает каждый файл через MinIOConnector по presigned_url.**
Верификация: SHA-256 скачанного файла == `add_data["sha256"]` из NOTIFICATION.
При несовпадении — повторный запрос (новый UPDATE_DOWNLOAD REQUEST для конкретного файла).

| Направление | Ключ | Обязателен |
|-------------|------|-----------|
| REQUEST | `"version"` | Да |
| NOTIFICATION | `"file_path"` | Да |
| NOTIFICATION | `"presigned_url"` | Да |
| NOTIFICATION | `"file_size"` | Нет |
| NOTIFICATION | `"sha256"` | Да |
| NOTIFICATION | `"file_index"` | Да |
| NOTIFICATION | `"files_total"` | Да |
| SUCCESS | `"version"` | Да |
| SUCCESS | `"files_total"` | Да |

---

## Kafka-сообщения (KafkaConnector → AppManager)

Все сообщения из Kafka имеют `subsystem != PROTOCOL` и один из статусов:

| MessageStatus | Смысл | Кто получает | add_data |
|--------------|-------|-------------|---------|
| `NOTIFICATION` | Изменение, сделанное **другим** пользователем | DataManager | `"kafka_offset"`: текущий offset (строка) |
| `SUCCESS` | Подтверждение **своей** CUD-операции, прошедшей через сервер | DataManager | `"kafka_offset"`: текущий offset; `"request_id"`: эхо из исходного запроса |

> `"kafka_offset"` добавляется KafkaConnector автоматически к каждому сообщению.
> DataManager использует его только для диагностики; постоянное хранение offset — 
> ответственность KafkaConnector (OS Secure Storage).

---

## Правила работы с add_data

1. Ключи — строго из этого документа. Строковые литералы в коде — источник ошибок.
   В будущем все ключи будут вынесены в `contract/add_data_keys.h` как `constexpr`.
2. Значения — всегда `QString`. Числа сериализуются `QString::number(x)`,
   булевы — `"true"` / `"false"`.
3. Отсутствующий необязательный ключ означает "не указано" — не ошибка.
4. При получении сообщения — проверять наличие обязательных ключей перед `operator[]`.

---

## Subsystem::GENERATIVE — CRUD на ресурсах генеративных подсистем

Для генеративных подсистем контекст сообщения определяется тройкой:
`subsystem=GENERATIVE`, `GenSubsystem`, `GenSubsystemId`.
Поле `add_data` используется минимально — основные данные в `resource`.

### Адресация генеративной подсистемы

| Поле UniterMessage | Значение |
|---|---|
| `subsystem` | `GENERATIVE` |
| `GenSubsystem` | `PRODUCTION` или `INTERGATION` |
| `GenSubsystemId` | ID конкретного производства / интеграции |
| `crudact` | `CREATE` / `UPDATE` / `DELETE` |
| `resource_type` | см. ниже |
| `resource` | объект ресурса |

---

### Виртуальный склад производства (GenSubsystem::PRODUCTION)

Склад — набор `MaterialInstance` внутри конкретного производства.
Управляется стандартными CRUD-операциями: нет специального ProtocolAction.

**Добавить позицию на склад:**
```
subsystem      = GENERATIVE
GenSubsystem   = PRODUCTION
GenSubsystemId = <id производства>
crudact        = CREATE
resource_type  = MATERIAL_INSTANCE
resource       = MaterialInstanceSimple | MaterialInstanceComposite
add_data       = {}  // пусто
```

**Изменить количество позиции:**
```
crudact        = UPDATE
resource_type  = MATERIAL_INSTANCE
resource       = MaterialInstance с обновлённым Quantity
```

**Удалить позицию со склада:**
```
crudact        = DELETE
resource_type  = MATERIAL_INSTANCE
resource       = MaterialInstance (достаточно id)
```

DataManager при обработке таких сообщений работает с таблицей
`production_stock(production_id, material_instance_id, ...)`,
а не с общей `material_instances`.

---

### Производственное задание (GenSubsystem::PRODUCTION)

**Создать задание:**
```
subsystem      = GENERATIVE
GenSubsystem   = PRODUCTION
GenSubsystemId = <id производства>
crudact        = CREATE
resource_type  = PRODUCTION_TASK
resource       = ProductionTask
add_data       = {}
```

**Привязка к конструкторским данным** (опционально, при создании задания):

| Ключ | Значение |
|------|---------|
| `"snapshot_id"` | ID снимка конструктора (PDM::Snapshot), из которого берётся состав |
| `"snapshot_version"` | Версия снимка (строка) |

---

### Аналогия для GenSubsystem::INTERGATION

Структура идентична PRODUCTION: `INTEGRATION_TASK` вместо `PRODUCTION_TASK`.
Детали будут проработаны при реализации подсистемы интеграции.

---

### Создание новой генеративной подсистемы (Subsystem::MANAGER)

Генеративный ресурс — точка входа. Создаётся через MANAGER:

```
subsystem      = MANAGER
crudact        = CREATE
resource_type  = PRODUCTION   // или INTEGRATION
resource       = Production { name, description, ... }
add_data       = {}
```

Сервер в ответе возвращает тот же ресурс с заполненным `id`.
KafkaConnector рассылает это `NOTIFICATION` всем участникам.
После получения NOTIFICATION DataManager регистрирует новую генеративную
подсистему, ConfigManager генерирует новую вкладку UI.
