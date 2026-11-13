# Artemis.Engine.Match

Group `5_Engine` - static library inside the game process - the cycle of matches and their recorded configurations.

## Responsibility
Sees and drives the match cycle of MCC: copies the vtable of the engine interface (`EngineInterfaceDetour`) to know when an engine
starts and is destroyed, keeps a library of recorded match configurations (`<storage>/MatchConfigs/<name>.bin`) and transplants
the pending one into the next start, writes the match setup into that copy (`Engine.Base.MatchSetup`, D286: preset and map variant, restored whole if either fails; `BeginStart` runs on every start and the start also consumes a setup request no round break served), and presses START on the custom games lobby from the MCC lifecycle update
(`LifecycleUpdateDetour`).

## Does not
- Decide which match comes next: the bench or the Python client ask through `Mod`.
- Read the state of the tick.

## Input and output
- Input: a request to launch a recorded configuration, or to leave.
- Output: a launched match; the cycle state (lobby open, engines started and destroyed).

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Engine.Match.Layer` | The facade: config and cycle stores, `MatchConfigService` and the two detours. Owner only. |
| `Engine.Match.Type` | Engine interface vtable entries and the `LoadGameEngine` signature. |
| `Engine.Match.State` | `MatchConfigStore`, `MatchCycleStore` and `MapVariantDecodeStore` (Q106: one request to decode a `.mvar`, with the bytes of the file read by the one that asks, a ticket, and the result of that ticket given once; a newer request replaces the waiting one and the stale result is dropped; thread-safe). |
| `Engine.Match.System` | `MatchConfigService`: list, record, load and transplant configurations, `GetNameProblem` (why a name is not valid, the rule `ArmRecord` uses), `RenameConfig` and `DeleteConfig` (U.1, D289: both validate names with that rule, never overwrite another configuration, refuse the pending one and return the reason, empty on success; Delete moves the file to `MatchConfigs/_deleted/`, numbering a repeated name, so the listing, which does not enter subfolders, no longer shows it), and `ReadMapVariant` (the map variant block of a recorded configuration, without making it pending; Q98, D266). `MapVariantCatalogService` (Q106, D291) lists the `.mvar` files of every `<LocalAppDataLow>/MCC/LocalFiles/<xuid>/HaloReach/Map/` folder as `MapVariantEntry` (path, the name inside the file or the file name when the header has none, description, account folder, date), sorted by name, reading only the first bytes of each file and skipping, with a WARNING, what is not a map variant file; `ListIn` takes the LocalFiles folder (tests). It touches the disk: a user action, never every frame. The start hook calls `Engine.Base` `MatchSetupService` after the transplant: `BeginStart` and, when there is something to apply, `ApplyToStart`, which writes the preset and the map variant (at +0x1D308, only if it fits the one the copy carries). A setup that does not fit is Rejected with a WARNING, the copy is left as it was and the start gets the setup the engine held before. |

## Depends on
`Common`, `Services`, `Engine.Base` (and `Blam.Layout` through its presets and the map variant constants).

## State between ticks
The recorded configurations on disk (they hold the user's XUID: never in the repo) and the pending one; the counts of the cycle.

## Threads
The detours run on the MCC thread that updates the lifecycle. Stores are thread-safe for the bench and the UI. Listing the folder
happens on a user action, never every frame. The same hook (`LifecycleUpdateDetour`) serves the request of the `MapVariantDecodeStore` before anything else, in the lobby and in a match alike, calling `MapVariantDecodeService` of `Engine.Base`: the file was read by the requester, so the MCC thread never touches the disk.

## Errors
An incompatible configuration or the wrong screen does not launch; the reason is logged and visible in the cycle state.

## Requirements
The format of the recorded configurations stays readable.

## Tests
`MatchConfigTests` (configurations on disk), `MapVariantCatalogTests` (the listing of `.mvar` files over a temporary copy of the layout of MCC) and `MapVariantDecodeStoreTests` in `Artemis.Tests`; the cycle in game with the bench's rotation scenarios.
