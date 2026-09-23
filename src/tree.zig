/// uiTree (torial fork, 2026-09-22): a native single-column tree -- one text column,
/// expand/collapse, single selection, activation. SysTreeView32 / GtkTreeView over a
/// hierarchical model / NSOutlineView / BOutlineListView. Pull-based like Table: the
/// app owns the data; a node is an opaque pointer the app hands out, null is the root.
pub const Tree = opaque {
    const Self = @This();
    pub fn as_control(self: *Self) *Control {
        return @ptrCast(@alignCast(self));
    }

    pub const Model = opaque {
        pub const Handler = extern struct {
            NumChildren: *const fn (*Handler, *Model, ?*anyopaque) callconv(.c) c_int,
            Child: *const fn (*Handler, *Model, ?*anyopaque, c_int) callconv(.c) ?*anyopaque,
            Text: *const fn (*Handler, *Model, ?*anyopaque) callconv(.c) [*:0]const u8,   // app-owned; valid until the next handler call
            HasChildren: *const fn (*Handler, *Model, ?*anyopaque) callconv(.c) c_int,
            Icon: ?*const fn (*Handler, *Model, ?*anyopaque) callconv(.c) ?*ui.Image = null,   // 2026-09-23: null fn or null result = no icon; the image must outlive the node
        };
        pub extern fn uiNewTreeModel(mh: *Handler) ?*Model;
        pub extern fn uiFreeTreeModel(m: *Model) void;
        pub extern fn uiTreeModelNodeInserted(m: *Model, parent: ?*anyopaque, index: c_int) void;
        pub extern fn uiTreeModelNodeDeleted(m: *Model, parent: ?*anyopaque, index: c_int) void;
        pub extern fn uiTreeModelNodeChanged(m: *Model, node: ?*anyopaque) void;

        pub fn New(mh: *Handler) !*Model {
            return uiNewTreeModel(mh) orelse error.InitTreeModel;
        }
        pub const Free = uiFreeTreeModel;
        pub const NodeInserted = uiTreeModelNodeInserted;
        pub const NodeDeleted = uiTreeModelNodeDeleted;
        pub const NodeChanged = uiTreeModelNodeChanged;
    };

    pub extern fn uiNewTree(m: *Model) ?*Tree;
    pub extern fn uiTreeSetExpanded(t: *Tree, node: ?*anyopaque, expanded: c_int) void;
    pub extern fn uiTreeExpanded(t: *Tree, node: ?*anyopaque) c_int;
    pub extern fn uiTreeSelection(t: *Tree) ?*anyopaque;
    pub extern fn uiTreeSetSelection(t: *Tree, node: ?*anyopaque) void;
    pub extern fn uiTreeOnSelectionChanged(t: *Tree, f: ?*const fn (?*Tree, ?*anyopaque) callconv(.c) void, data: ?*anyopaque) void;
    pub extern fn uiTreeOnNodeActivated(t: *Tree, f: ?*const fn (?*Tree, ?*anyopaque, ?*anyopaque) callconv(.c) void, data: ?*anyopaque) void;
    pub extern fn uiTreeOnNodeExpanded(t: *Tree, f: ?*const fn (?*Tree, ?*anyopaque, c_int, ?*anyopaque) callconv(.c) void, data: ?*anyopaque) void;

    pub fn New(m: *Model) !*Tree {
        return uiNewTree(m) orelse error.InitTree;
    }
    pub fn SetExpanded(t: *Tree, node: ?*anyopaque, expanded: bool) void {
        uiTreeSetExpanded(t, node, @intFromBool(expanded));
    }
    pub fn Expanded(t: *Tree, node: ?*anyopaque) bool {
        return uiTreeExpanded(t, node) != 0;
    }
    pub const Selection = uiTreeSelection;
    pub const SetSelection = uiTreeSetSelection;

    pub fn OnSelectionChanged(self: *Self, comptime T: type, comptime E: type, comptime f: *const fn (*Self, ?*T) E!void, userdata: ?*T) void {
        const callback = struct {
            fn callback(self_opt: ?*Self, t_opt: ?*anyopaque) callconv(.c) void {
                const err_ctx = ErrorContext{ .TreeOnSelectionChanged = self_opt };
                const s = self_opt orelse return error_handler(err_ctx, t_opt, error.LibUIPassedNullPointer);
                f(s, @as(?*T, @ptrCast(@alignCast(t_opt)))) catch |err| error_handler(err_ctx, t_opt, err);
            }
        }.callback;
        uiTreeOnSelectionChanged(self, callback, userdata);
    }
    pub fn OnNodeActivated(self: *Self, comptime T: type, comptime E: type, comptime f: *const fn (*Self, ?*anyopaque, ?*T) E!void, userdata: ?*T) void {
        const callback = struct {
            fn callback(self_opt: ?*Self, node: ?*anyopaque, t_opt: ?*anyopaque) callconv(.c) void {
                const err_ctx = ErrorContext{ .TreeOnNodeActivated = self_opt };
                const s = self_opt orelse return error_handler(err_ctx, t_opt, error.LibUIPassedNullPointer);
                f(s, node, @as(?*T, @ptrCast(@alignCast(t_opt)))) catch |err| error_handler(err_ctx, t_opt, err);
            }
        }.callback;
        uiTreeOnNodeActivated(self, callback, userdata);
    }
    pub fn OnNodeExpanded(self: *Self, comptime T: type, comptime E: type, comptime f: *const fn (*Self, ?*anyopaque, bool, ?*T) E!void, userdata: ?*T) void {
        const callback = struct {
            fn callback(self_opt: ?*Self, node: ?*anyopaque, expanded: c_int, t_opt: ?*anyopaque) callconv(.c) void {
                const err_ctx = ErrorContext{ .TreeOnNodeExpanded = self_opt };
                const s = self_opt orelse return error_handler(err_ctx, t_opt, error.LibUIPassedNullPointer);
                f(s, node, expanded != 0, @as(?*T, @ptrCast(@alignCast(t_opt)))) catch |err| error_handler(err_ctx, t_opt, err);
            }
        }.callback;
        uiTreeOnNodeExpanded(self, callback, userdata);
    }
};

pub const Control = ui.Control;
pub const error_handler = ui.error_handler;
pub const ErrorContext = ui.ErrorContext;

pub const ui = @import("ui.zig");
