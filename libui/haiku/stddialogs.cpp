// libui-ng Haiku backend — standard dialogs (BAlert for message boxes, BFilePanel for file pickers).
//
// libui's file-dialog API is synchronous (returns the chosen path), but BFilePanel is asynchronous —
// it posts B_REFS_RECEIVED / B_SAVE_REQUESTED / B_CANCEL to a target. We bridge the two with a tiny
// dedicated BLooper that captures the result and releases a semaphore, blocking the caller until the
// user is done. The file panel runs on its own looper, so blocking the calling (window) thread is
// safe and gives the expected modal feel.
#include <Alert.h>
#include <FilePanel.h>
#include <Path.h>
#include <Entry.h>
#include <Looper.h>
#include <NodeMonitor.h>
#include <OS.h>
#include "uipriv_haiku.h"

class FilePanelWaiter : public BLooper {
public:
	sem_id sem;
	BString path;
	bool gotResult;

	FilePanelWaiter() : BLooper("ui-filepanel-waiter")
	{
		this->sem = create_sem(0, "ui-filepanel");
		this->gotResult = false;
	}
	~FilePanelWaiter() { delete_sem(this->sem); }

	virtual void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {
		case B_REFS_RECEIVED: {		// open file / folder
			entry_ref ref;
			if (msg->FindRef("refs", &ref) == B_OK) {
				BPath p(&ref);
				this->path = p.Path();
				this->gotResult = true;
			}
			release_sem(this->sem);
			break;
		}
		case B_SAVE_REQUESTED: {	// save file: directory + name
			entry_ref dir;
			const char *name;
			if (msg->FindRef("directory", &dir) == B_OK &&
				msg->FindString("name", &name) == B_OK) {
				BPath p(&dir);
				p.Append(name);
				this->path = p.Path();
				this->gotResult = true;
			}
			release_sem(this->sem);
			break;
		}
		case B_CANCEL:
			release_sem(this->sem);
			break;
		default:
			BLooper::MessageReceived(msg);
		}
	}
};

static char *runFilePanel(file_panel_mode mode, uint32 nodeFlavors)
{
	FilePanelWaiter *waiter = new FilePanelWaiter();
	waiter->Run();				// start the looper thread

	BMessenger target(NULL, waiter);
	BFilePanel *panel = new BFilePanel(mode, &target, NULL, nodeFlavors,
		false /* multiple */, NULL, NULL, true /* modal */, true /* hideWhenDone */);
	panel->Show();

	acquire_sem(waiter->sem);		// block until the user picks or cancels

	char *result = NULL;
	if (waiter->gotResult)
		result = uiHaikuStrdupText(waiter->path.String());

	delete panel;
	waiter->Lock();
	waiter->Quit();				// stops + deletes the looper

	return result;
}

char *uiOpenFile(uiWindow *parent)
{
	(void) parent;
	return runFilePanel(B_OPEN_PANEL, B_FILE_NODE);
}

char *uiOpenFolder(uiWindow *parent)
{
	(void) parent;
	return runFilePanel(B_OPEN_PANEL, B_DIRECTORY_NODE);
}

char *uiSaveFile(uiWindow *parent)
{
	(void) parent;
	return runFilePanel(B_SAVE_PANEL, 0);
}

// The *WithParams variants honor nothing extra yet (default folder / name / filters are ignored).
char *uiOpenFileWithParams(uiWindow *parent, uiFileDialogParams *params)
{
	(void) params;
	return uiOpenFile(parent);
}

char *uiOpenFolderWithParams(uiWindow *parent, uiFileDialogParams *params)
{
	(void) params;
	return uiOpenFolder(parent);
}

char *uiSaveFileWithParams(uiWindow *parent, uiFileDialogParams *params)
{
	(void) params;
	return uiSaveFile(parent);
}

static void runAlert(const char *title, const char *description, alert_type type)
{
	BAlert *alert = new BAlert(title, description, "OK", NULL, NULL, B_WIDTH_AS_USUAL, type);
	alert->Go();		// modal; deletes itself when dismissed
}

void uiMsgBox(uiWindow *parent, const char *title, const char *description)
{
	(void) parent;
	runAlert(title, description, B_INFO_ALERT);
}

void uiMsgBoxError(uiWindow *parent, const char *title, const char *description)
{
	(void) parent;
	runAlert(title, description, B_STOP_ALERT);
}
