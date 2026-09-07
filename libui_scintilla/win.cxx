// A simple demonstration application using Scintilla
#include <stdio.h>
#include <windows.h>
#include <richedit.h>
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
};

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
	uiWindowsRegisterWM_NOTIFYHandler(s->hwnd, onWM_NOTIFY, uiControl(s));

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
	char *text = (char *)malloc((size_t)len);
	uiScintillaGetRange(s, 0, len, text);
	return text;
}
