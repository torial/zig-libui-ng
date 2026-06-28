// Minimal libui-ng program exercising the Haiku backend: a window with a vertical box holding a
// label and a button. Clicking the button changes its text; closing the window quits.
#include <stdio.h>
#include <string.h>
#include "../../ui.h"

static int onClosing(uiWindow *w, void *data)
{
	(void) w; (void) data;
	uiQuit();
	return 1;
}

static void onClicked(uiButton *b, void *data)
{
	(void) data;
	uiButtonSetText(b, "Clicked!");
}

int main(void)
{
	uiInitOptions o;
	const char *err;
	uiWindow *w;
	uiBox *box;
	uiButton *btn;

	memset(&o, 0, sizeof o);
	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "error initializing libui: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("libui-ng on Haiku", 320, 160, 0);
	uiWindowOnClosing(w, onClosing, NULL);
	uiWindowSetMargined(w, 1);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiBoxAppend(box, uiControl(uiNewLabel("Hello from libui-ng on Haiku!")), 0);

	btn = uiNewButton("Click me");
	uiButtonOnClicked(btn, onClicked, NULL);
	uiBoxAppend(box, uiControl(btn), 0);

	uiWindowSetChild(w, uiControl(box));
	uiControlShow(uiControl(w));

	uiMain();
	uiUninit();
	return 0;
}
