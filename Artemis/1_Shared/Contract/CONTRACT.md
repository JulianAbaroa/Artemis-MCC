# Artemis.Contract

Group `1_Shared` - pure static library, generated - the layout C++ and Python share over the bridge.

## Responsibility
The binary contract of the bridge with the Python client: the POD structs of the `BridgeTick` (players, entities, the match with
its round, team scores, round time, round limit and score to win, the flags `BridgeObjective` and the stands `BridgeStand` of a capture game, the weapons `BridgeWeapon`, the projectiles `BridgeProjectile` the explosives `BridgeExplosive` and the lifts `BridgeLift`, the events with both
sequences) and the `BridgeAction`, their constants, a `static_assert` per offset, the layout as JSON and its hash (`k_BridgeLayoutHash`), which Python compares with its own.

## Does not
- Flatten a tick or decode an action: that is `Bridge.Codec`.
- Open shared memory or talk to the client: that is `Bridge.Transport`.
- Get edited by hand: `Tools/TagTranspiler/GenerateBridge.py` writes it from `Data/Bridge/Bridge.xml`, together with
  `Agent/artemis/bridge_layout.py` (numpy dtypes with the same offsets).

## Input and output
Types and constants.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Contract.Bridge.Type` | Bridge structs, enums and constants, `k_BridgeLayoutVersion`, `k_BridgeLayoutHash`, `k_BridgeLayoutJson`. |

## Depends on
`Common`.

## State between ticks
None.

## Threads
Not applicable (types only).

## Errors
A client built for another layout sees a different hash and refuses to connect.

## Requirements
- One source: only `GenerateBridge.py` writes this project and the Python module.
- The layout changes only on purpose: every `.tdump` embeds the JSON of the layout it was written with.
- Flags and stands are two structs and not one block with a kind: a stand has three fields and a flag ten. Both arrays hold
  `k_BridgeMaxObjectives` (8) rows, the team arrays of the match `k_MaxTeams` (8), and the header carries the counts in use.
- Weapons and projectiles hold `k_BridgeMaxWeapons` and `k_BridgeMaxProjectiles` (256) rows each, after the events; the header carries
  `WeaponCount` and `ProjectileCount` and the flags `WeaponsTruncated` and `ProjectilesTruncated` tell when rows were cut.
- Explosives hold `k_BridgeMaxExplosives` (64) rows after the projectiles (D259): radius and damage of the explosion of the tags; the
  header carries `ExplosiveCount` after `MapId` and the flag `ExplosivesTruncated` tells when rows were cut.
- Lifts hold `k_BridgeMaxLifts` (64) rows after the explosives (D261): up to `k_BridgeMaxLiftStages` (3) stages of the tag (`BridgeLiftStage`: marker
  position and direction in the world, the push along it and toward its axis, who it affects) and the measured exit of the last biped
  (`HasExit`, `ExitTick`, `ExitBiped`, `ExitVelocity`). The header carries `LiftCount` after `ExplosiveCount`, in the padding it already had, and the
  flag `LiftsTruncated` tells when rows were cut.
- The header also carries the `MapId` of the scnr of the loaded map (engine id, 0 while no map).
- Each player row carries `RespawnTicks` (ticks left until it respawns, 0 while alive), right after `BipedHandle`: it takes the 2 bytes of padding
  the u8 block already had, so the layout adds 4 bytes per player and no new padding.
- Each player row carries the ground and the crouch of its body (D260): `Ground` (`BridgeGround`), `IsGrounded` and `IsCrouching` after `IsAbilityActive`
  (13 u8 and 3 bytes of padding), and `AirborneTicks`, `GroundObject`, `CrouchFraction` and `OwnVelocity` after `TicksSinceDamage`. They fill the
  4 bytes of padding that sat before `AppliedDecision`, so the row grows 24 bytes. All of them are 0 without a body.

## Tests
The `static_assert` of every offset (at compile time). `Tools/Tests/test_generators.py` and `test_components.py` check that the
generator writes this project. `ReplayTests.Recordings` compares the flattened output byte for byte.
