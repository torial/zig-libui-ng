// libui-ng Haiku backend — uiEditableCombobox.
// Haiku has no native editable combo, so this is a composite: a BTextControl (the editable field)
// plus a small dropdown BButton that pops a BPopUpMenu of the appended suggestions; picking one sets
// the field's text. OnChanged fires on user edits and on a pick.
#include "uipriv_haiku.h"

struct uiEditableCombobox {
	uiHaikuControl c;
	BGroupView *view;	// horizontal: [text field | dropdown]
	BTextControl *text;
	BButton *arrow;
	BList *items;		// char*
	int suppress;		// guard so programmatic SetText (from a pick) notifies exactly once
	void (*onChanged)(uiEditableCombobox *, void *);
	void *onChangedData;
};

static void uiEditableComboboxDestroy(uiControl *cc)
{
	uiEditableCombobox *c = (uiEditableCombobox *) cc;
	for (int32 i = 0; i < c->items->CountItems(); i++)
		uiprivFree(c->items->ItemAt(i));
	delete c->items;
	if (c->view->Parent() != NULL)
		c->view->RemoveSelf();
	delete c->view;		// deletes the text field and button
	uiFreeControl(cc);
}

uiHaikuControlAllDefaultsExceptDestroy(uiEditableCombobox)

static void defaultOnChanged(uiEditableCombobox *c, void *data) { (void) c; (void) data; }

static void changedDispatch(void *control)
{
	uiEditableCombobox *c = (uiEditableCombobox *) control;
	if (c->suppress)
		return;
	(*(c->onChanged))(c, c->onChangedData);
}

static void arrowDispatch(void *control)
{
	uiEditableCombobox *c = (uiEditableCombobox *) control;
	BPopUpMenu *menu = new BPopUpMenu("uiEditableCombobox", false, false);
	for (int32 i = 0; i < c->items->CountItems(); i++)
		menu->AddItem(new BMenuItem((const char *) c->items->ItemAt(i), NULL));
	BPoint where = c->arrow->ConvertToScreen(BPoint(0, c->arrow->Bounds().bottom));
	BMenuItem *sel = menu->Go(where, false, true);
	if (sel != NULL) {
		c->suppress = 1;
		c->text->SetText(sel->Label());
		c->suppress = 0;
		(*(c->onChanged))(c, c->onChangedData);
	}
	delete menu;
}

void uiEditableComboboxAppend(uiEditableCombobox *c, const char *text)
{
	c->items->AddItem(uiHaikuStrdupText(text));
}

char *uiEditableComboboxText(uiEditableCombobox *c)
{
	return uiHaikuStrdupText(c->text->Text());
}

void uiEditableComboboxSetText(uiEditableCombobox *c, const char *text)
{
	BWindow *win = c->text->Window();
	if (win != NULL) win->Lock();
	c->suppress = 1;
	c->text->SetText(text);
	c->suppress = 0;
	if (win != NULL) win->Unlock();
}

void uiEditableComboboxOnChanged(uiEditableCombobox *c,
	void (*f)(uiEditableCombobox *, void *), void *data)
{
	c->onChanged = f;
	c->onChangedData = data;
}

char *uiEditableComboboxPlaceholder(uiEditableCombobox *c)
{
	(void) c;
	return uiHaikuStrdupText("");	// placeholder text not supported
}

void uiEditableComboboxSetPlaceholder(uiEditableCombobox *c, const char *text)
{
	(void) c; (void) text;		// placeholder text not supported
}

uiEditableCombobox *uiNewEditableCombobox(void)
{
	uiEditableCombobox *c;

	uiHaikuNewControl(uiEditableCombobox, c);
	uiControl(c)->Destroy = uiEditableComboboxDestroy;

	c->view = new BGroupView(B_HORIZONTAL, 0);
	c->text = new BTextControl(NULL, "", NULL);
	c->text->SetModificationMessage(uiprivNewEventMessage(changedDispatch, c));
	c->arrow = new BButton("uiEditableComboboxArrow", "\xE2\x96\xBE" /* ▾ */,
		uiprivNewEventMessage(arrowDispatch, c));

	BGroupLayout *gl = c->view->GroupLayout();
	gl->AddView(c->text, 1.0f);
	gl->AddView(c->arrow, 0.0f);

	c->items = new BList();
	c->suppress = 0;
	c->onChanged = defaultOnChanged;

	return c;
}
