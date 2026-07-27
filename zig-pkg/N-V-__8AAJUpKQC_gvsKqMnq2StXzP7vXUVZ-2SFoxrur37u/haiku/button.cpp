// libui-ng Haiku backend — uiButton (BButton).
#include "uipriv_haiku.h"

struct uiButton {
	uiHaikuControl c;
	BButton *view;		// field named `view` so the uiHaikuControl* macros apply
	void (*onClicked)(uiButton *, void *);
	void *onClickedData;
};

uiHaikuControlAllDefaults(uiButton)

static void buttonDispatch(void *control)
{
	uiButton *b = (uiButton *) control;
	(*(b->onClicked))(b, b->onClickedData);
}

static void defaultOnClicked(uiButton *b, void *data)
{
	(void) b; (void) data;
}

char *uiButtonText(uiButton *b)
{
	return uiHaikuStrdupText(b->view->Label());
}

void uiButtonSetText(uiButton *b, const char *text)
{
	BWindow *win = b->view->Window();
	if (win != NULL) win->Lock();
	b->view->SetLabel(text);
	if (win != NULL) win->Unlock();
}

void uiButtonOnClicked(uiButton *b, void (*f)(uiButton *, void *), void *data)
{
	b->onClicked = f;
	b->onClickedData = data;
}

uiButton *uiNewButton(const char *text)
{
	uiButton *b;

	uiHaikuNewControl(uiButton, b);
	b->view = new BButton("uiButton", text, uiprivNewEventMessage(buttonDispatch, b));
	b->onClicked = defaultOnClicked;

	return b;
}
