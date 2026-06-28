// libui-ng Haiku backend — uiMultilineEntry (BTextView inside a BScrollView).
// BTextView has no change notification, so we subclass it and fire OnChanged from InsertText/
// DeleteText. Programmatic edits (SetText/Append) set `suppress` so only user edits notify.
#include <string.h>
#include "uipriv_haiku.h"

struct uiMultilineEntry;

class UiTextView : public BTextView {
public:
	uiMultilineEntry *owner;
	bool suppress;
	UiTextView(const char *name)
		: BTextView(name, B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE | B_FRAME_EVENTS)
	{
		this->owner = NULL;
		this->suppress = false;
	}
	void notifyChanged();
	virtual void InsertText(const char *text, int32 length, int32 offset,
		const text_run_array *runs)
	{
		BTextView::InsertText(text, length, offset, runs);
		if (!this->suppress) notifyChanged();
	}
	virtual void DeleteText(int32 from, int32 to)
	{
		BTextView::DeleteText(from, to);
		if (!this->suppress) notifyChanged();
	}
};

struct uiMultilineEntry {
	uiHaikuControl c;
	BScrollView *view;	// outer (added to parents); macros operate on this
	UiTextView *text;	// inner editor
	void (*onChanged)(uiMultilineEntry *, void *);
	void *onChangedData;
};

void UiTextView::notifyChanged()
{
	if (this->owner != NULL)
		(*(this->owner->onChanged))(this->owner, this->owner->onChangedData);
}

static void uiMultilineEntryDestroy(uiControl *cc)
{
	uiMultilineEntry *e = (uiMultilineEntry *) cc;
	if (e->view->Parent() != NULL)
		e->view->RemoveSelf();
	delete e->view;		// deletes the contained BTextView too
	uiFreeControl(cc);
}

uiHaikuControlAllDefaultsExceptDestroy(uiMultilineEntry)

static void defaultOnChanged(uiMultilineEntry *e, void *data) { (void) e; (void) data; }

char *uiMultilineEntryText(uiMultilineEntry *e)
{
	return uiHaikuStrdupText(e->text->Text());
}

void uiMultilineEntrySetText(uiMultilineEntry *e, const char *text)
{
	BWindow *win = e->text->Window();
	if (win != NULL) win->Lock();
	e->text->suppress = true;
	e->text->SetText(text);
	e->text->suppress = false;
	if (win != NULL) win->Unlock();
}

void uiMultilineEntryAppend(uiMultilineEntry *e, const char *text)
{
	BWindow *win = e->text->Window();
	if (win != NULL) win->Lock();
	e->text->suppress = true;
	int32 end = e->text->TextLength();
	e->text->Insert(end, text, strlen(text));
	e->text->suppress = false;
	if (win != NULL) win->Unlock();
}

void uiMultilineEntryOnChanged(uiMultilineEntry *e, void (*f)(uiMultilineEntry *, void *), void *data)
{
	e->onChanged = f;
	e->onChangedData = data;
}

int uiMultilineEntryReadOnly(uiMultilineEntry *e)
{
	return e->text->IsEditable() ? 0 : 1;
}

void uiMultilineEntrySetReadOnly(uiMultilineEntry *e, int readonly)
{
	BWindow *win = e->text->Window();
	if (win != NULL) win->Lock();
	e->text->MakeEditable(readonly ? false : true);
	if (win != NULL) win->Unlock();
}

static uiMultilineEntry *finishNewMultilineEntry(bool wrap)
{
	uiMultilineEntry *e;

	uiHaikuNewControl(uiMultilineEntry, e);
	uiControl(e)->Destroy = uiMultilineEntryDestroy;

	e->text = new UiTextView("uiMultilineEntry");
	e->text->owner = e;
	e->text->SetWordWrap(wrap);
	e->text->MakeEditable(true);
	// The scroll view is the control's outer widget; vertical scroller always, horizontal when not wrapping.
	e->view = new BScrollView("uiMultilineEntryScroll", e->text, 0, !wrap, true);
	e->onChanged = defaultOnChanged;

	return e;
}

uiMultilineEntry *uiNewMultilineEntry(void) { return finishNewMultilineEntry(true); }
uiMultilineEntry *uiNewNonWrappingMultilineEntry(void) { return finishNewMultilineEntry(false); }
