// libui-ng Haiku backend — uiProgressBar (BStatusBar).
// libui's indeterminate state (value -1) has no native BStatusBar animation; we store the value and
// leave the bar unchanged in that case.
#include <stdio.h>
#include "uipriv_haiku.h"

struct uiProgressBar {
	uiHaikuControl c;
	BStatusBar *view;
	int value;
};

uiHaikuControlAllDefaults(uiProgressBar)

int uiProgressBarValue(uiProgressBar *p)
{
	return p->value;
}

void uiProgressBarSetValue(uiProgressBar *p, int n)
{
	p->value = n;
	if (n < 0)
		return;			// indeterminate: not animated by BStatusBar
	BWindow *win = p->view->Window();
	if (win != NULL) win->Lock();
	p->view->SetTo((float) n);
	if (win != NULL) win->Unlock();
}

uiProgressBar *uiNewProgressBar(void)
{
	uiProgressBar *p;

	uiHaikuNewControl(uiProgressBar, p);
	p->view = new BStatusBar("uiProgressBar");
	p->view->SetMaxValue(100.0f);
	p->value = 0;

	return p;
}
