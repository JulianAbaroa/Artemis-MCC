# Artemis.Engine.Render

Group `5_Engine` - static library inside the game process - hooks the game's render and window input for the dev tools.

## Responsibility
Finds the swap chain methods without touching the game's one (`SwapChainLocator`), hooks `Present` and `ResizeBuffers` to give the
development tools a frame to draw on (device, context, render target), and hooks the window procedure and `GetRawInputData` to
dispatch window messages and raw mouse movement to registered handlers (`InputService`).

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
| `Engine.Render.Type`, `Engine.Render.State`, `Engine.Render.System`, `Engine.Render.Hook` | Frame types and handlers, `RenderStore` (D3D11 objects, render flags), `RenderService`, the present and resize detours. |
| `Engine.Render.Window.Type`, `Engine.Render.Window.System`, `Engine.Render.Window.Hook` | Window messages, raw mouse, `InputService` and the window detours. |

## Depends on
`Common`, `Services`, `Engine.Base`.

## State between ticks
The D3D11 objects bound to the game's swap chain and the render flags.

## Threads
The render thread of MCC for the frame hooks; the window thread for messages. Flags and sizes are atomic; D3D11 objects are touched
only from the render thread. `WaitForIdle` waits for the hooks in flight before unhooking.

## Errors
If it cannot hook, the tools do not appear and everything else runs.

## Requirements
Can be disabled entirely without changing the tick.

## Tests
`EngineLayerTests` builds it without the game; the rest is manual (the user sees the UI).
