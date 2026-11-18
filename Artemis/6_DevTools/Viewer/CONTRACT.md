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

Overlay on the game frame (VW.5a, D323, D325). With the game camera the view-projection comes from `s_render_projection` of the game (`EngineAccess::m_GameCameraService`, read at Present on the render thread, memory read only): the view from its axes and position, x and y of the projection as the game has them (m00, m11, m20, m21), the depth from the own `Lens.Near` and `Far`. The depth of the whole Viewer is reversed (VW.6a: near maps to 1 and far to 0, `MapService` clears to 0, `GpuPipeline` tests greater or equal), which also holds for the free camera. `ToScreen` and `ScreenToRay` use only x, y and w. The wireframe sits on its own mesh because the depth-only pass is pushed away from the camera, not because the lines are pulled forward. `SceneService::HiddenHandlesOf` gives the biped of the player whose eyes the camera is in (Game mode: the primary local player; Free with the FirstPerson follow: the followed one) and the objects hanging from it in the object graph, and `DynamicPass::Upload` leaves them out of both dynamic passes; Orbit and the free camera that follows nothing draw everything. `GetEye`, `GetForward`, `ToScreen` and `ScreenToRay` follow it, so labels and picking line up. A camera that is not valid (not finite, axes not orthonormal, m00 or m11 not positive) or no read makes the camera sit on the View of the local player as before; the free camera is unchanged.

Option `Overlay on the game frame` (key `Viewer_Overlay`, off by default) with the game camera: `MapService` does not clear the color, so the frame of the game stays under what the Viewer draws (its own depth is still cleared), the map collision is drawn in depth only (it hides what comes after it, no color) and, with `Overlay: collision wireframe` (key `Viewer_OverlayWireframe`, on by default), also as visible wireframe to check that it lines up with the walls. The other passes are the same. With the overlay off, or with the free camera, it draws as before.

The overlay window (telemetry mode) shows the game camera: valid or not, position, horizontal and vertical field of view, aspect (m11 / m00) against the back buffer, the off-center terms and the distance to the eye of the View of the local player.

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
| `DevTools.Viewer.Options.Type`, `DevTools.Viewer.Options.State` | Viewer options and their preference keys (`Viewer_PlayerView` is the one of the View pass, `Viewer_Overlay` and `Viewer_OverlayWireframe` the ones of the overlay). |
| `DevTools.Viewer.Overlay.State`, `DevTools.Viewer.Selection.State` | Overlay mode (telemetry, the panels of the selected object and `View`) and page; selected object and pick requests. |

## Depends on
Every pure project, `Services`, `Bridge.Transport`, the five `Engine.*` projects and `DevTools.Gui`; it imports engine modules only
through what `EngineAccess` exposes.

## State between ticks
Camera, selection and options (saved in the preferences file; the keys never change).

## Threads
Render thread for drawing and for reading the game camera; window thread for input; AI thread only for the collidable scene, after the tick. Stores are atomic.

## Errors
A failure in the viewer never reaches the tick; the viewer just draws nothing.

## Requirements
Never blocks the AI thread. Collidables are computed only while the viewer camera is active, outside the tick.

## Tests
`CollidableSceneTests` (the pipeline builds no scene; the scene on demand) in `Artemis.Tests`; the rest is manual.
