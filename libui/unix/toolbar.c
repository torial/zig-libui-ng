// 23 september 2026 -- uiToolbar on GTK: a GtkToolbar the window packs under its menubar.
#include "uipriv_unix.h"

struct uiToolbar {
	GtkWidget *widget;
	GtkToolbar *toolbar;
	int nitems;
	void (*onClicked)(uiToolbar *, int, void *);
	void *onClickedData;
};

static void defaultOnClicked(uiToolbar *t, int index, void *data)
{
	// do nothing
}

static void onItemClicked(GtkToolButton *b, gpointer data)
{
	uiToolbar *t = (uiToolbar *) data;
	int index;

	index = gtk_toolbar_get_item_index(t->toolbar, GTK_TOOL_ITEM(b));
	(*(t->onClicked))(t, index, t->onClickedData);
}

uiToolbar *uiNewToolbar(void)
{
	uiToolbar *t;

	t = uiprivNew(uiToolbar);
	t->widget = gtk_toolbar_new();
	t->toolbar = GTK_TOOLBAR(t->widget);
	// icons with the label beside them; an item without an icon is its label
	gtk_toolbar_set_style(t->toolbar, GTK_TOOLBAR_BOTH_HORIZ);
	gtk_toolbar_set_icon_size(t->toolbar, GTK_ICON_SIZE_SMALL_TOOLBAR);
	t->onClicked = defaultOnClicked;
	t->onClickedData = NULL;
	return t;
}

// the window's half (window.c keeps the vbox): pack under the menubar, above the child;
// the widget dies with the window, this frees the record
void uiprivFreeToolbar(uiToolbar *t)
{
	uiprivFree(t);
}

GtkWidget *uiprivToolbarWidget(uiToolbar *t)
{
	return t->widget;
}

int uiToolbarAppendItem(uiToolbar *t, const char *label, uiImage *icon, const char *tooltip)
{
	GtkToolItem *item;
	GtkWidget *image = NULL;
	cairo_surface_t *cs;
	GdkPixbuf *pb;

	if (icon != NULL) {
		cs = uiprivImageAppropriateSurface(icon, t->widget);
		if (cs != NULL) {
			pb = gdk_pixbuf_get_from_surface(cs, 0, 0,
				cairo_image_surface_get_width(cs), cairo_image_surface_get_height(cs));
			if (pb != NULL) {
				image = gtk_image_new_from_pixbuf(pb);
				g_object_unref(pb);
			}
		}
	}
	item = gtk_tool_button_new(image, label);
	// BOTH_HORIZ shows the label only for important items; every item here is
	gtk_tool_item_set_is_important(item, TRUE);
	if (tooltip != NULL)
		gtk_tool_item_set_tooltip_text(item, tooltip);
	g_signal_connect(item, "clicked", G_CALLBACK(onItemClicked), t);
	gtk_toolbar_insert(t->toolbar, item, -1);
	gtk_widget_show_all(GTK_WIDGET(item));
	t->nitems++;
	return t->nitems - 1;
}

int uiToolbarAppendSeparator(uiToolbar *t)
{
	GtkToolItem *item;

	item = gtk_separator_tool_item_new();
	gtk_toolbar_insert(t->toolbar, item, -1);
	gtk_widget_show_all(GTK_WIDGET(item));
	t->nitems++;
	return t->nitems - 1;
}

int uiToolbarNumItems(uiToolbar *t)
{
	return t->nitems;
}

static void removeItem(GtkWidget *child, gpointer data)
{
	gtk_container_remove(GTK_CONTAINER(data), child);
}

void uiToolbarClear(uiToolbar *t)
{
	gtk_container_foreach(GTK_CONTAINER(t->widget), removeItem, t->widget);
	t->nitems = 0;
}

void uiToolbarSetItemEnabled(uiToolbar *t, int index, int enabled)
{
	GtkToolItem *item;

	item = gtk_toolbar_get_nth_item(t->toolbar, index);
	if (item != NULL)
		gtk_widget_set_sensitive(GTK_WIDGET(item), enabled != 0);
}

void uiToolbarOnClicked(uiToolbar *t, void (*f)(uiToolbar *, int, void *), void *data)
{
	t->onClicked = f;
	t->onClickedData = data;
}
