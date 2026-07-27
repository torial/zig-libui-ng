// libui-ng on Haiku — uiForm (labeled rows) + uiEditableCombobox + uiGrid. The form holds an entry,
// an editable combobox, and a spinbox; below it a grid arranges two buttons.
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
	uiBox *box;
	uiForm *form;
	uiEditableCombobox *ecombo;
	uiGrid *grid;

	memset(&o, 0, sizeof o);
	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "error initializing libui: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("libui-ng on Haiku — forms & grid", 380, 240, 0);
	uiWindowOnClosing(w, onClosing, NULL);
	uiWindowSetMargined(w, 1);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);

	form = uiNewForm();
	uiFormSetPadded(form, 1);
	uiFormAppend(form, "Name", uiControl(uiNewEntry()), 0);
	ecombo = uiNewEditableCombobox();
	uiEditableComboboxAppend(ecombo, "Red");
	uiEditableComboboxAppend(ecombo, "Green");
	uiEditableComboboxAppend(ecombo, "Blue");
	uiEditableComboboxSetText(ecombo, "Green");
	uiFormAppend(form, "Color", uiControl(ecombo), 0);
	uiFormAppend(form, "Count", uiControl(uiNewSpinbox(0, 10)), 0);
	uiBoxAppend(box, uiControl(form), 0);

	uiBoxAppend(box, uiControl(uiNewHorizontalSeparator()), 0);

	grid = uiNewGrid();
	uiGridSetPadded(grid, 1);
	uiGridAppend(grid, uiControl(uiNewButton("OK")),     0, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(grid, uiControl(uiNewButton("Cancel")), 1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
	uiBoxAppend(box, uiControl(grid), 0);

	uiWindowSetChild(w, uiControl(box));
	uiControlShow(uiControl(w));

	uiMain();
	uiUninit();
	return 0;
}
