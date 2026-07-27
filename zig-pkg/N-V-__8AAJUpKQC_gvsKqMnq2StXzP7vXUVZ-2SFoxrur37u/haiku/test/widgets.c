// libui-ng on Haiku — showcase of the widgets implemented so far. Exercises every control in the
// "easy tier" and wires a couple of live interactions (slider -> progress bar, checkbox -> label).
#include <stdio.h>
#include <string.h>
#include "../../ui.h"

static uiProgressBar *progress;
static uiLabel *status;

static int onClosing(uiWindow *w, void *data)
{
	(void) w; (void) data;
	uiQuit();
	return 1;
}

static void onSliderChanged(uiSlider *s, void *data)
{
	(void) data;
	uiProgressBarSetValue(progress, uiSliderValue(s));
}

static void onToggled(uiCheckbox *c, void *data)
{
	(void) data;
	uiLabelSetText(status, uiCheckboxChecked(c) ? "Checkbox: ON" : "Checkbox: OFF");
}

static void onSelected(uiRadioButtons *r, void *data)
{
	(void) data;
	char buf[64];
	snprintf(buf, sizeof buf, "Radio choice: %d", uiRadioButtonsSelected(r));
	uiLabelSetText(status, buf);
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
	uiBox *box, *row;
	uiButton *btn;
	uiCheckbox *check;
	uiEntry *entry;
	uiSlider *slider;
	uiSpinbox *spin;
	uiGroup *group;
	uiBox *groupBox;
	uiRadioButtons *radio;

	memset(&o, 0, sizeof o);
	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "error initializing libui: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("libui-ng on Haiku — widgets", 360, 420, 0);
	uiWindowOnClosing(w, onClosing, NULL);
	uiWindowSetMargined(w, 1);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);

	uiBoxAppend(box, uiControl(uiNewLabel("A tour of the Haiku backend's controls:")), 0);

	entry = uiNewEntry();
	uiEntrySetText(entry, "editable text");
	uiBoxAppend(box, uiControl(entry), 0);

	check = uiNewCheckbox("Enable the thing");
	uiCheckboxOnToggled(check, onToggled, NULL);
	uiBoxAppend(box, uiControl(check), 0);

	spin = uiNewSpinbox(0, 100);
	uiSpinboxSetValue(spin, 42);
	uiBoxAppend(box, uiControl(spin), 0);

	row = uiNewHorizontalBox();
	uiBoxSetPadded(row, 1);
	uiBoxAppend(row, uiControl(uiNewLabel("Volume")), 0);
	slider = uiNewSlider(0, 100);
	uiSliderOnChanged(slider, onSliderChanged, NULL);
	uiBoxAppend(row, uiControl(slider), 1);
	uiBoxAppend(box, uiControl(row), 0);

	progress = uiNewProgressBar();
	uiBoxAppend(box, uiControl(progress), 0);

	uiBoxAppend(box, uiControl(uiNewHorizontalSeparator()), 0);

	group = uiNewGroup("Pick one");
	uiGroupSetMargined(group, 1);
	groupBox = uiNewVerticalBox();
	uiBoxSetPadded(groupBox, 1);
	radio = uiNewRadioButtons();
	uiRadioButtonsAppend(radio, "First");
	uiRadioButtonsAppend(radio, "Second");
	uiRadioButtonsAppend(radio, "Third");
	uiRadioButtonsOnSelected(radio, onSelected, NULL);
	uiBoxAppend(groupBox, uiControl(radio), 0);
	uiGroupSetChild(group, uiControl(groupBox));
	uiBoxAppend(box, uiControl(group), 0);

	status = uiNewLabel("Status: interact with the controls");
	uiBoxAppend(box, uiControl(status), 0);

	btn = uiNewButton("Click me");
	uiButtonOnClicked(btn, onClicked, NULL);
	uiBoxAppend(box, uiControl(btn), 0);

	uiWindowSetChild(w, uiControl(box));
	uiControlShow(uiControl(w));

	uiMain();
	uiUninit();
	return 0;
}
