# FSM AppManager

Diagram: [FSM-AppManager.pdf](FSM-AppManager.pdf)

## Principles

1. **State name = entry action.** Names reflect entry-action, not event
(`AUTHENIFICATION` = "auth data request", `KCONNECTOR` = "KafkaConnector initialization",
`DBCLEAR` = “database clearing”).
2. **One entry-action per state.** If there are several actions, enter
separate state (hence `KCONNECTOR` and `DBCLEAR`).
3. **Network requests - only through `UniterMessage`.** Entering a state that
requests something from the server, generates a `UniterMessage` and issues
`signalSendUniterMessage`. There are no separate signals of the “full-sync request” type.
4. **Online/offline are orthogonal to `AppState`.** Network loss only changes `NetState`
and blocks the UI; `AppState` remains. But the semantics of the response to network return
depends on the type of condition:
- **Network states** (have an offline mirror on the diagram):
     `AUTHENIFICATION`, `IDLE_AUTHENIFICATION`, `KAFKA`, `SYNC`, `READY` —
when returning online entry-action **repeats** (resume request).
- **Local states** (no offline mirror):
     `DBLOADING`, `CONFIGURATING`, `KCONNECTOR`, `DBCLEAR` —
when returning online entry-action **does not repeat** (internal operations
are not network dependent).
5. **Delayed entry into the network state.** If at the moment of logical transition
to the network state (for example `KCONNECTOR → KAFKA` after `OFFSET_RECEIVED`)
network in `OFFLINE`, entry-action is deferred and will be executed when
`NET_CONNECTED` via `reenterOnReconnect()`.
6. **`SHUTDOWN` is reachable from any state** (saving settings and buffers).

## States

| State | Type | Input action | What's being sent |
|---|---|---|---|
| `IDLE` | — | — | — |
| `STARTED` | — | request a connection from the network class | `signalMakeConnection` |
| `AUTHENIFICATION` | network | requesting auth data from the widget **or** sending those already received | `signalFindAuthData` / `UniterMessage(AUTH REQUEST)` |
| `IDLE_AUTHENIFICATION` | network | waiting for server response | — |
| `DBLOADING` | local | signal to the user about success + database initialization | `signalAuthed(true)`, `signalLoadResources(userhash)` |
| `CONFIGURATING` | local | calling the configuration manager | `signalConfigProc(User)` |
| `KCONNECTOR` | local | initialization of `KafkaConnector` for the user | `signalInitKafkaConnector(userhash)` |
| `KAFKA` | network | request from the server for relevance offset | `UniterMessage(GET_KAFKA_CREDENTIALS REQUEST)` + `add_data["offset"]` |
| `DBCLEAR` | local | complete clearing of database tables via `DataManager` | `signalClearDatabase` |
| `SYNC` | network | full-sync request from the server | `UniterMessage(FULL_SYNC REQUEST)` |
| `READY` | network | subscription to Kafka broadcast topic | `signalSubscribeKafka` |
| `SHUTDOWN` | — | saving settings and buffers (TODO) | — |

## Events

| Event | Source |
|---|---|
| `START` | `AppManager::start_run()` |
| `NET_CONNECTED` / `NET_DISCONNECTED` | network class (`onConnectionUpdated(bool)`) |
| `AUTH_DATA_READY` | internal: after sending `AUTH REQUEST` to the server |
| `AUTH_SUCCESS` / `AUTH_FAIL` | `UniterMessage(AUTH RESPONSE)` from server |
| `DB_LOADED` | `DataManager::signalResourcesLoaded` → `onResourcesLoaded` |
| `CONFIG_DONE` | `ConfigManager::signalConfigured` → `onConfigured` |
| `OFFSET_RECEIVED` | `KafkaConnector::signalOffsetReady(QString)` → `onKafkaOffsetReceived` |
| `OFFSET_ACTUAL` / `OFFSET_STALE` | `UniterMessage(GET_KAFKA_CREDENTIALS RESPONSE)` + `offset_actual` |
| `DB_CLEARED` | `DataManager::signalDatabaseCleared` → `onDatabaseCleared` |
| `SYNC_DONE` | `UniterMessage(FULL_SYNC SUCCESS)` from server |
| `LOGOUT` | UI → `onLogout` |
| `SHUTDOWN` | UI/system → `onShutDown` |

## Golden path

```
IDLE
 └─START→ STARTED
          └─NET_CONNECTED→ AUTHENIFICATION
                           └─AUTH_DATA_READY→ IDLE_AUTHENIFICATION
                                              └─AUTH_SUCCESS→ DBLOADING
                                                              └─DB_LOADED→ CONFIGURATING
                                                                           └─CONFIG_DONE→ KCONNECTOR
                                                                                          └─OFFSET_RECEIVED→ KAFKA
                                                                                                            ├─OFFSET_ACTUAL→ READY
                                                                                                            └─OFFSET_STALE→ DBCLEAR
                                                                                                                            └─DB_CLEARED→ SYNC
                                                                                                                                         └─SYNC_DONE→ READY
```

From `READY`:
- `LOGOUT` → `IDLE_AUTHENIFICATION` (cleaning resources, unbinding the user).
- `SHUTDOWN` → final state.

## Signal transitions in `main.cpp`

See blocks `=== 2. Network state management ===`, `=== 2.1 KafkaConnector ===`
and `=== 3. Resource and database management ===` in `src/uniter/main.cpp`.

## Working with `UniterMessage` in FSM

All communication with the server is via `UniterMessage`. Formats (see also
`docs/add_data_convention.md` and `src/uniter/contract/uniterprotocol.h`):

| State | Request | Server response |
|---|---|---|
| `AUTHENIFICATION` | `PROTOCOL` + `AUTH` + `REQUEST` + `add_data["login","password_hash"]` | `AUTH RESPONSE` + `resource = Employee` or `error` |
| `KAFKA` | `PROTOCOL` + `GET_KAFKA_CREDENTIALS` + `REQUEST` + `add_data["offset"]` | `GET_KAFKA_CREDENTIALS RESPONSE` + `add_data["offset_actual"]` |
| `SYNC` | `PROTOCOL` + `FULL_SYNC` + `REQUEST` | first `CRUD CREATE` flow through Kafka, then `FULL_SYNC SUCCESS` |

## Stub implementation status

| Class | Status |
|---|---|
| `MockNetManager` | stub; responds to `AUTH`, `GET_KAFKA_CREDENTIALS` (`offset_actual=true`), `FULL_SYNC REQUEST` (`SUCCESS`), CRUD echo |
| `KafkaConnector` | stub; `onInitConnection(hash)` gives an arbitrary offset via `QTimer::singleShot(0, ...)`; `onSubscribeKafka()` issues `signalKafkaSubscribed(true)` |
| `DataManager::onClearDatabase` | stub; immediately issues `signalDatabaseCleared` (TODO: real DROP/TRUNCATE of tables) |
| `ServerConnector` | **not implemented**, the role is played by `MockNetManager` |
| `FileManager` | **not implemented** |

## TODO (protocol/resources)

- User hash: `makeUserHash()` now returns `id` in ASCII. Need a real one
hash (e.g. SHA256 from `id + salt`) for the key in OS Secure Storage.
- In `DataManager::onClearDatabase` - real database cleaning (leave only
table structure).
- `KafkaConnector`: replace stub with librdkafka client + OS Secure Storage
for persistence offset.
