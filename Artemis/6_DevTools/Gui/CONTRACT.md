# Artemis.DevTools.Gui

Group `6_DevTools` - static library inside the game process - the ImGui backend and the widget kit of the development tools.

## Responsibility
Runs Dear ImGui (compiled here, with its DX11 and Win32 backends) on the game's render: starts, resizes, draws and shuts down with
the frame events of `Engine.Render`, and feeds it the window messages. Also the shared widgets, colors, number formats and style
the other tools draw with.

## Does not
- Know the tick pipeline or any pure project beyond `Common`: no Blam, Perception or bridge types.
- Draw any tool: `DevTools.Viewer` and `DevTools.Workbench` do.

## Input and output
- Input: frame and window callbacks of `Engine.Render`.
- Output: an ImGui frame ready to draw into, and the widget functions.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `DevTools.Gui.Layer` | The facade: `BackendGuiService` and `WndProcGuiService`. |
| `DevTools.Gui.Backend.System` | Whether the UI can draw now; the backend lifecycle. |
| `DevTools.Gui.Widget.System`, `DevTools.Gui.Color.System`, `DevTools.Gui.Format.System`, `DevTools.Gui.Style.Type` | Widgets, colors, formats and style constants. |

## Depends on
`Common`, `Services`, `Engine.Base`, `Engine.Render`.

## State between ticks
The ImGui context and its DX11 resources.

## Threads
Render thread for drawing, window thread for messages, as `Engine.Render` calls them.

## Errors
If the backend cannot start, the tools are not drawn; the game and the pipeline are unaffected.

## Requirements
Imports nothing of the tick pipeline (checked by `Tools/Tests/test_components.py`).

## Tests
Manual (the user sees the UI); boundaries in `test_components.py`.
