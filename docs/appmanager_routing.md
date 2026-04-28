# Routing messages inside AppManager

This document describes the `UniterMessage` dispatch rules
in the class `uniter::control::AppManager` - “above” (to `DataManager` /
`FileManager`) and "below" (into network classes). It complements
[`docs/fsm_appmanager.md`](fsm_appmanager.md) and
[`docs/add_data_convention.md`](add_data_convention.md), describing the behavior
specifically those two slots through which all AppManager traffic passes.

---

## General scheme

```
                 ┌─────────────────────────────────────────────────┐
                 │                 AppManager                      │
                 │                                                 │
   UI (AuthWidget/                                                 │
dynamic ──► onSendUniterMessage ──► dispatchOutgoing ──┬──│──► signalSendToServer → ServerConnector
widgets) │ │
   FileManager   ──►                                            ├──│──► signalSendToMinio    → MinIOConnector
                                                                │  │
   FSM (enter-*) ────────────────────────────────────────────── ├──│──► signalSendToServer   → ServerConnector
│ │ (bypassing onSendUniterMessage)
                                                                │  │
   ServerConnector ─┐                                           │  │
   KafkaConnector   ├─► onRecvUniterMessage                     │  │
   MinIOConnector   ┘                 │                         │  │
                                      ▼                         │  │
                            handleInitProtocolMessage  (FSM) ───┘  │
                            handleReadyProtocolMessage ────────────┼──► signalForwardToFileManager → FileManager
                            handleReadyCrudMessage ────────────────┼──► signalRecvUniterMessage    → DataManager
                                                                   │
                                                                   ▼
```

Both directions pass through one slot - `onRecvUniterMessage` for
incoming and `onSendUniterMessage` for outgoing; inside each slot
AppManager independently decides which bus to send
message. Network and subsystem classes do not filter traffic - they do
AppManager's responsibility.

---

## AppManager states important for routing

A detailed description of FSM is in [`fsm_appmanager.md`](fsm_appmanager.md). For
There are two important routing axes:

* `m_appState`:
  - `IDLE / NET_CONNECTING / IDLE_AUTHENIFICATION / AUTHENIFICATION /
KAFKA / SYNC` - **before READY** (initialization);
- `READY` - regular work with the user;
- `SHUTDOWN` - nothing is routed.
* `m_netState`: `ONLINE / OFFLINE` - in `OFFLINE` AppManager mutes
both tires.

In the following, “before READY” refers to all states
`m_appState != READY` (except SHUTDOWN, which is handled separately).

---

## Signals and slots AppManager

### Incoming signals from AppManager (up)

| Signal | Recipient | Issuer terms |
| --- | --- | --- |
| `signalRecvUniterMessage(msg)` | `DataManager::onRecvUniterMessage` | `READY` + `subsystem != PROTOCOL` (CRUD) |
| `signalForwardToFileManager(msg)` | `FileManager::onForwardUniterMessage` (TODO, not yet included in `main.cpp`) | `READY` + `subsystem == PROTOCOL` + `protact ∈ { GET_MINIO_PRESIGNED_URL, GET_MINIO_FILE, PUT_MINIO_FILE }` |

### Outgoing signals from AppManager (down)

| Signal | Recipient | Destination |
| --- | --- | --- |
| `signalSendToServer(msg)` | `ServerConnector::onSendMessage` | AUTH, `GET_KAFKA_CREDENTIALS`, `FULL_SYNC`, `GET_MINIO_PRESIGNED_URL`, any CRUD, `UPDATE_*` |
| `signalSendToMinio(msg)` | `MinIOConnector::onSendMessage` | `GET_MINIO_FILE`, `PUT_MINIO_FILE` |

All persistent connections of these signals are made in `main.cpp`
(section “AppManager ↔ Network Layer”).

### Mains input slot

| Slot | Followers |
| --- | --- |
| `onRecvUniterMessage(msg)` | `ServerConnector::signalRecvMessage`, `KafkaConnector::signalRecvMessage`, `MinIOConnector::signalRecvMessage` |

### Input slot from UI/subsystems

| Slot | Followers |
| --- | --- |
| `onSendUniterMessage(msg)` | `AuthWidget` (before READY), dynamic subsystem widgets (in READY), `FileManager` (in READY) |

---

## Upward routing rules (`onRecvUniterMessage`)

Message sources: `ServerConnector` (server responses and errors),
`KafkaConnector` (CRUD-`NOTIFICATION`, as well as `SUCCESS` confirmations),
`MinIOConnector` (responses to GET/PUT files).

Rules in order of priority:

1. **`m_netState == OFFLINE`** - any incoming is ignored with the log.
2. **`subsystem == PROTOCOL`**:
   - **`m_appState == READY`** → `handleReadyProtocolMessage(msg)`:
- if `protact` falls into `isMinioProtocolAction(...)` —
issue `signalForwardToFileManager` (→ FileManager);
- otherwise (updating Kafka credentials, `UPDATE_*`) - for now
We log and do not send anything (TODO).
   - **`m_appState != READY`** → `handleInitProtocolMessage(msg)`:
