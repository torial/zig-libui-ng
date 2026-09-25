# libui-ng bindings in zig

This repository is a work in progress. Libui-ng is a c library for creating
cross-platform applications using the native widget toolkits for each platform.
These bindings are a manual cleanup of the cimport of `ui.h`. Each control
type has been made an opaque with the extern functions embedded within in them.
Additionally, functions using boolean values have been converted to use `bool`.
Some helper functions have been made for writing event handlers.

Builds with **Zig 0.16**.

## This fork (torial)

One repository since 2026-09-17: the C library lives here as `libui/` (a `git subtree`
of torial/libui-ng, history kept; `git subtree pull --prefix=libui <remote> main` still
brings upstream commits), wired as a path dependency in `build.zig.zon`, so a C change
and its Zig binding land in one commit. Also here: Scintilla (`scintilla/`), the
libui-scintilla shim (`libui_scintilla/`, the `sci` module), and additions upstream
libui-ng does not have -- `uiTabName` / `uiTabSetName`, `uiScintillaOnNotify` /
`uiScintillaOnKey` (Scintilla on Windows and GTK), `uiWindowOnKey`, `uiTree` (a native
tree), `uiControlSetTooltip`, `uiClipboard*`, the area's full mouse and key events, and
`uiToolbar` (2026-09-23: GtkToolbar / ToolbarWindow32 / NSToolbar). Haiku was dropped on
2026-09-23. Platform tiers: Windows and GTK are built and witnessed here; macOS is the
second tier -- written against the Cocoa API without a build, tested when a Mac is to
hand. Consumer: the Zebra language's `--gui-backend=libui_ng`
(pinned by commit in its compiler; `tools/bump_libui_pin.sh` there after a push).
Builds with zig 0.16.

## Using it

Add the dependency (from a directory with a `build.zig.zon`):

```
zig fetch --save git+https://github.com/torial/zig-libui-ng#<commit>
```

then import its modules in `build.zig`:

```zig
const lui = b.dependency("bindings_libui_ng", .{ .target = target, .optimize = optimize });
exe.root_module.addImport("ui", lui.module("ui"));   // the libui-ng bindings
// optional: lui.module("sci")        Scintilla editor control
//           lui.module("ui-extras")  table helpers (see examples/table*.zig)
```

The C library is compiled from source as part of your build, so there is nothing to
install on Windows or macOS. **Linux needs GTK 3 development headers** (Debian/Ubuntu:
`sudo apt install libgtk-3-dev`; Fedora: `sudo dnf install gtk3-devel`).

Zebra users do not need any of this: `zebra --gui-backend=libui_ng app.zbr` scaffolds
the dependency for you.

## Examples

Twelve small programs live in `examples/`, most of them the 7GUIs tasks (counter,
temperature converter, flight booker, timer, CRUD, circle drawer) plus tables, drawing,
menus and a grid.

```
zig build examples          # compile all of them
zig build run-counter       # build and run one: run-<file name without .zig>
```

They are a named step, not part of the default build, so a project that depends on this
package does not build them. Every file in `examples/` must be listed in `build.zig`'s
`examples` table -- anything unlisted is built by nothing, which is how half of them
stopped compiling during the Zig 0.16 port without anyone noticing.

## Example

This is `examples/hello.zig`, verbatim -- `zig build examples` compiles it, so it cannot
drift from the API the way the previous README example did:

```zig
const std = @import("std");
const ui = @import("ui");

pub fn on_closing(_: *ui.Window, _: ?*void) !ui.Window.ClosingAction {
    ui.Quit();
    return .should_close;
}

pub fn main() !void {
    var init_data = ui.InitData{
        .options = .{ .Size = 0 },
    };
    ui.Init(&init_data) catch {
        std.debug.print("Error initializing LibUI: {s}\n", .{init_data.get_error()});
        init_data.free_error();
        return;
    };
    defer ui.Uninit();

    const main_window = try ui.Window.New("Hello, World!", 320, 240, .hide_menubar);

    main_window.SetChild((try ui.Label.New("Hello, World!")).as_control());

    main_window.as_control().Show();
    main_window.OnClosing(void, ui.Error, on_closing, null);

    ui.Main();
}
```

## Planned Features
- [x] Comptime function for defining a `Table` based on a struct
- [x] Nicer bindings for event callbacks
- [ ] More examples
- [ ] Project Template
