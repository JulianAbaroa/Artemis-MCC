# Artemis.Views

Group `3_Domain` - pure static library - what one agent can know, built from the omniscient world.

## Responsibility
Builds the view of the agent's local player: its self (the player it controls and where it is), the affordances around it (what it
can pick up, enter or use) and the raycasts it sees with.

## Does not
- Take part in the Tick with `Oracle`: `Views.Oracle.*` is the pure comparison, counters and report text of the raycast oracle (R3, D272), a development tool. It lives here only because it needs `RaycastHit` and `Artemis.Tests` cannot reference `DevTools.*`. `Views::Layer`, the services and the rest of the pipeline never import it, only `DevTools.Workbench` does.
- Decide actions: that is the Python agent.
- Build the action: that is `Control`.
- Flatten anything for Python: that is `Bridge.Codec`.

## Input and output
- Input: the stores of `Perception::Layer`, `Blam::Runtime::Layer` and `Blam::MapModel::Layer` (world and raycaster).
- Output: `SelfStore`, `AffordanceStore` and `RaycastStore`, members of `Views::Layer`, one published value per tick. `RaycastSceneStore` is also a member, but it is state between ticks and not an output.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Views.Layer` | The facade: stores and services; `Cleanup()` forgets the map. |
| `Views.Self.Type`, `Views.Self.State` | The agent's player (`k_SelfLocalIndex`) and its position. |
| `Views.Affordance.Type`, `Views.Affordance.State` | What the agent can interact with, and how. |
| `Views.Raycast.Type`, `Views.Raycast.State` | The rays of the agent and what they hit, cast through one scene of all the collidable objects of the tick (D298, D299), with no radius around the agent. `RaycastSceneStore` keeps that scene between ticks, and objects at rest go to its fixed tree (D300). A static hit is a collision surface and carries its flags (`SurfaceFlags`) and global material (`Material`, -1 if none), a dynamic hit or a miss gives 0 and -1 (D272). `PlayerExclusions` (the exclusion of each row of the players) and `EyePoint` (where an agent looks from) are for the views per agent (F4.B). |
| `Views.Oracle.Type`, `Views.Oracle.System` | Dev tool only (R3, D272), outside the pipeline: `OracleSample` (a ray with the engine answers and ours), `RayDifference`, `OracleStats` and its counters; `Compare` (the engine without objects against `Raycaster`, and with objects against the tick: hit or not, distance beyond `k_DistanceTolerance`, kind, object handle, material and surface flags), `Record` (counters, worst 20, cost histogram), `Summarize`, `Percentile`, `FormatSummary`, `FormatReport`, `ResolveRootObject` and `RandomDirection`. Pure functions, no engine call. |

## Depends on
`Common`, `Blam.Layout`, `Blam.Map`, `Blam.MapModel`, `Blam.Runtime`, `Perception`.

## State between ticks
- `RaycastSceneStore` (`Views.Raycast.State`): the two level ray scene (a hierarchy per collision mesh of the map and two trees of instances, FIXED and MOBILE) and, by handle, the parts of every object with the `CollidablePartsKey` they were built from and `StillGathers`, the gathers in a row with the same key (0 in the gather where it changes or appears). An object whose key is the same keeps its parts without collecting them again (D299), one that is gone leaves. The key of an object is brought up to date in its entry, in place, from the row of the table the gather already holds (D301): no lookup per object and a field is copied only when it differs. The `SceneObject`s of a gather view the vectors of those entries (`std::span`), they do not copy them. An entry is replaced only for an object whose key changed, before its `SceneObject` is made, and only entries of objects not in the result are erased, so the views are valid until the next gather: nothing keeps a `SceneObject` past the arming and casting of its tick (the trees and `FixedPoses` hold their own copies of the poses). It is used on the AI thread. The boxes of the instance trees are the posed box of each part cut with the box of its sphere (RK.1, D306). `GatherSceneReach` collects once per agent the pieces that rays from one origin within a range can touch and `CastSceneRayReach` casts through them (same hit as `CastSceneRay`, which it falls back to for another origin, a longer ray, another exclusion or a scene armed again since the gather: the reach keeps the sorted handles of its exclusion and a `SceneStamp` of the scene, RV.1). A ray whose origin, direction or range is not a number misses in `CastScene` and `CastSceneRayReach`. `RaycastService` uses it for the perception rays.
- Rest and the fixed tree (D300). A gather marks an object as at rest if its placement is Fixed or Phased and its key did not change since the last gather (or it is new), or if `StillGathers` reached `k_RestGathers` (8). A change of key takes it out of rest in that same gather. `ArmFixedScene` keeps the FIXED tree: an object in it that is not at rest with the same poses any more goes to a sorted filter of handles that casts skip, and its current parts go to the MOBILE tree, without building. An object at rest that is not in the tree waits in the MOBILE tree. The tree is built again with every object at rest and an empty filter when something waits or is filtered and either it was never built, or `k_FixedRebuildGathers` (32, the least spacing) gathers passed since the last build and there are `k_FixedRebuildPending` (16) waiting (objects at rest not in the tree plus filtered handles) or the wait has lasted `k_FixedRebuildMaxGathers` (512) gathers (D301, `FixedRebuildRule`, `RaycastScene::FixedWaitingGathers` counts the gathers in a row with something waiting and starts over when nothing waits). What waits lives in the MOBILE tree and the filter, so the hits do not depend on when the tree is built. The MOBILE tree is armed in every gather from the objects the FIXED tree does not cover, so the parts of an object are always in exactly one of the two. When its pieces (handle, mesh and part index, in order) are the same as in the last arming only the poses changed, and `ArmMobileScene` keeps the topology and refits the boxes from the leaves up (`RefitBoundsBvh`, D302): the leaf boxes are the spheres of the parts, so it holds exactly what a build would and only the quality can get worse. It builds again when the pieces change, or when the area of its inner nodes after the refit is above `MobileRebuildRule::MaxArea` (`k_MobileRebuildArea`, 2) times the area it had when it was built. `RaycastScene` keeps `MobileBuiltArea`, `MobileAreaRatio` and the counters `MobileBuilds`, `MobileRefits` and `MobileDegraded`, and is emptied with the rest of the scene. The constants count gathers: Self gathers every tick today, and every decision with RS.2. The tie of two hits at the same distance goes by handle, part and triangle, which does not depend on the tree. `Cleanup()` runs on the lifecycle thread, so it only marks the store, and the next use empties it all: the hierarchies of the meshes point into the collision models of the map, so nothing of an old map may be used. The first tick after a load builds the hierarchies of the meshes (about 14 ms once).
- Every other store holds only the value of the last tick.

## Threads
Updates run on the AI thread, called by `Bridge.Codec.Pipeline.System` after `Perception`. Nothing blocks.

## Errors
An agent without a unit (dead, not spawned) gets an empty view for that tick.

## Requirements
- One agent per process today (the local player `k_SelfLocalIndex`); the same rules must hold for every agent once there are more.
- Deterministic and pure, like the rest of the pipeline.
- Helpers of the views per agent (F4.B, planned for F4.2, not in the pipeline yet), pure functions of `Views.Raycast.System`, internal to the project and measured by the offline bench (`DISABLED_RaycastBench`, table M6): `BuildGraphExclusions(players, entities, nodes)` builds in one pass over the players the exclusion of every row (biped, loadout if alive, children of the biped in the object graph, root of the vehicle and its vehicle children), the same set `BuildSelfExclusion` scans the tables for (`DISABLED_ViewExclusionTests.GraphMatchesScan`, run by hand before F4.2 deletes `BuildSelfExclusion`); `FindVehicleRoot` is shared by both. `DerivedEye(biped, definition, crouchFraction)` is the eye A: the world origin plus the up vector times the camera height of the bipd tag, from standing to crouching; `HeadBoneEye(bones, headNode)` is the eye B: the node of the head, invalid if the matrix is not valid. Neither needs a camera, so they hold for bots (`DISABLED_ViewEyeTests.DerivedEyeMatchesLocalCamera`, 2-3 min, measures them against the camera of the local players).

## Tests
`PipelineTests` and `ReplayTests.Recordings` in `Artemis.Tests` (the views reach the flattened tick). `RaycastOracleCompare/Record/Cost/Report` tests the oracle logic with made-up results. `DISABLED_ViewExclusionTests` and `DISABLED_ViewEyeTests` test the helpers of the views per agent over every film (minutes: out of the suite, run with --disabled). `RaycastScene.*` and `RaycastSelfUpdateTests.*` test the shared ray scene (random rays against a linear reference with several rebuild rules, the reuse of parts and the view of the parts after a key change, the objects at rest with the filter and the rebuilds of the fixed tree by threshold and by age, the store between ticks and the update of Self over a made-up map). `RaycastScene.NonFiniteRayMisses` and the `RaycastScene.Reach*` tests (RV.1) fix the guard of the rays that are not numbers and the reach against the trees over random rays, with the fallbacks for another origin, range, exclusion or gather.
