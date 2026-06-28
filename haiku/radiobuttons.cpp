// libui-ng Haiku backend — uiRadioButtons (a vertical BGroupView of BRadioButtons).
// Radio buttons sharing a parent are mutually exclusive on Haiku automatically, and each fires its
// message on selection.
#include "uipriv_haiku.h"

struct uiRadioButtons {
	uiHaikuControl c;
	BGroupView *view;
	BList *buttons;		// BRadioButton* in append order
	void (*onSelected)(uiRadioButtons *, void *);
	void *onSelectedData;
};

static void uiRadioButtonsDestroy(uiControl *cc)
{
	uiRadioButtons *r = (uiRadioButtons *) cc;
	delete r->buttons;	// the BRadioButtons themselves are owned by (and freed with) the view
	if (r->view->Parent() != NULL)
		r->view->RemoveSelf();
	delete r->view;
	uiFreeControl(cc);
}

uiHaikuControlAllDefaultsExceptDestroy(uiRadioButtons)

static void radioDispatch(void *control)
{
	uiRadioButtons *r = (uiRadioButtons *) control;
	(*(r->onSelected))(r, r->onSelectedData);
}

static void defaultOnSelected(uiRadioButtons *r, void *data) { (void) r; (void) data; }

void uiRadioButtonsAppend(uiRadioButtons *r, const char *text)
{
	BWindow *win = r->view->Window();
	if (win != NULL) win->Lock();
	BRadioButton *rb = new BRadioButton("uiRadioButton", text,
		uiprivNewEventMessage(radioDispatch, r));
	r->view->AddChild(rb);
	r->buttons->AddItem(rb);
	if (win != NULL) win->Unlock();
}

int uiRadioButtonsSelected(uiRadioButtons *r)
{
	for (int32 i = 0; i < r->buttons->CountItems(); i++) {
		BRadioButton *rb = (BRadioButton *) r->buttons->ItemAt(i);
		if (rb->Value() == B_CONTROL_ON)
			return (int) i;
	}
	return -1;
}

void uiRadioButtonsSetSelected(uiRadioButtons *r, int index)
{
	BWindow *win = r->view->Window();
	if (win != NULL) win->Lock();
	for (int32 i = 0; i < r->buttons->CountItems(); i++) {
		BRadioButton *rb = (BRadioButton *) r->buttons->ItemAt(i);
		rb->SetValue(((int) i == index) ? B_CONTROL_ON : B_CONTROL_OFF);
	}
	if (win != NULL) win->Unlock();
}

void uiRadioButtonsOnSelected(uiRadioButtons *r, void (*f)(uiRadioButtons *, void *), void *data)
{
	r->onSelected = f;
	r->onSelectedData = data;
}

uiRadioButtons *uiNewRadioButtons(void)
{
	uiRadioButtons *r;

	uiHaikuNewControl(uiRadioButtons, r);
	uiControl(r)->Destroy = uiRadioButtonsDestroy;

	r->view = new BGroupView(B_VERTICAL, 0);
	r->buttons = new BList();
	r->onSelected = defaultOnSelected;

	return r;
}
