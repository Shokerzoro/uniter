# Convention add_data in UniterMessage

`UniterMessage::add_data` (`std::map<QString, QString>`) - unified mechanism
transfer of parameters of protocol operations. Intentionally not strongly typed,
so as not to create specialized fields. In the future it is planned to add
auxiliary interface (adapter/builder) for type-safe work with this field.

> **Important:** The keys below are the only source of truth. Any component
> the person generating or reading the UniterMessage must strictly use these keys.

---

## Subsystem::PROTOCOL

### AUTH

| Direction | Key | Meaning | Required |
|-------------|------|----------|-----------|
| REQUEST | `"login"` | User login | Yes |
| REQUEST | `"password_hash"` | SHA-256 from password (hex) | Yes |
| RESPONSE | — | User data is passed to `resource` (Employee) | — |

---

### GET_KAFKA_CREDENTIALS

| Direction | Key | Meaning | Required |
|-------------|------|----------|-----------|
| REQUEST | — | No additional options | — |
| RESPONSE | `"bootstrap_servers"` | `"host:port"` or `"host1:port1,host2:port2"` | Yes |
| RESPONSE | `"topic"` | Company topic name | Yes |
| RESPONSE | `"username"` | SASL username | Yes |
| RESPONSE | `"password"` | SASL password | Yes |
| RESPONSE | `"group_id"` | Consumer group ID (linked to user) | Yes |

---

### GET_MINIO_PRESIGNED_URL

| Direction | Key | Meaning | Required |
|-------------|------|----------|-----------|
| REQUEST | `"object_key"` | Object path in MinIO bucket, e.g. `"drawings/SB-001/Part-42.pdf"` | Yes |
| REQUEST | `"minio_operation"` | `"GET"` or `"PUT"` | Yes |
| RESPONSE | `"object_key"` | Echo the key from the request (to match the response with the request) | Yes |
| RESPONSE | `"presigned_url"` | Temporary URL with signature | Yes |
| RESPONSE | `"url_expires_at"` | ISO 8601 UTC, e.g. `"2026-04-18T20:00:00Z"` | Yes |

---

### GET_MINIO_FILE

Downloading a file from MinIO using a previously received presigned URL.

| Direction | Key | Meaning | Required |
|-------------|------|----------|-----------|
| REQUEST | `"presigned_url"` | URL obtained from GET_MINIO_PRESIGNED_URL RESPONSE | Yes |
| REQUEST | `"object_key"` | Object key (for identification in callback) | Yes |
| REQUEST | `"expected_sha256"` | SHA-256 of the expected file (hex), for verification after downloading | No |
| RESPONSE | `"object_key"` | Key Echo | Yes |
| RESPONSE | `"local_path"` | Path to the downloaded file on disk | Yes |
| ERROR | `"object_key"` | Key Echo | Yes |
| ERROR | `"reason"` | Text description of the error | No |

---

### PUT_MINIO_FILE

Uploading a file to MinIO using the previously received presigned URL (`GET_MINIO_PRESIGNED_URL` with `minio_operation="PUT"`).

| Direction | Key | Meaning | Required |
|-------------|------|----------|-----------|
| REQUEST | `"presigned_url"` | URL obtained from GET_MINIO_PRESIGNED_URL RESPONSE (PUT option) | Yes |
| REQUEST | `"object_key"` | Object key (for identification in callback) | Yes |
| REQUEST | `"local_path"` | Path to the local file that is loaded into MinIO | Yes |
| REQUEST | `"sha256"` | SHA-256 of the downloaded file (hex), for verification on the MinIO side | No |
| RESPONSE | `"object_key"` | Key Echo | Yes |
| RESPONSE | `"presigned_url"` | Echo original URL | Yes |
| ERROR | `"object_key"` | Key Echo | Yes |
| ERROR | `"reason"` | Text description of the error | No |

---

### FULL_SYNC

| Direction | Key | Meaning | Required |
|-------------|------|----------|-----------|
| REQUEST | — | No options | — |
| SUCCESS | — | Completion marker: `status=SUCCESS`, no keys in add_data | — |

> The body of FULL_SYNC is a stream of CRUD NOTIFICATION messages via Kafka.
> FULL_SYNC REQUEST/SUCCESS itself goes through ServerConnector (TCP).

---

### UPDATE_CHECK

| Direction | Key | Meaning | Required |
|-------------|------|----------|-----------|
| REQUEST | `"current_version"` | Current version of the application, e.g. `"1.2.3"` | Yes |
| RESPONSE (update available) | `"update_available"` | `"true"` | Yes |
| RESPONSE (update available) | `"new_version"` | Update version, e.g. `"1.3.0"` | Yes |
| RESPONSE (update available) | `"release_notes"` | Brief description of changes | No |
| RESPONSE (no update) | `"update_available"` | `"false"` | Yes |

---

### UPDATE_CONSENT

| Direction | Key | Meaning | Required |
|-------------|------|----------|-----------|
| (client action) | `"accepted"` | `"true"` or `"false"` | Yes |

> UPDATE_CONSENT - **purely client action** (user consent to installation).
> It is not sent to the server. This is a signal from UI → UpdaterWorker via AppManager.
> If the user has agreed, the UpdaterWorker initiates an UPDATE_DOWNLOAD REQUEST.

