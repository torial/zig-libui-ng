// libui-ng Haiku backend — uiLabel (BStringView).
#include "uipriv_haiku.h"

struct uiLabel {
	uiHaikuControl c;
	BStringView *view;
};

uiHaikuControlAllDefaults(uiLabel)

char *uiLabelText(uiLabel *l)
{
	return uiHaikuStrdupText(l->view->Text());
}

void uiLabelSetText(uiLabel *l, const char *text)
{
	BWindow *win = l->view->Window();
	if (win != NULL) win->Lock();
	l->view->SetText(text);
	if (win != NULL) win->Unlock();
}

uiLabel *uiNewLabel(const char *text)
{
	uiLabel *l;

	uiHaikuNewControl(uiLabel, l);
	l->view = new BStringView("uiLabel", text);

	return l;
}
