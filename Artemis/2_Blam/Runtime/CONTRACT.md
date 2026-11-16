# Artemis.Blam.Runtime

Group `2_Blam` - pure static library - the live objects, players, interaction and game state of one tick.

## Responsibility
Interprets the Snapshot of a tick with the static model of the map: the alive objects (with bone matrices and damage sections),
the players of the player table, the interaction of every local player, and the game globals (clock, phase, scores) with the new
entries of the engine's event ring.

## Does not
- Read the process or follow pointers: `Engine.Capture` already copied everything into the Snapshot. No type holds a process
  address.
- Classify objects or build entities: that is `Perception`.

## Input and output
- Input: a `Blam::Layout::Snapshot::Type::Snapshot` and `Blam::MapModel::Layer`.
- Output: the stores of `Blam::Runtime::Layer` (`ObjectTableStore`, `BoneMatricesStore`, `DamageSectionsStore`, `PlayerTableStore`,
  `InteractionStore`, `GameStore`), one published value per tick.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Blam.Runtime.Layer` | The facade: stores and services; `Cleanup()` forgets the map. |
| `Blam.Runtime.Object.Type`, `Blam.Runtime.Object.State` | `AliveObject`, the object table, bone matrices, damage sections. Weapon: ammo per magazine (`Magazines`), `IsOverheated`, `BusyTimer` (the weapon cannot fire), `Magazine0ReloadState`, charge (`Common.Semantic` `ChargeState`), tracking (`TrackedObject` of any kind, `TrackingState`). Projectile: `OwnerPlayer`, `OwnerBiped`, `ShooterRoot`, `ParentNode`. Biped: `CrouchFraction` (unit +0x2AC, the real pose), `ContactMaterial` (0xFFFF without contact, `k_NoContact`) and `OnStructure` (bipd +0xAD4 and +0xAD7), `GroundObjectHandle` (lags one tick when landing and drops to none when standing still on an object, D260). Every object: `IsInPhantom` (obje +0xDC bit 16: inside a phantom volume that pushes it, set the tick it enters and cleared the tick it leaves, D261). |
| `Blam.Runtime.Player.Type`, `Blam.Runtime.Player.State` | `AlivePlayer` and the player table. `RespawnTicks` is read from the player row (+0x1BC, u16 widened) and `Traits` (the decoded block at +0x328, D262) and `PowerupPickupTicks` (the three game times of the powerup pickups at +0x300, raw, D263) for every player of the table, alive or dead. |
| `Blam.Runtime.Interaction.Type`, `Blam.Runtime.Interaction.State` | Interaction of each local player. |
| `Blam.Runtime.Game.Type`, `Blam.Runtime.Game.State` | Game globals of the tick (clock, phase, round, round clock and time left, game variant round, respawn and powerup settings and score to win, team mapping and scores, megalo objects), event cursor and dropped events. |

## Depends on
`Common`, `Blam.Layout`, `Blam.Map`, `Blam.MapModel`.

## State between ticks
- The objects and players are kept by handle and refreshed every tick; a handle whose salt changed is replaced.
- `ObjectTableStore` publishes into a small pool of tables (S2.O, D276): the table of a tick is written over one nobody holds, so a reader that holds a table sees it unchanged, and a steady tick allocates none. Since OT.1.B (D303) it copies only what changed: the store keeps per slot (handle index) the live row and the tick of its last change, a log of the objects that left, and for each pooled table the tick it was published at, so a publish erases from the table it writes over the rows of the dead and writes the rows added or changed since its tick (a new table, or one the log cannot bring up to date, is a copy of the whole table). The result is the same as a copy of the whole table and the type `ObjectTable` is unchanged. `Edit(editor)` holds the lock once so `ObjectTableService` can use the row functions (`FindRow`, `AddRow`, `RemoveRow`, `MarkChanged`, `SlotHandle`) by slot with no search and no lock per object. `AddObject` and `RemoveObject` keep working on their own lock. `BoneMatricesStore` and `DamageSectionsStore` keep their buffers between ticks and their tables between ticks too: `Write(handle)` gives the table to fill in place and shows it, `Discard(handle)` takes it back, `Get` gives a table until it is discarded or written again, and `Prune()` drops the buffers of the handles with no table when they outnumber the tables twice over. The service discards the tables of the dead and of every dirty object before it decodes it, so what `Get` gives is what the last decode of the object found.
- `ObjectTableService` updates the table in one pass over the objects of the snapshot, by slot: a known handle updates its row, a new one (or a slot reused under another salt, which makes the old object leave) adds a row, and the slots of the last tick that the snapshot did not have remove theirs. A row is marked changed when the object is new or dirty or its Placement or WorldOrigin moved. It keeps per slot (`RawCopies`, AI thread only) the datum bytes of the last tick that read it (OT.1, D303). An object with the same handle and the same bytes (size and memcmp, no mask) is clean: it is not decoded again and keeps its row (apart from Placement and WorldOrigin, recomputed every tick) and its bone and damage tables. A new object, a reused slot (another salt) or any changed byte is dirty and is decoded in full. Everything `UpdateObjectData` reads comes from the datum, so a clean object would decode to the same values. `Cleanup` does not touch the copies (it runs on another thread): `AddObject` drops the copy of a new object, which makes it dirty. The light service (Q101) uses the same rule on its own copies.
- The light snapshot of a tick without pipeline (Q101) is decoded by a second `ObjectTableService` and a second `GameService` into their own light stores (`m_LightObjectStore`, `m_LightBoneMatricesStore`, `m_LightDamageSectionsStore`, `m_LightGameStore`), never into the stores of the full tick. The light object table has only the weapons, and the light game only the clock, the round, the team mapping and scores and the megalo objects (no event ring, no game variant). `ObjectTableService` skips the handle audit on a light snapshot, which has not the objects the links point to.
- `GameStore` keeps the cursor of the event ring (last sequence read) and the count of entries overwritten before they were read.

## Threads
Updates run on the AI thread, called by `Bridge.Codec.Pipeline.System`. Readers on other threads use the published tables and
never wait.

## Errors
A region or table that was not captured leaves its output empty (or as it was) for that tick; a datum of unknown kind is skipped.
`ObjectTableService` counts per tick (`HandleAudit`) what the object handles the runtime follows resolve to: matching, old salt (the object was deleted and its slot reused, normal in play: only counted), free slot, and salt 0 (WARNING once per map load).

## Requirements
Pure: no process, no address (checked by `Tools/Tests/test_components.py`). Deterministic, like the rest of the pipeline.

## Tests
`RuntimeTests` (hand-made Snapshots: a biped, a weapon, a player, the event ring) in `Artemis.Tests`, and `ReplayTests.Recordings`.