---

### UPDATE_DOWNLOAD

Update file transfer protocol:

**Initiation (client → server):**
```
subsystem: PROTOCOL
protact:   UPDATE_DOWNLOAD
status:    REQUEST
add_data:  { "version": "1.3.0" }   // version we want to download
```

**The server sends each update file as a separate message:**
```
subsystem: PROTOCOL
protact:   UPDATE_DOWNLOAD
status:    NOTIFICATION
add_data:
  "file_path":   "uniter_setup_1.3.0.exe"   // file name with extension (full, how to save)
  "presigned_url": "https:// minio.../..." // temporary download URL
  "file_size":   "15728640"                 // size in bytes (for progress bar)
  "sha256":      "a3f1..."                  // SHA-256 for post-download verification
  "file_index":  "1"                        // serial number of the file in the update package
  "files_total": "3"                        // total files in the package
```

**After all files have been sent, the server closes the stream:**
```
subsystem: PROTOCOL
protact:   UPDATE_DOWNLOAD
status:    SUCCESS
add_data:
  "version":     "1.3.0"
  "files_total": "3"                        // final number (for comparison with the received one)
```

**The client downloads each file via MinIOConnector via presigned_url.**
Verification: SHA-256 of the downloaded file == `add_data["sha256"]` from NOTIFICATION.
If there is a discrepancy, a repeated request is made (a new UPDATE_DOWNLOAD REQUEST for a specific file).

| Direction | Key | Required |
|-------------|------|-----------|
| REQUEST | `"version"` | Yes |
| NOTIFICATION | `"file_path"` | Yes |
| NOTIFICATION | `"presigned_url"` | Yes |
| NOTIFICATION | `"file_size"` | No |
| NOTIFICATION | `"sha256"` | Yes |
| NOTIFICATION | `"file_index"` | Yes |
| NOTIFICATION | `"files_total"` | Yes |
| SUCCESS | `"version"` | Yes |
| SUCCESS | `"files_total"` | Yes |

---

## Kafka messages (KafkaConnector → AppManager)

All messages from Kafka have `subsystem != PROTOCOL` and one of the statuses:

| MessageStatus | Meaning | Who receives | add_data |
|--------------|-------|-------------|---------|
| `NOTIFICATION` | Change made by **other** user | DataManager | `"kafka_offset"`: current offset (string) |
| `SUCCESS` | Confirmation of **your** CUD operation through the server | DataManager | `"kafka_offset"`: current offset; `"request_id"`: echo from the original request |

> `"kafka_offset"` is added automatically by KafkaConnector to each message.
> DataManager uses it only for diagnostics; permanent storage offset -
> responsibility of KafkaConnector (OS Secure Storage).

---

## Rules for working with add_data

1. Keys - strictly from this document. String literals in code are a source of errors.
In the future, all keys will be placed in `contract/add_data_keys.h` as `constexpr`.
2. Values ​​are always `QString`. Numbers are serialized by `QString::number(x)`,
Boolean - `"true"` / `"false"`.
3. A missing optional key means "not specified" - not an error.

---

## Subsystem::GENERATIVE - CRUD based on the resources of generative subsystems

The context of the message is determined by the triple: `subsystem=GENERATIVE`, `GenSubsystem`, `GenSubsystemId`.

### Creating a generative subsystem (Subsystem::MANAGER)

```
subsystem=MANAGER, crudact=CREATE, resource_type=PRODUCTION | INTEGRATION
```
The server returns the resource with the `id` filled in. Kafka sends out a NOTIFICATION to all participants.
DataManager registers the new subsystem, ConfigManager generates the UI tab.

---

### GenSubsystem::PRODUCTION

**Production task (`PRODUCTION_TASK`):**
```
subsystem=GENERATIVE, GenSubsystem=PRODUCTION, GenSubsystemId=<id>
crudact=CREATE|UPDATE|DELETE, resource_type=PRODUCTION_TASK
```
Optional `add_data` keys for CREATE:

| Key | Meaning |
|------|---------|
| `"snapshot_id"` | ID of the PDM::Snapshot from which the composition is taken |
| `"snapshot_version"` | Photo version |

**Stock position (`PRODUCTION_STOCK`):**

`ProductionStock` is a separate resource containing a link to `MaterialInstance` by id and quantity.
Stored in the table `production_stock(id, production_id, material_instance_id, quantity)`.

```
subsystem=GENERATIVE, GenSubsystem=PRODUCTION, GenSubsystemId=<id>
crudact=CREATE|UPDATE|DELETE, resource_type=PRODUCTION_STOCK
resource = ProductionStock { id, material_instance_id, quantity }
```
`add_data = {}`

---

### GenSubsystem::INTERGATION

The integration subsystem does not contain a materials warehouse.
The only resource is `INTEGRATION_TASK`, which describes what is being integrated and with whom.

```
subsystem=GENERATIVE, GenSubsystem=INTERGATION, GenSubsystemId=<id>
crudact=CREATE|UPDATE|DELETE, resource_type=INTEGRATION_TASK
```
`add_data = {}`
