# Artemis.Engine.Render

Group `5_Engine` - static library inside the game process - hooks the game's render and window input for the dev tools.

## Responsibility
Finds the swap chain methods without touching the game's one (`SwapChainLocator`), hooks `Present` and `ResizeBuffers` to give the
development tools a frame to draw on (device, context, render target), hooks the three HUD passes of the game (`ChudDetour`, VW.6.x), and hooks the window procedure and `GetRawInputData` to
dispatch window messages and raw mouse movement to registered handlers (`InputService`).

HUD passes of the game (VW.6.x): `ChudDetour` hooks `chud draw` (0x2c29f8), `chud draw late` (0x2c2628) and `chud draw 3D` (0x2c279c), the game's HUD passes inside the render of a view, all `void(uint32 user)`. Each is found with `AOBService::FindFunctionAtRva` by its RVA and the exact start of its prologue (`k_ChudDrawRva` / `k_ChudDrawPrologue` and the like in `Engine.Base.Memory.Type`, no slack: a moved function is not hooked). A prologue that does not match logs an ERROR and that one hook is not installed, the others and the Viewer keep working as before. Installed by `Mod` after Present and ResizeBuffers, removed first at shutdown (disable, `WaitForIdle`, remove).

With the atomic flag `RenderStore::IsGameHudHidden` (set by the Viewer option `Hide game HUD`) the three detours return without calling the original. Otherwise they call it untouched. `chud draw` also measures, for user 0 only: the number of calls between two Present (`CountHudCall` / `EndHudFrame`, `HudHookInfo::CallsPerFrame`) and, only when it runs on the thread of Present (`RenderStore::GetPresentThread`, noted at each Present), the render target bound in slot 0 (size, format, whether it is the back buffer) and viewport 0, read with `OMGetRenderTargets` and `RSGetViewports` on the immediate context. On another thread nothing of D3D is touched and the sample only says "other thread". The sample is copied into the store under a short lock (`SetHudSample`, `GetHudHook`). No allocation, no D3D call other than those reads.

## Does not
- Draw anything itself: `DevTools.Gui` and `DevTools.Viewer` draw.
- Affect the tick: it can be left uninstalled and the pipeline runs the same.

## Input and output
- Input: the game's swap chain and window messages.
- Output: frame callbacks (`FrameContext`) and input callbacks for the handlers.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Engine.Render.Layer` | The facade: store, services, locator and detours. Owner only. |
| `Engine.Render.Type`, `Engine.Render.State`, `Engine.Render.System`, `Engine.Render.Hook` | Frame types and handlers, `RenderStore` (D3D11 objects, render flags, the `HideGameHud` flag and the `HudHookInfo` measure), the HUD detours (`ChudDetour`), `RenderService`, the present and resize detours, and the `GpuPipeline` of the debug passes with its `SurfaceMode` (opaque, overlay, translucent, depth only with the color write mask at 0 and a depth bias away from the camera, wireframe with no bias and no depth write; D325). The depth is reversed (near 1, far 0, test greater or equal; the caller clears it to 0). |
| `Engine.Render.Window.Type`, `Engine.Render.Window.System`, `Engine.Render.Window.Hook` | Window messages, raw mouse, `InputService` and the window detours. |

## Depends on
`Common`, `Services`, `Engine.Base`.

## State between ticks
The D3D11 objects bound to the game's swap chain and the render flags.

## Threads
The render thread of MCC for the frame hooks (the HUD passes run on the thread that renders the view, which the measure compares with the thread of Present); the window thread for messages. Flags and sizes are atomic; D3D11 objects are touched
only from the render thread. `WaitForIdle` waits for the hooks in flight before unhooking.

## Errors
If it cannot hook, the tools do not appear and everything else runs.

## Requirements
Can be disabled entirely without changing the tick.

## Tests
`EngineLayerTests` builds it without the game; the rest is manual (the user sees the UI).
