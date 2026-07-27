# libui-ng — Haiku backend

A native [Haiku](https://www.haiku-os.org/) backend for libui-ng, built on the Interface Kit
(BeAPI). It maps libui's portable control model onto `BApplication` / `BWindow` / `BView`, so a
program written once against `ui.h` renders with native Haiku widgets.

This is the missing fourth backend alongside `unix/` (GTK), `windows/` (Win32), and `darwin/`
(Cocoa). The motivation: a common UI library lets you build cross-platform experiences without
touching the BeAPI's C++ directly — which is the hard part for most people.

## Status

**Proof of concept — proven on-device.** Implemented and verified running on Haiku (hrev59810):

- Event loop / lifecycle: `uiInit`, `uiUninit`, `uiMain`, `uiQuit`, `uiQueueMain`
- `uiWindow` (BWindow) — title, content child, margins, close handler
- `uiButton` (BButton) — text, click callback
- `uiLabel` (BStringView) — text
- `uiBox` (BGroupView + BGroupLayout) — vertical/horizontal, padding, stretchy children
- `uiCheckbox` (BCheckBox) — text, checked state, toggle callback
- `uiEntry` (BTextControl) — text, read-only, change callback (Password/Search fall back to a plain
  entry; Placeholder is a no-op — see notes)
- `uiSlider` (BSlider) — value, range, OnChanged (drag) + OnReleased
- `uiProgressBar` (BStatusBar) — value (indeterminate `-1` is stored but not animated)
- `uiSpinbox` / `uiSpinboxDouble` (BSpinner / BDecimalSpinner, from the "shared" private kit)
- `uiSeparator` (BSeparatorView) — horizontal/vertical
- `uiGroup` (BBox) — title, single child, margins
- `uiRadioButtons` (BRadioButton group) — append, selected index, OnSelected
- `uiCombobox` (BOptionPopUp) — append/insert/delete/clear, selected index, OnSelected
- `uiMultilineEntry` (BTextView in a BScrollView) — text, append, read-only, OnChanged; wrapping and
  non-wrapping variants
- `uiTab` (BTabView) — append/insert/delete pages, selected index, per-page margins, OnSelected
- `uiEditableCombobox` (BTextControl + dropdown BButton/BPopUpMenu composite) — append, text, OnChanged
- `uiForm` (BGridView) — labeled rows, stretchy rows, padding, delete
- `uiGrid` (BGridView) — explicit cell placement, spans, per-cell expand/alignment, insert-at, padding
- Menus (`uiMenu` / `uiMenuItem` → BMenuBar / BMenu / BMenuItem) — regular / check / quit /
  preferences / about / separator items, OnClicked, enable/disable, checked state. The
  `uiNewWindow` `hasMenubar` argument is now honored.
- `uiColorButton` (color swatch + modal BColorControl dialog) — color, OnChanged
- `uiFontButton` (button + modal font-family/size dialog with Bold/Italic toggles) — font descriptor,
  OnChanged; plus `uiLoadControlFont` / `uiFreeFontDescriptor` / `uiFreeFontButtonFont`. Stretch and a
  full weight scale aren't exposed (bold = weight 700).
- `uiDateTimePicker` / `uiDatePicker` / `uiTimePicker` (BSpinner fields) — value via `struct tm`,
  OnChanged
- Standard dialogs — `uiMsgBox` / `uiMsgBoxError` (BAlert), `uiOpenFile` / `uiOpenFolder` /
  `uiSaveFile` (BFilePanel, made synchronous; see notes)
- `uiTable` + `uiTableModel` (BColumnListView) — text columns and image columns (BBitmapColumn) fully
  (model-backed rows, row insert/change/delete, selection, row clicked/double-clicked, column widths,
  selection mode). Progress-bar, checkbox, and button columns render natively via custom `BColumn`
  subclasses; checkbox and button columns are **interactive** (`BColumn::MouseDown` + `SetWantsEvents`)
  — a checkbox click toggles the cell through the model's `SetCellValue`, a button click fires it with
  `NULL`. **Editable text columns** support in-place editing: clicking an editable cell overlays a
  `BTextControl` (positioned via `GetFieldRect`/`ScrollView`), and Enter commits via `SetCellValue`.
- `uiImage` (BBitmap) — `uiNewImage`/`uiImageAppend` (RGBA → BGRA), used by image table columns
- `uiArea` + the vector drawing API — a custom-drawn BView whose `Draw` calls the handler, plus
  `uiDrawPath` (BShape; lines, beziers, rectangles, flattened arcs), `uiDrawFill` (solid + linear/
  radial gradients), `uiDrawStroke` (thickness, caps, joins), `uiDrawMatrix` (translate/scale/rotate/
  skew/invert), `uiDrawTransform`, `uiDrawClip`, `uiDrawSave`/`uiDrawRestore`, and mouse/key events.
- `uiDrawText` + text layout — renders an attributed string word-wrapped to the layout width and
  horizontally aligned (`uiDrawNewTextLayout`/`uiDrawText`/`uiDrawTextLayoutExtents`), with **per-run
  attributes**: family, size, weight (bold), italic, color, underline, background, and underline-color
  spans (flattened to a per-byte style array, drawn as same-style segments). Stretch and OpenType-
  feature attributes aren't applied (no `BFont` equivalent). Uses the common attributed-string code
  plus a simple 1-codepoint-per-grapheme breaker (correct for Latin/precomposed text).

Eight demos build and run: `haiku/test/hello.c` (minimal window + label + button),
`haiku/test/widgets.c` (a tour of the controls, slider wired to the progress bar),
`haiku/test/menus.c` (a menubar plus dialog-launching buttons), `haiku/test/tabs.c`
(tabs + combobox + multiline entry), `haiku/test/forms.c` (form rows + editable combobox + grid),
`haiku/test/draw.c` (uiArea: solid + gradient fills, stroked path, rotated rectangle),
`haiku/test/table.c` (a model-backed table with an image column), `haiku/test/choosers.c`
(color/font buttons + date/time pickers), and `haiku/test/drawtext.c` (uiArea text: title, wrapped
paragraph, aligned lines). All display correct native UI. See the build/run steps below.

**Every libui control constructor is implemented**, as is `uiArea` drawing (vector + text), `uiImage`,
and rich table columns (image/progress/checkbox/button, with interactive checkbox + button cells).
What remains is small: a proper Unicode grapheme breaker for complex-script text (the stub is correct
for Latin/precomposed text), and the two unapplied text attributes (stretch, OpenType features) that
have no `BFont` equivalent. The in-place editor commits on Enter (not yet on focus-loss). (Image
brushes are not exposed by libui's `uiDrawBrush` struct, so there's nothing to implement there.)

### Menus and dialogs — notes

The menu model is global, matching the other backends: `uiNewMenu`/`uiMenuAppend*` build an in-memory
description that finalizes when the first window is created, and each window with a menubar
materializes its own native `BMenuBar` from it. A `uiMenuItem` tracks every `BMenuItem` it spawns so
checked/enabled changes stay in sync across windows. Clicks reach the owning window's looper as a
`'uiMN'` message (see `menu.cpp`, `window.cpp`).

libui's file dialogs are synchronous (they return the chosen path), but `BFilePanel` is asynchronous.
`stddialogs.cpp` bridges this with a small dedicated `BLooper` + semaphore: it shows the panel and
blocks the caller until the user picks or cancels. The panel runs on its own looper, so blocking the
calling (window) thread is safe and gives the expected modal behavior. `BFilePanel` lives in
`libtracker`; `BAlert` is in `libbe`. The `*WithParams` variants currently ignore their params
(default folder/name/filters).

### Control event plumbing

All control callbacks share one mechanism (`uiprivNewEventMessage` in `control.cpp`): a control
points its BControl invocation/modification message at a `'uiEV'` `BMessage` carrying a dispatch
function pointer plus the owning `uiControl*`. The window looper's `MessageReceived` just calls that
function — so `window.cpp` never needs to know about individual control types, and adding a control
requires no change there.

## Building

### Today (standalone — validated)

The backend is partial, so the full meson `library('ui', …)` does not link yet (common/ references
draw/area/text/menu symbols this backend doesn't define). The validated path compiles only the
common files the four controls actually need, on a Haiku machine, from the repo root:

```sh
sh haiku/build-haiku.sh      # -> ./libui-haiku-hello
./libui-haiku-hello          # shows the native window
```

`build-haiku.sh` compiles `common/{control,debug,shouldquit}.c` + `haiku/*.cpp` with `g++` and
links against `libbe`.

### Future (meson — once the backend is complete)

`meson.build` already wires Haiku in: `host_machine.system() == 'haiku'` selects `subdir('haiku')`
and links `libbe`. When the remaining controls + draw/area/text/menu/dialogs land in `haiku/`, a
normal `meson setup build && ninja -C build` will produce `libui.so`. Until then that link reports
the expected undefined symbols.

## Design

The backend mirrors the structure of `unix/`:

| libui concept            | Haiku mapping                                              |
|--------------------------|-----------------------------------------------------------|
| event loop               | `BApplication::Run()` (one app object, `be_app`)          |
| `uiControl` base         | `uiHaikuControl` (in `ui_haiku.h`), parallel to `uiUnixControl` |
| a control's OS handle    | a `BView*` (`view` field), accessed via the vtable macros  |
| parenting (`SetContainer`)| `BView::AddChild` / `RemoveChild` (routes into the layout)|
| toplevel                 | `uiWindow` wraps a `BWindow` (not a `BView`) with a hand-written vtable |
| per-control event thread | each `BWindow` runs its own `BLooper`; clicks/close arrive there |

### `ui_haiku.h` — the control base and macros

`uiHaikuControl` embeds `uiControl` plus a `parent` pointer and a `SetContainer` function pointer
(add/remove this control's `BView` to/from a parent `BView`). The `uiHaikuControlAllDefaults(type)`
and `uiHaikuNewControl(type, var)` macros generate and wire the 11-entry `uiControl` vtable for any
control whose struct has a `BView *view` field — exactly as `uiUnixControlAllDefaults` does for GTK.
`Enable`/`Disable`/`Enabled` dynamic-cast the view to `BControl` (only controls can be disabled).

### Windows are special

A `BWindow` is **not** a `BView`, so `uiWindow` does not use the macros — `window.cpp` writes its
own vtable (`Toplevel` returns 1, `Handle` returns the `BWindow*`, show/hide lock the looper). The
window owns a `BGroupView` "content" area (a real `BView`) so `uiWindowSetChild` reuses the same
`AddChild`/`SetContainer` path the boxes use. Close requests run through `QuitRequested()` →
`uiWindowOnClosing`. A `uiHaikuWindowImpl::MessageReceived` override dispatches button invocations
(a `BControl` with no explicit target posts to its window's looper) back to the right `uiButton`
callback via a pointer carried in the invocation `BMessage`.

### Threading / locking

libui calls happen on the main thread, but each window's events dispatch on that window's looper
thread. Mutations to attached views are wrapped in `BWindow::Lock()/Unlock()`; when a control is
not yet attached to a window (e.g. a box being assembled), there is no looper and the lock is
skipped. This is the same discipline any BeAPI program follows.

### Allocator

`alloc.cpp` provides `uiprivAlloc`/`uiprivRealloc`/`uiprivFree` over `calloc`/`realloc`/`free`.
Unlike `unix/alloc.c` it does **not** track allocations for leak reporting — a worthwhile addition
later, but deliberately omitted to keep the proof-of-concept small.

## Isolation boundary

The backend is self-contained under `haiku/` and the root `ui.h`/meson dispatch; nothing in
`common/` or the other backends changed except one pre-existing fix (below). Completing the backend
means adding files under `haiku/` and listing them in `haiku/meson.build` — no churn elsewhere.

## Note on a pre-existing `ui.h` fix

`ui.h`'s `uiDrawBitmap` block (vendored from petabyt/libui-dev) referenced a `uiRect` type whose
definition was never copied in, so `ui.h` failed to compile under a strict C++ frontend. The
missing integer-rect `typedef` was added next to that block. This is unrelated to Haiku but was
required to compile the header at all.
