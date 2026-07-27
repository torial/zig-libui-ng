// libui-ng on Haiku — menus and standard dialogs. Builds a menubar (File with Open/check/Quit, Help
// with About) and buttons that open message boxes and file pickers. A queued welcome uiMsgBox pops
// once the loop starts (so a screenshot shows a live BAlert over the menubar'd window).
#include <stdio.h>
#include <string.h>
#include "../../ui.h"

static uiWindow *mainwin;

static int onClosing(uiWindow *w, void *data)
{
	(void) w; (void) data;
	uiQuit();
	return 1;
}

static int onShouldQuit(void *data)
{
	(void) data;
	return 1;	// let the Quit menu item actually quit
}

static void onMenuOpen(uiMenuItem *item, uiWindow *w, void *data)
{
	(void) item; (void) data;
	char *path = uiOpenFile(w);
	if (path != NULL) {
		uiMsgBox(w, "You opened", path);
		uiFreeText(path);
	} else
		uiMsgBox(w, "Open", "(no file selected)");
}

static void onMsg(uiButton *b, void *data)
{
	(void) b; (void) data;
	uiMsgBox(mainwin, "Message", "This is a uiMsgBox, backed by BAlert on Haiku.");
}

static void onErr(uiButton *b, void *data)
{
	(void) b; (void) data;
	uiMsgBoxError(mainwin, "Something went wrong", "This is a uiMsgBoxError (B_STOP_ALERT).");
}

static void onSave(uiButton *b, void *data)
{
	(void) b; (void) data;
	char *path = uiSaveFile(mainwin);
	if (path != NULL) {
		uiMsgBox(mainwin, "Would save to", path);
		uiFreeText(path);
	}
}

static void welcome(void *data)
{
	(void) data;
	uiMsgBox(mainwin, "Welcome",
		"Menus and dialogs are live.\nTry the File menu and the buttons below.");
}

int main(void)
{
	uiInitOptions o;
	const char *err;
	uiMenu *file, *help;
	uiMenuItem *open;
	uiBox *box;

	memset(&o, 0, sizeof o);
	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "error initializing libui: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	// Menus must be defined before the first window (they finalize when it is created).
	file = uiNewMenu("File");
	open = uiMenuAppendItem(file, "Open...");
	uiMenuItemOnClicked(open, onMenuOpen, NULL);
	uiMenuAppendCheckItem(file, "Toggle me");
	uiMenuAppendQuitItem(file);
	help = uiNewMenu("Help");
	uiMenuAppendAboutItem(help);

	uiOnShouldQuit(onShouldQuit, NULL);

	mainwin = uiNewWindow("libui-ng on Haiku — menus & dialogs", 380, 160, 1 /* hasMenubar */);
	uiWindowOnClosing(mainwin, onClosing, NULL);
	uiWindowSetMargined(mainwin, 1);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiBoxAppend(box, uiControl(uiNewLabel("Buttons below open standard dialogs:")), 0);

	uiButton *bm = uiNewButton("Message box");
	uiButtonOnClicked(bm, onMsg, NULL);
	uiBoxAppend(box, uiControl(bm), 0);

	uiButton *be = uiNewButton("Error box");
	uiButtonOnClicked(be, onErr, NULL);
	uiBoxAppend(box, uiControl(be), 0);

	uiButton *bs = uiNewButton("Save file...");
	uiButtonOnClicked(bs, onSave, NULL);
	uiBoxAppend(box, uiControl(bs), 0);

	uiWindowSetChild(mainwin, uiControl(box));
	uiControlShow(uiControl(mainwin));

	uiQueueMain(welcome, NULL);	// pops once the loop is running
	uiMain();
	uiUninit();
	return 0;
}
