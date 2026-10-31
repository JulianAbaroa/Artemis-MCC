# Artemis.Contract

Group `1_Shared` - pure static library, generated - the layout C++ and Python share over the bridge.

## Responsibility
The binary contract of the bridge with the Python client: the POD structs of the `BridgeTick` (players, entities, the match with
its round, team scores, round time, round limit and score to win, the flags `BridgeObjective` and the stands `BridgeStand` of a capture game, the weapons `BridgeWeapon` and the projectiles `BridgeProjectile`, the events with both
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

## Tests
The `static_assert` of every offset (at compile time). `Tools/Tests/test_generators.py` and `test_components.py` check that the
generator writes this project. `ReplayTests.Recordings` compares the flattened output byte for byte.
