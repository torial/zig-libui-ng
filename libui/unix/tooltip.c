// 23 september 2026 -- tooltips and the clipboard on GTK (torial fork)
#include "uipriv_unix.h"

void uiControlSetTooltip(uiControl *c, const char *text)
{
	GtkWidget *w = GTK_WIDGET(uiControlHandle(c));

	if (text != NULL && *text == '\0')
		text = NULL;
	gtk_widget_set_tooltip_text(w, text);
}

char *uiClipboardText(void)
{
	GtkClipboard *cb;
	gchar *s;
	char *out;

	cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	s = gtk_clipboard_wait_for_text(cb);
	if (s == NULL)
		return NULL;
	out = uiUnixStrdupText(s);
	g_free(s);
	return out;
}

void uiClipboardSetText(const char *text)
{
	GtkClipboard *cb;

	cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text(cb, text == NULL ? "" : text, -1);
	gtk_clipboard_store(cb);
}
