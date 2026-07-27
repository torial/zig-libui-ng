// libui-ng on Haiku — tabs, combobox, and multiline entry. A BTabView with two pages: page one has
// a combobox and a multiline entry; page two has a label. Demonstrates the container + data-entry
// widget cluster.
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
	uiTab *tab;
	uiBox *page1;
	uiCombobox *combo;
	uiMultilineEntry *mle;
	uiBox *page2;

	memset(&o, 0, sizeof o);
	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "error initializing libui: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("libui-ng on Haiku — tabs", 380, 300, 0);
	uiWindowOnClosing(w, onClosing, NULL);
	uiWindowSetMargined(w, 1);

	tab = uiNewTab();

	page1 = uiNewVerticalBox();
	uiBoxSetPadded(page1, 1);
	uiBoxAppend(page1, uiControl(uiNewLabel("Pick a fruit:")), 0);
	combo = uiNewCombobox();
	uiComboboxAppend(combo, "Apple");
	uiComboboxAppend(combo, "Banana");
	uiComboboxAppend(combo, "Cherry");
	uiComboboxSetSelected(combo, 1);
	uiBoxAppend(page1, uiControl(combo), 0);
	uiBoxAppend(page1, uiControl(uiNewLabel("Notes:")), 0);
	mle = uiNewMultilineEntry();
	uiMultilineEntrySetText(mle, "Multi-line text\nin a BTextView\ninside a scroll view.");
	uiBoxAppend(page1, uiControl(mle), 1);

	page2 = uiNewVerticalBox();
	uiBoxSetPadded(page2, 1);
	uiBoxAppend(page2, uiControl(uiNewLabel("This is the second tab page.")), 0);

	uiTabAppend(tab, "Entry", uiControl(page1));
	uiTabAppend(tab, "About", uiControl(page2));
	uiTabSetMargined(tab, 0, 1);
	uiTabSetMargined(tab, 1, 1);

	uiWindowSetChild(w, uiControl(tab));
	uiControlShow(uiControl(w));

	uiMain();
	uiUninit();
	return 0;
}
