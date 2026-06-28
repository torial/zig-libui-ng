// libui-ng Haiku backend — uiEntry (BTextControl).
// Password/search variants fall back to a plain entry for now: Haiku's BTextControl has no built-in
// echo-masking or search affordance in its public API. Placeholder text is likewise unsupported.
#include "uipriv_haiku.h"

struct uiEntry {
	uiHaikuControl c;
	BTextControl *view;
	void (*onChanged)(uiEntry *, void *);
	void *onChangedData;
};

uiHaikuControlAllDefaults(uiEntry)

static void entryDispatch(void *control)
{
	uiEntry *e = (uiEntry *) control;
	(*(e->onChanged))(e, e->onChangedData);
}

static void defaultOnChanged(uiEntry *e, void *data) { (void) e; (void) data; }

char *uiEntryText(uiEntry *e)
{
	return uiHaikuStrdupText(e->view->Text());
}

void uiEntrySetText(uiEntry *e, const char *text)
{
	BWindow *win = e->view->Window();
	if (win != NULL) win->Lock();
	e->view->SetText(text);
	if (win != NULL) win->Unlock();
}

void uiEntryOnChanged(uiEntry *e, void (*f)(uiEntry *, void *), void *data)
{
	e->onChanged = f;
	e->onChangedData = data;
}

int uiEntryReadOnly(uiEntry *e)
{
	return e->view->TextView()->IsEditable() ? 0 : 1;
}

void uiEntrySetReadOnly(uiEntry *e, int readonly)
{
	BWindow *win = e->view->Window();
	if (win != NULL) win->Lock();
	e->view->TextView()->MakeEditable(readonly ? false : true);
	if (win != NULL) win->Unlock();
}

char *uiEntryPlaceholder(uiEntry *e)
{
	(void) e;
	return uiHaikuStrdupText("");	// placeholder text not supported
}

void uiEntrySetPlaceholder(uiEntry *e, const char *text)
{
	(void) e; (void) text;		// placeholder text not supported
}

static uiEntry *finishNewEntry(void)
{
	uiEntry *e;

	uiHaikuNewControl(uiEntry, e);
	e->view = new BTextControl("uiEntry", NULL, "", NULL);
	// per-keystroke change notification (the plain message fires only on Enter / focus loss)
	e->view->SetModificationMessage(uiprivNewEventMessage(entryDispatch, e));
	e->onChanged = defaultOnChanged;

	return e;
}

uiEntry *uiNewEntry(void) { return finishNewEntry(); }
uiEntry *uiNewPasswordEntry(void) { return finishNewEntry(); }
uiEntry *uiNewSearchEntry(void) { return finishNewEntry(); }
