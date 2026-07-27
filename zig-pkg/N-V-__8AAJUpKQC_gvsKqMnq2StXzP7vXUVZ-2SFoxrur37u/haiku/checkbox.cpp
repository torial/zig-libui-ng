// libui-ng Haiku backend — uiCheckbox (BCheckBox).
#include "uipriv_haiku.h"

struct uiCheckbox {
	uiHaikuControl c;
	BCheckBox *view;
	void (*onToggled)(uiCheckbox *, void *);
	void *onToggledData;
};

uiHaikuControlAllDefaults(uiCheckbox)

static void checkboxDispatch(void *control)
{
	uiCheckbox *c = (uiCheckbox *) control;
	(*(c->onToggled))(c, c->onToggledData);
}

static void defaultOnToggled(uiCheckbox *c, void *data) { (void) c; (void) data; }

char *uiCheckboxText(uiCheckbox *c)
{
	return uiHaikuStrdupText(c->view->Label());
}

void uiCheckboxSetText(uiCheckbox *c, const char *text)
{
	BWindow *win = c->view->Window();
	if (win != NULL) win->Lock();
	c->view->SetLabel(text);
	if (win != NULL) win->Unlock();
}

void uiCheckboxOnToggled(uiCheckbox *c, void (*f)(uiCheckbox *, void *), void *data)
{
	c->onToggled = f;
	c->onToggledData = data;
}

int uiCheckboxChecked(uiCheckbox *c)
{
	return c->view->Value() == B_CONTROL_ON ? 1 : 0;
}

void uiCheckboxSetChecked(uiCheckbox *c, int checked)
{
	BWindow *win = c->view->Window();
	if (win != NULL) win->Lock();
	c->view->SetValue(checked ? B_CONTROL_ON : B_CONTROL_OFF);
	if (win != NULL) win->Unlock();
}

uiCheckbox *uiNewCheckbox(const char *text)
{
	uiCheckbox *c;

	uiHaikuNewControl(uiCheckbox, c);
	c->view = new BCheckBox("uiCheckbox", text, uiprivNewEventMessage(checkboxDispatch, c));
	c->onToggled = defaultOnToggled;

	return c;
}
