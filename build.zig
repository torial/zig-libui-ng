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
}
