// libui-ng Haiku backend — initialization and the main event loop (mirrors unix/main.c).
// The Haiku app object (be_app) IS the event loop: uiInit constructs it, uiMain runs it,
// uiQuit asks it to stop. Each BWindow runs its own BLooper thread, so per-window events
// (clicks, close requests) are dispatched off be_app->Run() automatically.
#include "uipriv_haiku.h"

uiInitOptions uiprivOptions;

// Our BApplication subclass: handles uiQueueMain jobs posted from any thread.
class uiHaikuApp : public BApplication {
public:
	uiHaikuApp(const char *sig) : BApplication(sig) {}
	virtual void MessageReceived(BMessage *msg)
	{
		if (msg->what == 'qjob') {
			void (*f)(void *) = NULL;
			void *data = NULL;
			if (msg->FindPointer("f", (void **) &f) == B_OK && f != NULL) {
				msg->FindPointer("data", &data);
				(*f)(data);
			}
			return;
		}
		BApplication::MessageReceived(msg);
	}
};

const char *uiInit(uiInitOptions *options)
{
	uiprivOptions = *options;
	uiprivInitAlloc();
	// Application signatures must be unique MIME types; this one is fine for libui-hosted apps.
	new uiHaikuApp("application/x-vnd.libui-ng");
	if (be_app == NULL)
		return "unable to create BApplication";
	return NULL;
}

void uiUninit(void)
{
	uiprivUninitMenus();
	if (be_app != NULL) {
		delete be_app;
		be_app = NULL;
	}
	uiprivUninitAlloc();
}

void uiFreeInitError(const char *err)
{
	// uiInit returns string literals; nothing to free.
	(void) err;
}

void uiMain(void)
{
	be_app->Run();
}

void uiMainSteps(void)
{
	// No-op: Haiku has no incremental main-loop priming step (cf. GTK).
}

int uiMainStep(int wait)
{
	// BApplication has no public single-step; callers wanting a step loop should use uiMain.
	(void) wait;
	return 0;
}

void uiQuit(void)
{
	if (be_app != NULL)
		be_app->PostMessage(B_QUIT_REQUESTED);
}

void uiQueueMain(void (*f)(void *data), void *data)
{
	BMessage msg('qjob');
	msg.AddPointer("f", (void *) f);
	msg.AddPointer("data", data);
	be_app->PostMessage(&msg);
}
