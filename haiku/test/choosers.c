// libui-ng on Haiku — the chooser controls: color button, font button, date picker, time picker,
// and a date+time picker. Laid out as labeled form rows.
#include <stdio.h>
#include <string.h>
#include "../../ui.h"

static int onClosing(uiWindow *w, void *data)
{
	(void) w; (void) data;
	uiQuit();
	return 1;
}

int main(void)
{
	uiInitOptions o;
	const char *err;
	uiWindow *w;
	uiForm *form;

	memset(&o, 0, sizeof o);
	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "error initializing libui: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("libui-ng on Haiku — choosers", 420, 220, 0);
	uiWindowOnClosing(w, onClosing, NULL);
	uiWindowSetMargined(w, 1);

	form = uiNewForm();
	uiFormSetPadded(form, 1);
	uiFormAppend(form, "Color", uiControl(uiNewColorButton()), 0);
	uiFormAppend(form, "Font", uiControl(uiNewFontButton()), 0);
	uiFormAppend(form, "Date", uiControl(uiNewDatePicker()), 0);
	uiFormAppend(form, "Time", uiControl(uiNewTimePicker()), 0);
	uiFormAppend(form, "Date + time", uiControl(uiNewDateTimePicker()), 0);

	uiWindowSetChild(w, uiControl(form));
	uiControlShow(uiControl(w));

	uiMain();
	uiUninit();
	return 0;
}
