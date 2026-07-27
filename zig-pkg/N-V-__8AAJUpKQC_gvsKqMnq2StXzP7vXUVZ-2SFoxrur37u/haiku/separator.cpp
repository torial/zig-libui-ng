// libui-ng Haiku backend — uiSeparator (BSeparatorView).
#include "uipriv_haiku.h"

struct uiSeparator {
	uiHaikuControl c;
	BSeparatorView *view;
};

uiHaikuControlAllDefaults(uiSeparator)

static uiSeparator *finishNewSeparator(orientation o)
{
	uiSeparator *s;

	uiHaikuNewControl(uiSeparator, s);
	s->view = new BSeparatorView(o);

	return s;
}

uiSeparator *uiNewHorizontalSeparator(void) { return finishNewSeparator(B_HORIZONTAL); }
uiSeparator *uiNewVerticalSeparator(void) { return finishNewSeparator(B_VERTICAL); }
