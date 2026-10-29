# Artemis.Contract

Group `1_Shared` - pure static library, generated - the layout C++ and Python share over the bridge.

## Responsibility
The binary contract of the bridge with the Python client: the POD structs of the `BridgeTick` and the `BridgeAction`, their
constants, a `static_assert` per offset, the layout as JSON and its hash (`k_BridgeLayoutHash`), which Python compares with its own.

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

## Tests
The `static_assert` of every offset (at compile time). `Tools/Tests/test_generators.py` and `test_components.py` check that the
generator writes this project. `ReplayTests.Recordings` compares the flattened output byte for byte.
