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
            .files = &scintilla_win_sources,
            .flags = &.{"-std=c++17"},
        });
        sci_c.linkSystemLibrary("imm32", .{});
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
}

const scintilla_win_sources = [_][]const u8{
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
    "scintilla/win32/HanjaDic.cxx",
    "scintilla/win32/PlatWin.cxx",
    "scintilla/win32/ScintillaWin.cxx",
    "libui_scintilla/win.cxx",
};
