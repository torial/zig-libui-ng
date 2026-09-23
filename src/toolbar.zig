/// uiToolbar (torial fork, 2026-09-23): a native toolbar under the menubar. Window-owned
/// like the menubar: build it, append items, hand it to `Window.SetToolbar`. Items are
/// addressed by the index `AppendItem` returns; `OnClicked` receives that index. GtkToolbar /
/// ToolbarWindow32 / NSToolbar.
pub const Toolbar = opaque {
    const Self = @This();

    pub extern fn uiNewToolbar() ?*Toolbar;
    pub extern fn uiToolbarAppendItem(t: *Toolbar, label: [*:0]const u8, icon: ?*Image, tooltip: ?[*:0]const u8) c_int;
    pub extern fn uiToolbarAppendSeparator(t: *Toolbar) c_int;
    pub extern fn uiToolbarNumItems(t: *Toolbar) c_int;
    pub extern fn uiToolbarClear(t: *Toolbar) void;
    pub extern fn uiToolbarSetItemEnabled(t: *Toolbar, index: c_int, enabled: c_int) void;
    pub extern fn uiToolbarOnClicked(t: *Toolbar, f: ?*const fn (?*Toolbar, c_int, ?*anyopaque) callconv(.c) void, data: ?*anyopaque) void;

    pub fn New() !*Toolbar {
        return uiNewToolbar() orelse error.InitToolbar;
    }
    /// Returns the item's index; `icon` may be null (label only), `tooltip` may be null.
    pub const AppendItem = uiToolbarAppendItem;
    pub const AppendSeparator = uiToolbarAppendSeparator;
    pub const NumItems = uiToolbarNumItems;
    pub const Clear = uiToolbarClear;
    pub fn SetItemEnabled(t: *Toolbar, index: c_int, enabled: bool) void {
        uiToolbarSetItemEnabled(t, index, @intFromBool(enabled));
    }

    pub fn OnClicked(self: *Self, comptime T: type, comptime E: type, comptime f: *const fn (*Self, c_int, ?*T) E!void, userdata: ?*T) void {
        const callback = struct {
            fn callback(self_opt: ?*Self, index: c_int, t_opt: ?*anyopaque) callconv(.c) void {
                const err_ctx = ErrorContext{ .ToolbarOnClicked = self_opt };
                const s = self_opt orelse return error_handler(err_ctx, t_opt, error.LibUIPassedNullPointer);
                f(s, index, @as(?*T, @ptrCast(@alignCast(t_opt)))) catch |err| error_handler(err_ctx, t_opt, err);
            }
        }.callback;
        uiToolbarOnClicked(self, callback, userdata);
    }
};

pub const Image = ui.Image;
pub const error_handler = ui.error_handler;
pub const ErrorContext = ui.ErrorContext;

pub const ui = @import("ui.zig");
