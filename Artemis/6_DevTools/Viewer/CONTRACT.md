# Artemis.DevTools.Viewer

Group `6_DevTools` - static library inside the game process - the 3D debug viewer: free camera, map, HUD and selection.

## Responsibility
In sparse mode (Q101) it sees only the decision ticks and does not raise the every-tick demand. The HUD shows the light ticks per second and their mean time next to the sweep time.
Draws the map and a HUD over the game frame from the published Tick: a free or game camera (the game one is the camera the game rendered the frame with, see Overlay below), the collidables, zones, fixtures and
labels, an overlay with telemetry, the panel of the selected object or the panel of the View of the watched player, and picking by ray.
It also draws that View (`ViewPass`, option `View of the watched player`, on by default): the cone of sight (`k_SightHalfAngle` and
`k_SightRange` of `Views.Raycast.Type`, from the eye along the aim, none in Full mode), a line to each player it sees, markers for its
allies, the two flags (a line to the stand when away, the carrier ringed), the radar points and the sounds, all in the color of the
relation (enemy red, ally blue, none yellow). The only thing it draws from the Tick is, in grey, the players the View does not contain, to
compare by eye with what the game shows. It also defines `EngineAccess`: the
exact members of the engine and bridge Layers the development tools may use, so they import none of those Layers.

Overlay on the game frame (VW.5a, D323, D325). With the game camera the view-projection comes from `s_render_projection` of the game (`EngineAccess::m_GameCameraService`, read at Present on the render thread, memory read only): the view from its axes and position, x and y of the projection as the game has them (m00, m11, m20, m21), the depth from the own `Lens.Near` and `Far`. The depth of the whole Viewer is reversed (VW.6a: near maps to 1 and far to 0, `MapService` clears to 0, `GpuPipeline` tests greater or equal), which also holds for the free camera. `ToScreen` and `ScreenToRay` use only x, y and w. The wireframe sits on its own mesh because the depth-only pass is pushed away from the camera, not because the lines are pulled forward. `SceneService::Update` computes once per frame the hidden set (`GetHiddenHandles`): the biped of the player whose eyes the camera is in and the objects hanging from it in the object graph. In Game mode that is the primary local player only while the game camera is within `k_FirstPersonDistance` (0.1 wu, on foot it measures 0.004, in third person it is meters) of the View eye (`GetEyeDistance`, the same measure as the "Distance to View eye" telemetry), so in a vehicle or a mounted turret the body is drawn; with no valid game camera the camera sits on the View and it is hidden as before. With the Free FirstPerson follow it is the followed player. `MapService` gives the set to `DynamicPass::Upload` (both dynamic passes) and `AimPass::Upload` (spheres and center markers), and the health bars (`HealthLabels`, through `LabelContext::Hidden`) skip it too. The set is part of the upload gate of both passes (`HandleSetKeyOf`). Orbit and the free camera that follows nothing draw everything. Option `Aim spheres` (key `Viewer_AimSpheres`, on by default): off, `AimPass` draws nothing. `GetEye`, `GetForward`, `ToScreen` and `ScreenToRay` follow it, so labels and picking line up. A camera that is not valid (not finite, axes not orthonormal, m00 or m11 not positive) or no read makes the camera sit on the View of the local player as before; the free camera is unchanged.

Option `Overlay on the game frame` (key `Viewer_Overlay`, off by default) with the game camera: `MapService` does not clear the color, so the frame of the game stays under what the Viewer draws (its own depth is still cleared), the map collision is drawn in depth only (it hides what comes after it, no color) and, with `Overlay: collision wireframe` (key `Viewer_OverlayWireframe`, on by default), also as visible wireframe to check that it lines up with the walls. The other passes are the same. With the overlay off, or with the free camera, it draws as before.

Overlay under the game HUD (VW.6.x B): with the overlay active (game camera and `Viewer_Overlay`), `MapService::RenderBeforeHud` runs the whole 3D frame (scene update, uploads and every pass) from `RenderService::OnBeforeHud`, so the passes go under the game's HUD. It declines (returns false, before any update) when the overlay is not active. ImGui, labels and panels stay in `OnFrame` at Present, which calls `MapService::Render` only when `RenderStore::IsBeforeHudMarked` is false: the 3D passes and the scene update run once per frame, at one point or the other (HUD off by the game, menus, pause, `Hide game HUD`, a target of another size, a handler that declined). The labels and panels of Present use the camera, hidden set and projection computed by that one update, and the scene that update acquired (`SceneService::GetFrameScene`, one acquire per frame, D294), never a newer tick. Before the HUD the color is never cleared. If the mode changed since the decision and the overlay is no longer active after the update, nothing is drawn but the frame counts as handled. A throwing pass leaves the GPU timing closed (`GpuTimingScope`) and the frame handled. Without the overlay everything stays at Present. The telemetry line `Overlay drawn` says `before HUD` or `at Present` with the reason (`RenderStore::GetBeforeHudStatus`).

