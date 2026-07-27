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
