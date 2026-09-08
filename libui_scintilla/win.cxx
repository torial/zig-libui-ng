// A simple demonstration application using Scintilla
#include <stdio.h>
#include <windows.h>
#include <richedit.h>
#include <commctrl.h>
#include <ui.h>
#include <Scintilla.h>
#include <ScintillaTypes.h>
#include <ScintillaWin.h>
#include <ui_windows.h>
extern HWND utilWindow;
extern HINSTANCE hInstance;
extern void uiprivDestroyTooltip(uiControl* c);
#include <ui_scintilla.h>

#define uiScintilla(this) ((uiScintilla *)this)
#define uiScintillaSignature 0x1234

struct uiScintilla {
	struct uiWindowsControl c;
	HWND hwnd;
	void (*onNotify)(uiScintilla *, SCNotification *, void *);
	void *onNotifyData;
	int (*onKey)(uiScintilla *, int, int, void *);
	void *onKeyData;
	int swallowChar;
};

// Keys. Windows delivers WM_KEYDOWN to the focused window, i.e. to Scintilla
// itself, so nothing in libui-ng sees it; a window subclass (SetWindowSubclass,
// comctl32) is the standard way to look first. The handler is asked with the
// virtual key and the modifier state; returning nonzero CONSUMES the key — and
// the WM_CHAR that follows a consumed Ctrl+letter, which would otherwise insert
// the control character (Scintilla adds a control char when the key-down was
// not consumed by it: ScintillaWin.cxx, WM_CHAR + lastKeyDownConsumed).
#define uiScintillaKeyCtrl  1
#define uiScintillaKeyShift 2
#define uiScintillaKeyAlt   4

static LRESULT CALLBACK sciSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	uiScintilla *s = reinterpret_cast<uiScintilla *>(dwRefData);
	switch (uMsg) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (s->onKey != NULL) {
			int mods = 0;
			if (GetKeyState(VK_CONTROL) & 0x8000) mods |= uiScintillaKeyCtrl;
			if (GetKeyState(VK_SHIFT) & 0x8000) mods |= uiScintillaKeyShift;
			if (GetKeyState(VK_MENU) & 0x8000) mods |= uiScintillaKeyAlt;
			if ((*(s->onKey))(s, (int) wParam, mods, s->onKeyData)) {
				s->swallowChar = 1;
				return 0;
			}
		}
		s->swallowChar = 0;
		break;
	case WM_CHAR:
	case WM_SYSCHAR:
		if (s->swallowChar) {
			s->swallowChar = 0;
			return 0;
		}
		break;
	// The flag must not outlive the keystroke that armed it: a consumed F-key
	// produces no WM_CHAR, and an armed flag would then eat the next WM_CHAR
	// that arrives WITHOUT a key-down through this subclass (an IME commit, an
	// injected WM_CHAR). Refuter finding, 2026-09-08 (harness case 5).
	case WM_KEYUP:
	case WM_SYSKEYUP:
	case WM_KILLFOCUS:
		s->swallowChar = 0;
		break;
	case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd, sciSubclassProc, uIdSubclass);
		break;
	}
	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

_UI_EXTERN void uiScintillaOnKey(uiScintilla *s, int (*f)(uiScintilla *, int, int, void *), void *data)
{
	s->onKey = f;
	s->onKeyData = data;
}

// Notifications. Scintilla sends WM_NOTIFY to its PARENT; libui-ng already routes
// every container's WM_NOTIFY to a per-child-HWND handler (windows/events.cpp,
// uiWindowsRegisterWM_NOTIFYHandler — the same hook uiTab and uiTable use), so
// no subclassing is needed: register once at creation, forward the SCNotification
// to whoever asked. shouldRun() in events.cpp ignores children of the utility
// window, which is exactly right: before the control is parented into a box no
// user can be listening.
static BOOL onWM_NOTIFY(uiControl *c, HWND hwnd, NMHDR *nmhdr, LRESULT *lResult)
{
	uiScintilla *s = uiScintilla(c);
	if (s->onNotify != NULL)
		(*(s->onNotify))(s, reinterpret_cast<SCNotification *>(nmhdr), s->onNotifyData);
	*lResult = 0;
	return TRUE;
}

static void uiScintillaDestroy(uiControl *c)
{
	uiScintilla *s = uiScintilla(c);
	uiWindowsUnregisterWM_NOTIFYHandler(s->hwnd);
	uiWindowsEnsureDestroyWindow(s->hwnd);
	uiFreeControl(uiControl(s));
}

uiWindowsControlAllDefaultsExceptDestroy(uiScintilla);

_UI_EXTERN void uiScintillaOnNotify(uiScintilla *s, void (*f)(uiScintilla *, SCNotification *, void *), void *data)
{
	s->onNotify = f;
	s->onNotifyData = data;
}

static void uiScintillaMinimumSize(uiWindowsControl *c, int *width, int *height) {
	*width = 100;
	*height = 100;
}

_UI_EXTERN uiScintilla *uiNewScintilla() {
	struct uiScintilla *s;
	uiWindowsNewControl(uiScintilla, s);

	static int init_flag = 1;
	if (init_flag) {
		Scintilla::Internal::RegisterClasses(hInstance);
		init_flag = 0;
	}

	s->hwnd = CreateWindowEx(0,
		"Scintilla", "Source", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
		0, 0, 100, 100, utilWindow, NULL, hInstance, NULL);
	s->onNotify = NULL;
	s->onNotifyData = NULL;
	s->onKey = NULL;
	s->onKeyData = NULL;
	s->swallowChar = 0;
	uiWindowsRegisterWM_NOTIFYHandler(s->hwnd, onWM_NOTIFY, uiControl(s));
	SetWindowSubclass(s->hwnd, sciSubclassProc, 1, reinterpret_cast<DWORD_PTR>(s));

	return s;
}

uintptr_t uiScintillaSendMessage(uiScintilla *s, uint32_t code, uintptr_t w, uintptr_t l) {
	return SendMessage(s->hwnd, code, w, l);
}

void uiScintillaGetRange(uiScintilla *s, unsigned int start, unsigned int end, char *text) {
	TEXTRANGE tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText = reinterpret_cast<LPSTR>(text);
	SendMessage(s->hwnd, SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
}

void uiScintillaSetText(uiScintilla *s, const char *text, unsigned int len) {
	SendMessage(s->hwnd, SCI_SETTEXT, len, reinterpret_cast<LPARAM>(text));
}

void uiScintillaAppend(uiScintilla *s, const char *text) {
	SendMessage(s->hwnd, SCI_APPENDTEXT, strlen(text), reinterpret_cast<LPARAM>(text));
}

unsigned int uiScintillaGetLength(uiScintilla *s) {
	return SendMessage(s->hwnd, SCI_GETLENGTH, 0, 0);
}

char *uiScintillaText(uiScintilla *s) {
	unsigned int len = uiScintillaGetLength(s);
	// Scintilla's GetTextRange writes a NUL at buffer[len] (Editor.cxx, "Spec says copied
	// text is terminated with a NUL"): the buffer needs len + 1. Pre-existing one-byte
	// heap overflow on every call; refuter bycatch, 2026-09-08.
	char *text = (char *)malloc((size_t)len + 1);
	uiScintillaGetRange(s, 0, len, text);
	text[len] = '\0';
	return text;
}
