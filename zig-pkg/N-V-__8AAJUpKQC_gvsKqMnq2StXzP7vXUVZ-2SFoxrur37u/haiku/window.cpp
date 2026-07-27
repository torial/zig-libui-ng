// libui-ng Haiku backend — uiWindow (BWindow).
// A window is toplevel and is NOT a BView, so it gets a hand-written vtable rather than the
// uiHaikuControl* macros. The window owns a BGroupView "content" area (a real BView) so that
// uiWindowSetChild can reuse the same AddChild/SetContainer path the boxes use.
#include "uipriv_haiku.h"

struct uiWindow {
	uiHaikuControl c;
	BWindow *window;
	BGroupView *content;
	uiControl *child;
	int margined;
	int (*onClosing)(uiWindow *, void *);
	void *onClosingData;
};

#define toWindow(c) ((uiWindow *) (c))

// BWindow subclass: routes the close request to onClosing and button invocations to their
// uiButton callbacks (a BControl with no explicit target sends to its window's looper).
class uiHaikuWindowImpl : public BWindow {
	uiWindow *w;
public:
	uiHaikuWindowImpl(uiWindow *uw, BRect frame, const char *title)
		: BWindow(frame, title, B_TITLED_WINDOW,
			B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
	{
		this->w = uw;
	}
	virtual bool QuitRequested()
	{
		if (w->onClosing != NULL)
			return (*(w->onClosing))(w, w->onClosingData) ? true : false;
		return true;
	}
	virtual void MessageReceived(BMessage *msg)
	{
		if (msg->what == uiprivMsgControlEvent) {
			uiprivEventFn fn = NULL;
			void *control = NULL;
			if (msg->FindPointer("fn", (void **) &fn) == B_OK && fn != NULL) {
				msg->FindPointer("control", &control);
				(*fn)(control);
			}
			return;
		}
		if (msg->what == uiprivMsgMenuItem) {
			uiMenuItem *item = NULL;
			if (msg->FindPointer("item", (void **) &item) == B_OK && item != NULL)
				uiprivMenuItemClicked(item, w);
			return;
		}
		BWindow::MessageReceived(msg);
	}
};

static void uiWindowDestroy(uiControl *c)
{
	uiWindow *w = toWindow(c);
	if (w->child != NULL) {
		uiControlSetParent(w->child, NULL);
		uiControlDestroy(w->child);
	}
	if (w->window->Lock())
		w->window->Quit();		// quits the looper and deletes the BWindow
	uiFreeControl(c);
}

static uintptr_t uiWindowHandle(uiControl *c) { return (uintptr_t) (toWindow(c)->window); }
static uiControl *uiWindowParent(uiControl *c) { (void) c; return NULL; }
static void uiWindowSetParent(uiControl *c, uiControl *parent)
{
	(void) parent;
	uiprivUserBug("You cannot give a uiWindow a parent. (window: %p)", c);
}
static int uiWindowToplevel(uiControl *c) { (void) c; return 1; }
static int uiWindowVisible(uiControl *c) { return toWindow(c)->window->IsHidden() ? 0 : 1; }
static void uiWindowShow(uiControl *c)
{
	BWindow *win = toWindow(c)->window;
	if (win->Lock()) {
		if (win->IsHidden())
			win->Show();
		win->Unlock();
	}
}
static void uiWindowHide(uiControl *c)
{
	BWindow *win = toWindow(c)->window;
	if (win->Lock()) {
		win->Hide();
		win->Unlock();
	}
}
static int uiWindowEnabled(uiControl *c) { (void) c; return 1; }
static void uiWindowEnable(uiControl *c) { (void) c; }
static void uiWindowDisable(uiControl *c) { (void) c; }
static void uiWindowSetContainer(uiHaikuControl *c, BView *parent, int remove)
{
	(void) parent; (void) remove;
	uiprivImplBug("attempt to place uiWindow %p in a container", c);
}

char *uiWindowTitle(uiWindow *w)
{
	return uiHaikuStrdupText(w->window->Title());
}

void uiWindowSetTitle(uiWindow *w, const char *title)
{
	if (w->window->Lock()) {
		w->window->SetTitle(title);
		w->window->Unlock();
	}
}

void uiWindowContentSize(uiWindow *w, int *width, int *height)
{
	BRect b = w->window->Bounds();
	*width = (int) b.Width();
	*height = (int) b.Height();
}

void uiWindowOnClosing(uiWindow *w, int (*f)(uiWindow *, void *), void *data)
{
	w->onClosing = f;
	w->onClosingData = data;
}

void uiWindowSetChild(uiWindow *w, uiControl *child)
{
	bool locked = w->window->Lock();
	if (w->child != NULL) {
		uiHaikuControlSetContainer(uiHaikuControl(w->child), w->content, 1);
		uiControlSetParent(w->child, NULL);
	}
	w->child = child;
	if (w->child != NULL) {
		uiControlSetParent(w->child, uiControl(w));
		uiHaikuControlSetContainer(uiHaikuControl(w->child), w->content, 0);
	}
	if (locked)
		w->window->Unlock();
}

int uiWindowMargined(uiWindow *w) { return w->margined; }

void uiWindowSetMargined(uiWindow *w, int margined)
{
	w->margined = margined;
	if (w->window->Lock()) {
		float m = margined ? B_USE_WINDOW_SPACING : 0;
		w->content->GroupLayout()->SetInsets(m, m, m, m);
		w->window->Unlock();
	}
}

uiWindow *uiNewWindow(const char *title, int width, int height, int hasMenubar)
{
	uiWindow *w;

	w = toWindow(uiHaikuAllocControl(sizeof (uiWindow), uiWindowSignature, "uiWindow"));

	uiControl(w)->Destroy = uiWindowDestroy;
	uiControl(w)->Handle = uiWindowHandle;
	uiControl(w)->Parent = uiWindowParent;
	uiControl(w)->SetParent = uiWindowSetParent;
	uiControl(w)->Toplevel = uiWindowToplevel;
	uiControl(w)->Visible = uiWindowVisible;
	uiControl(w)->Show = uiWindowShow;
	uiControl(w)->Hide = uiWindowHide;
	uiControl(w)->Enabled = uiWindowEnabled;
	uiControl(w)->Enable = uiWindowEnable;
	uiControl(w)->Disable = uiWindowDisable;
	uiHaikuControl(w)->SetContainer = uiWindowSetContainer;

	BRect frame(100, 100, 100 + width, 100 + height);
	w->window = new uiHaikuWindowImpl(w, frame, title);
	w->content = new BGroupView(B_VERTICAL);
	w->window->SetLayout(new BGroupLayout(B_VERTICAL, 0));
	// The menubar (if any) sits flush at the top, above the content area.
	if (hasMenubar)
		w->window->AddChild(uiprivMakeMenubar(w));
	w->window->AddChild(w->content);

	return w;
}