part of the initialization process, no forwarding needed - FSM
itself responds to `AUTH RESPONSE`, `GET_KAFKA_CREDENTIALS RESPONSE`
and `FULL_SYNC SUCCESS`.
3. **`subsystem != PROTOCOL`** (CRUD):
- **`m_appState == READY`** → `handleReadyCrudMessage(msg)` issues
`signalRecvUniterMessage` (→ DataManager). Typically the source is
`KafkaConnector` with `MessageStatus = NOTIFICATION/SUCCESS`.
- **`m_appState != READY`** — CRUD should not be in initialization,
ignored with the log.

Helper function:

```cpp
static bool AppManager::isMinioProtocolAction(contract::ProtocolAction a);
// true for GET_MINIO_PRESIGNED_URL, GET_MINIO_FILE, PUT_MINIO_FILE.
```

---

## Down routing rules (`onSendUniterMessage`)

Message sources:

* FSM itself in enter-actions `enterAuthenification` /
`enterKafka` / `enterSync` - uses `signalSendToServer` **directly**,
bypassing this slot;
* `AuthWidget` - sends AUTH REQUEST (before READY);
* dynamic subsystem widgets and `FileManager` - CRUD and
`MINIO`-protocol requests in READY.

### In `READY` state

1. **`m_netState == OFFLINE`** - the message is ignored (TODO: buffering).
2. Otherwise, `dispatchOutgoing(msg)` is called:
   - `subsystem != PROTOCOL` (CRUD) → `signalSendToServer`;
   - `subsystem == PROTOCOL`:
- `protact == GET_MINIO_FILE` or `PUT_MINIO_FILE` →
       `signalSendToMinio`;
     - `protact == GET_MINIO_PRESIGNED_URL` → `signalSendToServer`
(presigned URL is issued by the server, not MinIO);
- any other (`AUTH`, `GET_KAFKA_CREDENTIALS`, `FULL_SYNC`,
       `UPDATE_*`) → `signalSendToServer`.

### Before `READY`

**only** `AUTH REQUEST` from UI is skipped - it is saved as
`m_authMessage`, and if FSM is already in `AUTHENIFICATION` and `ONLINE`, immediately
`signalSendToServer` and `ProcessEvent(AUTH_DATA_READY)` are issued.

Any other message before READY from this slot is **rejected with a log**.
In particular, `GET_KAFKA_CREDENTIALS` and `FULL_SYNC` do not come here - FSM
forms them inside its enter-actions and issues `signalSendToServer`
directly.

---

## Why does FSM bypass `onSendUniterMessage`

Before READY, the only external channel is AUTH REQUEST from `AuthWidget`;
all other PROTOCOL requests are initiated by the FSM itself. To
`onSendUniterMessage` remains the "UI gate" with a simple and strict
invariant (“before READY – only AUTH”), enter-actions FSM are issued
`signalSendToServer` directly. This keeps the slot unique:

* **UI entry point** - `onSendUniterMessage`;
* **output bus to the server** - `signalSendToServer`, they can
handle both slot and enter-actions FSM.

---

## Pivot table

| Direction | Source | State | `subsystem` | `protact` | Where does it go |
| --- | --- | --- | --- | --- | --- |
| Up | Server/Kafka/MinIO | OFFLINE | any | any | *drop* |
| Up | Server | !READY | PROTOCOL | AUTH / GET_KAFKA_CREDENTIALS / FULL_SYNC | FSM (internal) |
| Up | Kafka | READY | CRUD | — | DataManager |
| Up | Server | READY | PROTOCOL | GET_MINIO_PRESIGNED_URL | FileManager |
| Up | MinIO | READY | PROTOCOL | GET_MINIO_FILE / PUT_MINIO_FILE | FileManager |
| Up | Server | READY | PROTOCOL | UPDATE_* / GET_KAFKA_CREDENTIALS | *log, TODO* |
| Down | UI (AuthWidget) | AUTHENIFICATION+ONLINE | PROTOCOL | AUTH REQUEST | ServerConnector |
| Down | UI/FileManager | READY+ONLINE | CRUD | — | ServerConnector |
| Down | FileManager | READY+ONLINE | PROTOCOL | GET_MINIO_PRESIGNED_URL | ServerConnector |
| Down | FileManager | READY+ONLINE | PROTOCOL | GET_MINIO_FILE / PUT_MINIO_FILE | MinIOConnector |
| Down | UI | READY+ONLINE | PROTOCOL | UPDATE_* | ServerConnector |
| Down | FSM (enter-actions) | !READY+ONLINE | PROTOCOL | GET_KAFKA_CREDENTIALS / FULL_SYNC / AUTH | ServerConnector (directly, bypassing the slot) |

---

## Related files

* `src/uniter/control/appmanager.h` - declaration of signals and
`dispatchOutgoing` / `handle*` methods;
* `src/uniter/control/appmanager.cpp` - implementation of the rules, this
the document reproduces the comments above the methods verbatim;
* `src/uniter/main.cpp` - constant `QObject::connect` between
AppManager and network layer;
* `src/uniter/network/miniconnector.cpp` - outgoing handler
  `GET_MINIO_FILE` / `PUT_MINIO_FILE`;
* `docs/add_data_convention.md` - `add_data` contracts for each
`ProtocolAction` including `PUT_MINIO_FILE`;
* `docs/fsm_appmanager.md` - actual FSM and enter-actions.