Option `Hide game HUD` (key `Viewer_HideGameHud`, off by default, group Camera; VW.6.x): `Layer` copies it to `RenderStore::SetGameHudHidden` at the start of every Present frame, and the HUD detours of `Engine.Render` then skip the game's HUD passes (clean captures). Without the hooks installed it does nothing.

Option `Drive game camera` (key `Viewer_DriveGameCamera`, off by default, group Camera; VW.5b, D323): with the Free mode and nothing followed or orbited, every frame `CameraService` asks `GameCameraService::SetOverride` for the eye, forward and up of the free camera, and the observer hook of `Engine.Base` makes the game build its camera there (position, location for culling and vertical field of view the normal way, one frame later). In any other case, when the option is turned off or when the Viewer is deactivated, it calls `ClearOverride` and the camera goes back to the player. While it drives, the projection of the Viewer comes from the game camera read (x and y as in Game mode, depth its own) and the pose is the free one, so `SceneService::IsGameCamera` is true and the overlay lines up with the frame. The telemetry shows `Override: on/off`, `Override faults` (only when the hook caught any fault touching the engine memory) and `Override error`: the distance between the eye asked last frame and the position of the game camera read (about 0 with a frame of delay).

The overlay window (telemetry mode) shows the game camera: valid or not, position, horizontal and vertical field of view, aspect (m11 / m00) against the back buffer, the off-center terms and the distance to the eye of the View of the local player, and the line `HUD hook` (thread same or other than Present, calls per frame of the HUD draw, the render target it draws to with size and format, whether that is the back buffer, and viewport 0) from `RenderStore::GetHudHook`.

## Does not
- Take part in the tick: it reads published values only, after the tick (the observers of the AI thread).
- Build the collidable scene inside the pipeline: its `CollidableSceneService` asks `Perception.Collidable.System` for it on the AI
  thread after the tick, and only while the viewer camera is active (free or game).

## Input and output
- Input: the Tick (taken with the collidable scene, published together after the tick, D294), the stores of the pure projects, `EngineAccess`, the window input of `Engine.Render`.
- Output: drawing on the game frame; the viewer options in the preferences file.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `DevTools.Viewer.Layer` | The facade: camera, selection, overlay, options and the services that draw. While the camera follows or orbits the biped of a player, `SceneService` writes that player in the `ObservedPlayerStore` of `Services` (none otherwise), so the AI thread gives it a View (F4.2). The camera, the labels, the aim hit label and the rays and the View it draws come from the View of the watched player (`SceneService::GetObservedView`: the observed one, else the primary local player), and the free camera seats on the View of the local player. |
| `DevTools.Viewer.Access.Type` | `EngineAccess`: what the tools use of the engine and the bridge (it also carries the `OracleStore` of the raycast oracle and the `MapVariantDecodeStore` the bench asks decodes through), built by `Mod`. |
| `DevTools.Viewer.Camera.Type`, `DevTools.Viewer.Camera.State` | Camera modes, lens, rays and the camera input. |
| `DevTools.Viewer.Options.Type`, `DevTools.Viewer.Options.State` | Viewer options and their preference keys (`Viewer_PlayerView` is the one of the View pass, `Viewer_AimSpheres` the one of the aim spheres, `Viewer_Overlay` and `Viewer_OverlayWireframe` the ones of the overlay, `Viewer_DriveGameCamera` the one that moves the camera of the game). |
| `DevTools.Viewer.Overlay.State`, `DevTools.Viewer.Selection.State` | Overlay mode (telemetry, the panels of the selected object and `View`) and page; selected object and pick requests. |

## Depends on
Every pure project, `Services`, `Bridge.Transport`, the five `Engine.*` projects and `DevTools.Gui`; it imports engine modules only
through what `EngineAccess` exposes.

## State between ticks
Camera, selection and options (saved in the preferences file; the keys never change).

## Threads
Render thread for drawing, for reading the game camera and for writing the camera override (the one writer of the `CameraOverrideStore`); window thread for input; AI thread only for the collidable scene, after the tick. Stores are atomic.

## Errors
A failure in the viewer never reaches the tick; the viewer just draws nothing.

## Requirements
Never blocks the AI thread. Collidables are computed only while the viewer camera is active, outside the tick.

## Tests
`CollidableSceneTests` (the pipeline builds no scene; the scene on demand) in `Artemis.Tests`; the rest is manual.
