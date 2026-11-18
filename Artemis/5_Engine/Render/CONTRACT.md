# Artemis.Engine.Render

Group `5_Engine` - static library inside the game process - hooks the game's render and window input for the dev tools.

## Responsibility
Finds the swap chain methods without touching the game's one (`SwapChainLocator`), hooks `Present` and `ResizeBuffers` to give the
development tools a frame to draw on (device, context, render target), hooks the three HUD passes of the game (`ChudDetour`, VW.6.x), and hooks the window procedure and `GetRawInputData` to
dispatch window messages and raw mouse movement to registered handlers (`InputService`).

HUD passes of the game (VW.6.x): `ChudDetour` hooks `chud draw` (0x2c29f8), `chud draw late` (0x2c2628) and `chud draw 3D` (0x2c279c), the game's HUD passes inside the render of a view, all `void(uint32 user)`. Each is found with `AOBService::FindFunctionAtRva` by its RVA and the exact start of its prologue (`k_ChudDrawRva` / `k_ChudDrawPrologue` and the like in `Engine.Base.Memory.Type`, no slack: a moved function is not hooked). A prologue that does not match logs an ERROR and that one hook is not installed, the others and the Viewer keep working as before. Installed by `Mod` after Present and ResizeBuffers, removed first at shutdown (disable, `WaitForIdle`, remove).

First person (D331): `FirstPersonDetour` hooks the first person model pass (`void(void* camera, uint32 user, uint8 pass)`, 0x286ebc, the arms and weapon) and the game's first person unit test (`bool(uint32 user, uint32 objectHandle)`, 0x266104, answer in AL). Each is found with `FindFunctionAtRva` by RVA and exact prologue (`k_FirstPersonModelRva` and the like in `Engine.Base.Memory.Type`, window 0). A prologue that does not match logs an ERROR, that hook is not installed and the other still is. Installed by `Mod` in `OnEngineInitialized` after `ChudDetour`, removed in `OnUnhook` and at shutdown (disable, `WaitForIdle`, remove, serialized by a mutex). The model detour returns without calling the original when the user is 0 and `RenderStore::IsFirstPersonHidden` (option `Hide first person`) is set or Drive is active. The unit test detour calls the original and, for user 0 with Drive active and an original answer of true, answers false so the game draws the player's own biped and the third person effects. Users other than 0 are never changed. Drive is active when `CameraOverrideStore::Read(k_OverrideMaxAge)` finds a pose, the same rule `ObserverCameraDetour` applies. The unit test is also called by effects and possibly from a thread other than the render: the detours do not assume a thread, read only atomics and the store without locking, do not allocate and do not log. The decisions are the pure static `IsFirstPersonModelSkipped` and `IsThirdPersonShown`. `RenderStore` counts the turned answers (`CountThirdPerson`), `EndHudFrame` at Present closes the count (`WasThirdPersonLastFrame`) and `IsFirstPersonModelHooked` / `IsFirstPersonUnitHooked` say what is installed. Render only: the pose of the director is never written. First person measurement (VW.6.y.2, measures only, the drawing does not change): the model detour, for user 0 and before the original, takes `_ReturnAddress()`, turns it into an RVA of `haloreach.dll` (the base is stored at `Install`, the address of a hooked function minus its RVA) and classifies it with the pure `FirstPersonDetour::ClassifyCaller` by function range (`k_FirstPersonCallerRanges` in `Engine.Render.Type`: 0x26DA08 / 498, 0x26E2A0 / 605, 0x26EA78 / 788, 0x277D00 / 543, any other is `Other` with the raw RVA). Each call is counted per caller in `RenderStore` (`CountFirstPersonCall`, closed by `EndHudFrame`). Only while a probe is armed (`RequestFirstPersonProbe` sets it, `StepFirstPersonProbe` at Present arms it at the next frame so that frame is measured whole) `RenderService::MeasureFirstPerson` reads, on the thread of Present only (another thread: the sample says "other thread" and no D3D is touched), `OMGetRenderTargets(8)` (non null count, render target 0: size, texture and view format, back buffer), the depth stencil view (size, texture and view format, view flags, resource address kept as an integer to compare), viewport 0 (with MinDepth and MaxDepth), `OMGetDepthStencilState` and `RSGetState` (a null state is the default state), releases everything and copies one `FirstPersonSample` per caller under a short lock (`SetFirstPersonSample`, 5 slots). No allocation and no log in the detour. At the Present that closes the first armed frame with first person calls, `RenderService` writes one `[FirstPersonProbe] INFO:` line per caller with calls and the HUD draw line (which also says whether its depth stencil resource is the one of 0x26EA78, compared as integers, never dereferenced) and the probe ends. The two detours read Drive each on its own, maybe on two threads, so on the frame Drive starts or ends (or when a read meets the writer, or a pose ages past 100 ms) the arms and the body may disagree for one frame: accepted. `Uninstall` waits 20 ms between disabling and draining, so a thread past the jump but not yet inside its `HookScope` is counted before the trampolines are removed.

