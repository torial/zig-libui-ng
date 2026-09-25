const std = @import("std");

pub fn build(b: *std.Build) void {
    const optimize = b.standardOptimizeOption(.{});
    const target = b.standardTargetOptions(.{});

    const libui = b.dependency("libui", .{
        .target = target,
        .optimize = optimize,
    });

    const ui_module = b.addModule("ui", .{
        .root_source_file = b.path("src/ui.zig"),
    });
    ui_module.linkLibrary(libui.artifact("ui"));

    // Scintilla + libui-scintilla static library
    const sci_c = b.createModule(.{
        .target = target,
        .optimize = optimize,
    });
    sci_c.link_libc = true;
    sci_c.link_libcpp = true;
    sci_c.addIncludePath(b.path("scintilla/include"));
    sci_c.addIncludePath(b.path("scintilla/src"));
    sci_c.addIncludePath(libui.path("."));
    sci_c.addIncludePath(b.path("libui_scintilla/include"));

    if (target.result.os.tag == .windows) {
        sci_c.addIncludePath(b.path("scintilla/win32"));
        sci_c.addCSourceFiles(.{
            .files = &scintilla_common_sources,
            .flags = &.{"-std=c++17"},
        });
        sci_c.addCSourceFiles(.{
            .files = &scintilla_win_sources,
            .flags = &.{"-std=c++17"},
        });
        sci_c.linkSystemLibrary("imm32", .{});
    } else if (target.result.os.tag == .linux) {
        // ScintillaGTK (2026-09-23): the gtk/ platform layer from the same 5.5.3
        // tarball as src/, the GObject marshaller, and libui_scintilla/unix.cxx.
        sci_c.addIncludePath(b.path("scintilla/gtk"));
        sci_c.addIncludePath(libui.path("unix"));
        sci_c.addCSourceFiles(.{
            .files = &scintilla_common_sources,
            .flags = &.{ "-std=c++17", "-DGTK", "-DNDEBUG" },
        });
        sci_c.addCSourceFiles(.{
            .files = &scintilla_gtk_sources,
            .flags = &.{ "-std=c++17", "-DGTK", "-DNDEBUG" },
        });
        sci_c.addCSourceFile(.{ .file = b.path("scintilla/gtk/scintilla-marshal.c"), .flags = &.{"-DGTK"} });
        sci_c.linkSystemLibrary("gtk+-3.0", .{});
        sci_c.linkSystemLibrary("gmodule-2.0", .{});
    } else {
        sci_c.addCSourceFile(.{ .file = b.path("libui_scintilla/stub.c") });
    }
    sci_c.linkLibrary(libui.artifact("ui"));

    const sci_lib = b.addLibrary(.{
        .name = "sci_native",
        .root_module = sci_c,
        .linkage = .static,
    });

    const sci_module = b.addModule("sci", .{
        .root_source_file = b.path("src/sci.zig"),
        .target = target,
        .optimize = optimize,
    });
    sci_module.addImport("ui", ui_module);
    sci_module.linkLibrary(sci_lib);

    // `ui-extras` (table helpers built on `ui`) and the examples were cut in eceb8117,
    // "Zig 0.16 compat -- minimal build (no examples)". Nothing built them afterwards, so
    // they rotted: six of twelve stopped compiling on 0.16, and `table`/`table-mvc`
    // imported a module the package no longer exported -- unbuildable by anyone. Restored
    // 2026-09-25.
    //
    // The examples hang off their OWN step and are NOT installed by default: consumers
    // (Zebra's `--gui-backend=libui_ng` scaffold depends on this package) run the default
    // step and must not pay for twelve demo executables.
    //   zig build examples          compile every example
    //   zig build run-<name>        build and run one (e.g. zig build run-counter)
    const ui_extras_module = b.addModule("ui-extras", .{
        .root_source_file = b.path("src/extras.zig"),
        .imports = &.{.{ .name = "ui", .module = ui_module }},
    });

    const examples_step = b.step("examples", "Build every example in examples/");
    inline for (examples) |ex| {
        const mod = b.createModule(.{
            .root_source_file = b.path("examples/" ++ ex.name ++ ".zig"),
            .target = target,
            .optimize = optimize,
        });
        mod.addImport("ui", ui_module);
        if (ex.extras) mod.addImport("ui-extras", ui_extras_module);
        const exe = b.addExecutable(.{
            .name = ex.name,
            .root_module = mod,
            .win32_manifest = b.path("examples/example.static.manifest"),
        });
        exe.subsystem = .windows;
        examples_step.dependOn(&exe.step);
        const run = b.addRunArtifact(exe);
        b.step("run-" ++ ex.name, "Build and run examples/" ++ ex.name ++ ".zig").dependOn(&run.step);
    }
}

// Every file in examples/ must be listed here, or it is built by nothing -- which is how
// half of them broke unnoticed. `extras` = the example imports `ui-extras`.
const Example = struct { name: []const u8, extras: bool = false };
const examples = [_]Example{
    .{ .name = "hello" },
    .{ .name = "counter" },
    .{ .name = "timer" },
    .{ .name = "grid" },
    .{ .name = "menu" },
    .{ .name = "draw" },
    .{ .name = "temperature-converter" },
    .{ .name = "flight-booker" },
    .{ .name = "crud" },
    .{ .name = "circle-drawer" },
    .{ .name = "table", .extras = true },
    .{ .name = "table-mvc", .extras = true },
};

const scintilla_common_sources = [_][]const u8{
    "scintilla/src/AutoComplete.cxx",
    "scintilla/src/CallTip.cxx",
    "scintilla/src/CaseConvert.cxx",
    "scintilla/src/CaseFolder.cxx",
    "scintilla/src/CellBuffer.cxx",
    "scintilla/src/ChangeHistory.cxx",
    "scintilla/src/CharacterCategoryMap.cxx",
    "scintilla/src/CharacterType.cxx",
    "scintilla/src/CharClassify.cxx",
    "scintilla/src/ContractionState.cxx",
    "scintilla/src/DBCS.cxx",
    "scintilla/src/Decoration.cxx",
    "scintilla/src/Document.cxx",
    "scintilla/src/EditModel.cxx",
    "scintilla/src/Editor.cxx",
    "scintilla/src/EditView.cxx",
    "scintilla/src/Geometry.cxx",
    "scintilla/src/Indicator.cxx",
    "scintilla/src/KeyMap.cxx",
    "scintilla/src/LineMarker.cxx",
    "scintilla/src/MarginView.cxx",
    "scintilla/src/PerLine.cxx",
    "scintilla/src/PositionCache.cxx",
    "scintilla/src/RESearch.cxx",
    "scintilla/src/RunStyles.cxx",
    "scintilla/src/ScintillaBase.cxx",
    "scintilla/src/Selection.cxx",
    "scintilla/src/Style.cxx",
    "scintilla/src/UndoHistory.cxx",
    "scintilla/src/UniConversion.cxx",
    "scintilla/src/UniqueString.cxx",
    "scintilla/src/ViewStyle.cxx",
    "scintilla/src/XPM.cxx",
};
const scintilla_win_sources = [_][]const u8{
    "scintilla/win32/HanjaDic.cxx",
    "scintilla/win32/PlatWin.cxx",
    "scintilla/win32/ScintillaWin.cxx",
    "libui_scintilla/win.cxx",
};
const scintilla_gtk_sources = [_][]const u8{
    "scintilla/gtk/PlatGTK.cxx",
    "scintilla/gtk/ScintillaGTK.cxx",
    "scintilla/gtk/ScintillaGTKAccessible.cxx",
    "libui_scintilla/unix.cxx",
};
