// GDK keyval -> the Windows virtual-key vocabulary uiWindowOnKey / uiScintillaOnKey
// speak (ui.h): letters are their ASCII uppercase, digits ASCII, F-keys 0x70.., and
// the navigation keys their VK_ values. Anything else maps to 0, which no program
// can claim. Shared by window.c and the Scintilla shim (2026-09-23).
#include "uipriv_unix.h"

int uiprivUnixKeyvalToVK(guint keyval)
{
	guint lower = gdk_keyval_to_lower(keyval);
	if (lower >= GDK_KEY_a && lower <= GDK_KEY_z)
		return 'A' + (int) (lower - GDK_KEY_a);
	if (keyval >= GDK_KEY_0 && keyval <= GDK_KEY_9)
		return '0' + (int) (keyval - GDK_KEY_0);
	if (keyval >= GDK_KEY_F1 && keyval <= GDK_KEY_F24)
		return 0x70 + (int) (keyval - GDK_KEY_F1);
	switch (keyval) {
	case GDK_KEY_Return: case GDK_KEY_KP_Enter: return 0x0D;
	case GDK_KEY_Escape: return 0x1B;
	case GDK_KEY_Tab: case GDK_KEY_ISO_Left_Tab: return 0x09;
	case GDK_KEY_BackSpace: return 0x08;
	case GDK_KEY_space: return 0x20;
	case GDK_KEY_Delete: case GDK_KEY_KP_Delete: return 0x2E;
	case GDK_KEY_Insert: case GDK_KEY_KP_Insert: return 0x2D;
	case GDK_KEY_Home: case GDK_KEY_KP_Home: return 0x24;
	case GDK_KEY_End: case GDK_KEY_KP_End: return 0x23;
	case GDK_KEY_Page_Up: case GDK_KEY_KP_Page_Up: return 0x21;
	case GDK_KEY_Page_Down: case GDK_KEY_KP_Page_Down: return 0x22;
	case GDK_KEY_Left: case GDK_KEY_KP_Left: return 0x25;
	case GDK_KEY_Up: case GDK_KEY_KP_Up: return 0x26;
	case GDK_KEY_Right: case GDK_KEY_KP_Right: return 0x27;
	case GDK_KEY_Down: case GDK_KEY_KP_Down: return 0x28;
	default: return 0;
	}
}

int uiprivUnixKeyMods(guint state)
{
	int mods = 0;
	if (state & GDK_CONTROL_MASK) mods |= 1;
	if (state & GDK_SHIFT_MASK) mods |= 2;
	if (state & GDK_MOD1_MASK) mods |= 4;
	return mods;
}