Draw before the HUD (VW.6.x B): `RenderService::OnBeforeHud(BeforeHudHandler)` registers a handler that `chud draw` calls BEFORE the original, so what it draws ends up under the game's HUD. The handler gets a `FrameContext` made from the render target bound in slot 0 (device, immediate context, that view, its size, the store's depth stencil) and returns whether it drew. `RenderService::OnHudDraw` (the only thing the detour calls) runs it only for user 0 when: the game HUD is not hidden, it did not already run in this Present frame, the measure says the same thread as Present, the store is ready, not resizing and not shutting down, and the bound target is the store's current back buffer, of the store's device, a texture of the size of the back buffer, and viewport 0 is the whole target, so split screen never draws before the HUD (`FindBeforeHudBlocker`, pure). Nothing escapes the HUD draw: `OnHudDraw` catches everything and counts it (`RenderStore::CountHudFailure`). The whole call is wrapped in a `GpuStateGuard` (render targets 0 to 7 with the depth stencil, viewports and scissors with the counts that were bound, predication (off meanwhile), blend, depth stencil and rasterizer states, shaders, input layout, topology, vertex and index buffers, vertex constant buffer 0: all that the Viewer passes set, they bind no shader resources or samplers). A handler that returns true marks the frame: `RenderStore::MarkBeforeHud` records the index of the Present frame (`NextPresentFrame` ends each Present, after the frame handlers), `IsBeforeHudMarked` tells the Present handlers that the work was done, so each one draws at Present only when it was not: exactly once per frame, never twice. A handler that throws is logged and the frame counts as handled too (status `Failed`), so Present does not draw it a second time. `BeforeHudStatus` keeps why the last completed frame did or did not draw before the HUD (no HUD draw in the frame, other thread, not ready, target of another size, game HUD hidden, handler declined).

With the atomic flag `RenderStore::IsGameHudHidden` (set by the Viewer option `Hide game HUD`) the three detours return without calling the original. Otherwise they call it untouched. `chud draw` also measures, for user 0 only: the number of calls between two Present (`CountHudCall` / `EndHudFrame`, `HudHookInfo::CallsPerFrame`) and, only when it runs on the thread of Present (`RenderStore::GetPresentThread`, noted at each Present), the render target bound in slot 0 (size, format, whether it is the back buffer) and viewport 0, read with `OMGetRenderTargets` and `RSGetViewports` on the immediate context. On another thread nothing of D3D is touched and the sample only says "other thread". It also reads the depth stencil view bound in the same call (`OMGetRenderTargets` with a view out, `HudHookInfo::Depth`: size, texture and view format, flags, resource address as an integer). The sample is copied into the store under a short lock (`SetHudSample`, `GetHudHook`). No allocation, no D3D call other than those reads.

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
