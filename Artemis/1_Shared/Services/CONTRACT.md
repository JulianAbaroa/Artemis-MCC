# Artemis.Services

Group `1_Shared` - static library with side effects (disk) - logs, settings, preferences and telemetry of the DLL.

## Responsibility
The services every impure project shares: the log sink (in-memory buffer for the UI plus the log file), the runtime settings and the
AppData folder (`config.ini`), the user preferences file (`user_preferences.cfg`) and the telemetry counters.

## Does not
- Know the engine or the tick pipeline.
- Reach the pure projects: they receive a `Common::Log::System::Log` and plain values, never a `Services` store.

## Input and output
- Input: log messages, settings changes, preference sections registered by other projects, telemetry counts from hooks and threads.
- Output: files on disk, the log buffer and the per-second telemetry rates the UI shows.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Services.Layer` | The facade: owns `SettingsStore`, `LogsStore`, the `LogsSink`, `TelemetryStore` and their services. |
| `Services.Logs.Type`, `Services.Logs.State` | The log entries and the bounded, thread-safe buffer the log window reads. |
| `Services.Settings.State`, `Services.Settings.System` | Shared runtime flags and paths; the AppData folder and its `config.ini` flag. |
| `Services.Preferences.System` | `PreferencesService`: other projects register a section of keys; saved to and loaded from AppData. |
| `Services.Telemetry.State` | Lock-free counters (dropped ticks, read wait, sweep time, presents) and the rates derived from them, plus cumulative counters that `Update` never resets: ticks, engine time, read wait, sweeps, a 25 us histogram of the sweep duration and the time of each sweep stage (names registered once by `Mod`). `CopyPerf` takes a photo and restarts the maxima (the sweep, and the highest first-unused slot and live count of the engine object table, which `Capture` records with `RecordObjectTable`). |
| `Services.Telemetry.Type` | `PerfSnapshot` (a photo of the cumulative counters) and the histogram constants. |
| `Services.Telemetry.System` | `TelemetryService`, and the pure functions over photos: `SweepBucket`, `HistogramPercentileNs`, `OtherStageNs`, `Subtract`. |

## Depends on
`Common`.

## State between ticks
The settings and preferences files, the log file and the log buffer. Keys on disk never change meaning or name.

## Threads
`LogsSink::Write` and the settings flags are thread-safe. Telemetry counters are fed from any thread without locks;
`TelemetryService::Update` is called periodically from one thread.

## Errors
A file that cannot be written is skipped; nothing blocks the game. Preferences do nothing while AppData is disabled, and a missing
file keeps the defaults.

## Requirements
- Same keys on disk as before (settings, preferences).
- Never called on a hot path of the game thread except the lock-free telemetry counters.

## Tests
`TelemetryTests` covers the cumulative counters, the histogram and the pure functions. The rest has no unit tests of its own: `EngineLayerTests`, `MatchConfigTests` and `TransportTests` build it as a dependency.
Logs, settings and preferences are checked in game by the bench.
