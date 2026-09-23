// The Scintilla shim for GTK (2026-09-23): the same uiScintilla API win.cxx gives
// Windows, over ScintillaGTK. The widget from scintilla_new() is a GtkContainer
// that scrolls itself, so it is the control's widget directly. Notifications come
// from the "sci-notify" signal (the SCNotification is the same struct Windows
// receives through WM_NOTIFY); keys from "key-press-event", which GTK runs on
// user handlers BEFORE the class handler that is Scintilla's own key processing,
// so returning TRUE consumes the key exactly as the Windows subclass does -- and
// no separate WM_CHAR swallow is needed, because GTK generates the character
// inside the same event.
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <ui.h>
#include <ui_unix.h>
#include <Scintilla.h>
#include <ScintillaWidget.h>
#include <ui_scintilla.h>

extern "C" int uiprivUnixKeyvalToVK(guint keyval);
extern "C" int uiprivUnixKeyMods(guint state);

#define uiScintilla(this) ((uiScintilla *) (this))
#define uiScintillaSignature 0x1234

struct uiScintilla {
	uiUnixControl c;
	GtkWidget *widget;
	ScintillaObject *sci;
	void (*onNotify)(uiScintilla *, SCNotification *, void *);
	void *onNotifyData;
	int (*onKey)(uiScintilla *, int, int, void *);
	void *onKeyData;
};

uiUnixControlAllDefaults(uiScintilla)

static void onSciNotify(ScintillaObject *sci, gint id, SCNotification *n, gpointer data)
{
	uiScintilla *s = uiScintilla(data);

	if (s->onNotify != NULL)
		(*(s->onNotify))(s, n, s->onNotifyData);
}

static gboolean onKeyPress(GtkWidget *widget, GdkEventKey *e, gpointer data)
{
	uiScintilla *s = uiScintilla(data);
	int vk;

	if (s->onKey == NULL)
		return FALSE;
	vk = uiprivUnixKeyvalToVK(e->keyval);
	if (vk == 0)
		return FALSE;
	if ((*(s->onKey))(s, vk, uiprivUnixKeyMods(e->state), s->onKeyData))
		return TRUE;
	return FALSE;
}

void uiScintillaOnNotify(uiScintilla *s, void (*f)(uiScintilla *, SCNotification *, void *), void *data)
{
	s->onNotify = f;
	s->onNotifyData = data;
}

void uiScintillaOnKey(uiScintilla *s, int (*f)(uiScintilla *, int, int, void *), void *data)
{
	s->onKey = f;
	s->onKeyData = data;
}

uiScintilla *uiNewScintilla(void)
{
	uiScintilla *s;

	uiUnixNewControl(uiScintilla, s);

	s->widget = scintilla_new();
	s->sci = SCINTILLA(s->widget);
	s->onNotify = NULL;
	s->onNotifyData = NULL;
	s->onKey = NULL;
	s->onKeyData = NULL;
	// a bare Scintilla is 0x0 in a box; ask for the same floor the Windows shim reports
	gtk_widget_set_size_request(s->widget, 100, 100);
	gtk_widget_set_hexpand(s->widget, TRUE);
	gtk_widget_set_vexpand(s->widget, TRUE);
	g_signal_connect(s->widget, SCINTILLA_NOTIFY, G_CALLBACK(onSciNotify), s);
	g_signal_connect(s->widget, "key-press-event", G_CALLBACK(onKeyPress), s);
	return s;
}

uintptr_t uiScintillaSendMessage(uiScintilla *s, uint32_t code, uintptr_t w, uintptr_t l)
{
	return (uintptr_t) scintilla_send_message(s->sci, code, (uptr_t) w, (sptr_t) l);
}

void uiScintillaGetRange(uiScintilla *s, unsigned int start, unsigned int end, char *text)
{
	Sci_TextRangeFull tr;

	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText = text;
	scintilla_send_message(s->sci, SCI_GETTEXTRANGEFULL, 0, (sptr_t) &tr);
}

void uiScintillaSetText(uiScintilla *s, const char *text, unsigned int len)
{
	scintilla_send_message(s->sci, SCI_SETTEXT, len, (sptr_t) text);
}

void uiScintillaAppend(uiScintilla *s, const char *text)
{
	scintilla_send_message(s->sci, SCI_APPENDTEXT, strlen(text), (sptr_t) text);
}

unsigned int uiScintillaGetLength(uiScintilla *s)
{
	return (unsigned int) scintilla_send_message(s->sci, SCI_GETLENGTH, 0, 0);
}

char *uiScintillaText(uiScintilla *s)
{
	unsigned int len = uiScintillaGetLength(s);
	// len + 1: GetTextRange writes the terminating NUL (see win.cxx)
	char *text = (char *) malloc((size_t) len + 1);

	uiScintillaGetRange(s, 0, len, text);
	text[len] = '\0';
	return text;
}
