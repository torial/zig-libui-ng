// 16 august 2015
#include "uipriv_unix.h"

void uiUnixControlSetContainer(uiUnixControl *c, GtkContainer *container, gboolean remove)
{
	(*(c->SetContainer))(c, container, remove);
}

#define uiUnixControlSignature 0x556E6978

uiUnixControl *uiUnixAllocControl(size_t n, uint32_t typesig, const char *typenamestr)
{
	return uiUnixControl(uiAllocControl(n, uiUnixControlSignature, typesig, typenamestr));
}

// uiControlSetMinSize() hint (torial fork): GTK has size requests; -1 clears.
void uiprivControlMinSizeChanged(uiControl *c)
{
	GtkWidget *w;

	w = GTK_WIDGET(uiControlHandle(c));
	if (w == NULL)
		return;
	gtk_widget_set_size_request(w,
		c->MinWidth > 0 ? c->MinWidth : -1,
		c->MinHeight > 0 ? c->MinHeight : -1);
}
