// libui-ng Haiku backend — uiCombobox (BOptionPopUp, the public dropdown control).
// BOptionPopUp selects by an option's integer "value"; we keep an items list as the source of truth
// and rebuild the popup so each option's value equals its index, giving libui's index semantics.
#include "uipriv_haiku.h"

struct uiCombobox {
	uiHaikuControl c;
	BOptionPopUp *view;
	BList *items;		// char* (strdup'd), in order
	int selected;		// -1 = none chosen yet
	void (*onSelected)(uiCombobox *, void *);
	void *onSelectedData;
};

static void uiComboboxDestroy(uiControl *cc)
{
	uiCombobox *c = (uiCombobox *) cc;
	for (int32 i = 0; i < c->items->CountItems(); i++)
		uiprivFree(c->items->ItemAt(i));
	delete c->items;
	if (c->view->Parent() != NULL)
		c->view->RemoveSelf();
	delete c->view;
	uiFreeControl(cc);
}

uiHaikuControlAllDefaultsExceptDestroy(uiCombobox)

static void comboDispatch(void *control)
{
	uiCombobox *c = (uiCombobox *) control;
	c->selected = c->view->SelectedOption(NULL, NULL);
	(*(c->onSelected))(c, c->onSelectedData);
}

static void defaultOnSelected(uiCombobox *c, void *data) { (void) c; (void) data; }

// Rebuild the popup from items so value == index; restore the selection if still valid.
static void sync(uiCombobox *c)
{
	BWindow *win = c->view->Window();
	if (win != NULL) win->Lock();
	while (c->view->CountOptions() > 0)
		c->view->RemoveOptionAt(0);
	for (int32 i = 0; i < c->items->CountItems(); i++)
		c->view->AddOptionAt((const char *) c->items->ItemAt(i), (int32) i, (int32) i);
	if (c->selected >= 0 && c->selected < c->items->CountItems())
		c->view->SetValue(c->selected);
	if (win != NULL) win->Unlock();
}

void uiComboboxAppend(uiCombobox *c, const char *text)
{
	c->items->AddItem(uiHaikuStrdupText(text));
	sync(c);
}

void uiComboboxInsertAt(uiCombobox *c, int index, const char *text)
{
	c->items->AddItem(uiHaikuStrdupText(text), index);
	if (c->selected >= index)
		c->selected++;
	sync(c);
}

void uiComboboxDelete(uiCombobox *c, int index)
{
	char *s = (char *) c->items->RemoveItem(index);
	if (s != NULL) uiprivFree(s);
	if (c->selected == index)
		c->selected = -1;
	else if (c->selected > index)
		c->selected--;
	sync(c);
}

void uiComboboxClear(uiCombobox *c)
{
	for (int32 i = 0; i < c->items->CountItems(); i++)
		uiprivFree(c->items->ItemAt(i));
	c->items->MakeEmpty();
	c->selected = -1;
	sync(c);
}

int uiComboboxNumItems(uiCombobox *c)
{
	return (int) c->items->CountItems();
}

int uiComboboxSelected(uiCombobox *c)
{
	return c->selected;
}

void uiComboboxSetSelected(uiCombobox *c, int index)
{
	c->selected = index;
	BWindow *win = c->view->Window();
	if (win != NULL) win->Lock();
	if (index >= 0 && index < c->items->CountItems())
		c->view->SetValue(index);
	if (win != NULL) win->Unlock();
}

void uiComboboxOnSelected(uiCombobox *c, void (*f)(uiCombobox *, void *), void *data)
{
	c->onSelected = f;
	c->onSelectedData = data;
}

uiCombobox *uiNewCombobox(void)
{
	uiCombobox *c;

	uiHaikuNewControl(uiCombobox, c);
	uiControl(c)->Destroy = uiComboboxDestroy;
	c->view = new BOptionPopUp("uiCombobox", NULL, uiprivNewEventMessage(comboDispatch, c));
	c->items = new BList();
	c->selected = -1;
	c->onSelected = defaultOnSelected;

	return c;
}
