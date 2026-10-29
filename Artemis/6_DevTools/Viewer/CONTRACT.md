# Artemis.DevTools.Viewer

Group `6_DevTools` - static library inside the game process - the 3D debug viewer: free camera, map, HUD and selection.

## Responsibility
Draws the map and a HUD over the game frame from the published Tick: a free or game camera, the collidables, zones, fixtures and
labels, an overlay with telemetry or the panel of the selected object, and picking by ray. It also defines `EngineAccess`: the
exact members of the engine and bridge Layers the development tools may use, so they import none of those Layers.

## Does not
- Take part in the tick: it reads published values only, after the tick (the observers of the AI thread).
- Build the collidable scene inside the pipeline: its `CollidableSceneService` asks `Perception.Collidable.System` for it on the AI
  thread after the tick, and only while the free camera is active.

## Input and output
- Input: the Tick, the stores of the pure projects, `EngineAccess`, the window input of `Engine.Render`.
- Output: drawing on the game frame; the viewer options in the preferences file.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `DevTools.Viewer.Layer` | The facade: camera, selection, overlay, options and the services that draw. |
| `DevTools.Viewer.Access.Type` | `EngineAccess`: what the tools use of the engine and the bridge, built by `Mod`. |
| `DevTools.Viewer.Camera.Type`, `DevTools.Viewer.Camera.State` | Camera modes, lens, rays and the camera input. |
| `DevTools.Viewer.Options.Type`, `DevTools.Viewer.Options.State` | Viewer options and their preference keys. |
| `DevTools.Viewer.Overlay.State`, `DevTools.Viewer.Selection.State` | Overlay mode and page; selected object and pick requests. |

## Depends on
Every pure project, `Services`, `Bridge.Transport`, the five `Engine.*` projects and `DevTools.Gui`; it imports engine modules only
through what `EngineAccess` exposes.

## State between ticks
Camera, selection and options (saved in the preferences file; the keys never change).

## Threads
Render thread for drawing; window thread for input; AI thread only for the collidable scene, after the tick. Stores are atomic.

## Errors
A failure in the viewer never reaches the tick; the viewer just draws nothing.

## Requirements
Never blocks the AI thread. Collidables are computed only with the free camera on, outside the tick.

## Tests
`CollidableSceneTests` (the pipeline builds no scene; the scene on demand) in `Artemis.Tests`; the rest is manual.
