const ui = @import("ui");

pub const Scintilla = opaque {
    pub fn new() !*Scintilla {
        return uiNewScintilla() orelse error.CreateFailed;
    }

    pub fn setText(self: *Scintilla, text: []const u8) void {
        uiScintillaSetText(self, text.ptr, @intCast(text.len));
    }

    pub fn getLength(self: *Scintilla) usize {
        return @intCast(uiScintillaGetLength(self));
    }

    pub fn getRange(self: *Scintilla, start: usize, end: usize, buf: [*]u8) void {
        uiScintillaGetRange(self, @intCast(start), @intCast(end), buf);
    }

    pub fn sendMessage(self: *Scintilla, code: u32, w: usize, l: usize) usize {
        return @intCast(uiScintillaSendMessage(self, code, @intCast(w), @intCast(l)));
    }

    /// A Scintilla notification as the control reports it (struct SCNotification in
    /// Scintilla.h, laid out for C). Only the fields the IDE needs are named; the
    /// rest are kept in order so the offsets stay right. Valid for the callback only.
    pub const Notification = extern struct {
        hwndFrom: ?*anyopaque,
        idFrom: usize,
        code: c_uint,           // SCN_* (2000-range) — SCN_MODIFIED 2008, SCN_CHARADDED 2001, SCN_MARGINCLICK 2010, SCN_DWELLSTART 2016 ...
        position: isize,
        ch: c_int,
        modifiers: c_int,
        modificationType: c_int, // SC_MOD_INSERTTEXT 0x1, SC_MOD_DELETETEXT 0x2, ...
        text: ?[*]const u8,
        length: isize,
        linesAdded: isize,
        message: c_int,
        wParam: usize,
        lParam: isize,
        line: isize,
        foldLevelNow: c_int,
        foldLevelPrev: c_int,
        margin: c_int,
        listType: c_int,
        x: c_int,
        y: c_int,
        token: c_int,
        annotationLinesAdded: isize,
        updated: c_int,
        listCompletionMethod: c_int,
        characterSource: c_int,
    };

    /// Receive Scintilla notifications (SCN_*). One handler per control; libui
    /// delivers them on the UI thread through its WM_NOTIFY routing (windows only —
    /// the Haiku/other platform layers have no shim yet, so this is a no-op there
    /// only in the sense that the extern is never called).
    pub fn OnNotify(self: *Scintilla, comptime T: type, comptime E: type, comptime f: *const fn (*Scintilla, *const Notification, ?*T) E!void, userdata: ?*T) void {
        const callback = struct {
            fn callback(s_opt: ?*Scintilla, n_opt: ?*const Notification, t_opt: ?*anyopaque) callconv(.c) void {
                const s = s_opt orelse return;
                const n = n_opt orelse return;
                f(s, n, @as(?*T, @ptrCast(@alignCast(t_opt)))) catch {};
            }
        }.callback;
        uiScintillaOnNotify(self, callback, userdata);
    }

    pub const KeyMods = struct {
        pub const ctrl: c_int = 1;
        pub const shift: c_int = 2;
        pub const alt: c_int = 4;
    };

    /// See a key before Scintilla does. `f` returns true to CONSUME it (Scintilla
    /// never sees it, nor the character it would have inserted), false to let it
    /// through. `vk` is the Windows virtual-key code; `mods` is a KeyMods bitmask.
    pub fn OnKey(self: *Scintilla, comptime T: type, comptime f: *const fn (*Scintilla, c_int, c_int, ?*T) bool, userdata: ?*T) void {
        const callback = struct {
            fn callback(s_opt: ?*Scintilla, vk: c_int, mods: c_int, t_opt: ?*anyopaque) callconv(.c) c_int {
                const s = s_opt orelse return 0;
                return if (f(s, vk, mods, @as(?*T, @ptrCast(@alignCast(t_opt))))) 1 else 0;
            }
        }.callback;
        uiScintillaOnKey(self, callback, userdata);
    }

    pub fn as_control(self: *Scintilla) *ui.Control {
        // Scintilla is `opaque` (alignment 1); Control has alignment 8. Zig 0.16
        // requires an explicit @alignCast to widen the pointer alignment — the
        // other widget bindings (ui.zig) all use this @ptrCast(@alignCast(...))
        // form. This path is only compiled when a program references a code
        // editor, so the missing @alignCast stayed latent until first use.
        return @ptrCast(@alignCast(self));
    }
};

extern fn uiNewScintilla() ?*Scintilla;
extern fn uiScintillaSetText(s: *Scintilla, text: [*]const u8, len: c_uint) void;
extern fn uiScintillaGetRange(s: *Scintilla, start: c_uint, end: c_uint, text: [*]u8) void;
extern fn uiScintillaGetLength(s: *Scintilla) c_uint;
extern fn uiScintillaSendMessage(s: *Scintilla, code: u32, w: usize, l: usize) usize;
extern fn uiScintillaOnNotify(s: *Scintilla, f: ?*const fn (?*Scintilla, ?*const Scintilla.Notification, ?*anyopaque) callconv(.c) void, data: ?*anyopaque) void;
extern fn uiScintillaOnKey(s: *Scintilla, f: ?*const fn (?*Scintilla, c_int, c_int, ?*anyopaque) callconv(.c) c_int, data: ?*anyopaque) void;
