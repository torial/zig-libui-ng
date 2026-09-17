// libui-ng Haiku backend — uiArea (a custom-drawn BView).
// The view's Draw() builds a uiDrawContext over itself and calls the handler; mouse/key events are
// translated into uiArea*Event structs. Scrolling areas wrap the canvas in a BScrollView.
#include "uipriv_haiku.h"

struct uiArea;

class UiAreaView : public BView {
public:
	uiArea *area;
	uint32 prevButtons;
	UiAreaView()
		: BView("uiArea", B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE)
	{
		this->area = NULL;
		this->prevButtons = 0;
	}
	virtual void Draw(BRect update);
	virtual void MouseDown(BPoint where);
	virtual void MouseUp(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 transit, const BMessage *drag);
	virtual void KeyDown(const char *bytes, int32 numBytes);
	virtual void KeyUp(const char *bytes, int32 numBytes);
private:
	void doMouse(BPoint where, int down, int up);
	void doKey(const char *bytes, int32 numBytes, int up);
};

struct uiArea {
	uiHaikuControl c;
	BView *view;		// outer widget (macros/layout): the canvas, or a BScrollView wrapping it
	UiAreaView *canvas;
	uiAreaHandler *ah;
	int scrolling;
	int width, height;
};

uiHaikuControlAllDefaults(uiArea)

static uiModifiers modsFrom(uint32 m)
{
	int r = 0;
	if (m & B_CONTROL_KEY) r |= uiModifierCtrl;
	if (m & B_OPTION_KEY)  r |= uiModifierAlt;
	if (m & B_SHIFT_KEY)   r |= uiModifierShift;
	if (m & B_COMMAND_KEY) r |= uiModifierSuper;
	return (uiModifiers) r;
}

static int buttonNum(uint32 mask)
{
	if (mask & B_PRIMARY_MOUSE_BUTTON)   return 1;
	if (mask & B_SECONDARY_MOUSE_BUTTON) return 2;
	if (mask & B_TERTIARY_MOUSE_BUTTON)  return 3;
	return 0;
}

void UiAreaView::Draw(BRect update)
{
	uiDrawContext ctx;
	ctx.view = this;
	uiAreaDrawParams p;
	p.Context = &ctx;
	p.AreaWidth = this->area->scrolling ? this->area->width : this->Bounds().Width();
	p.AreaHeight = this->area->scrolling ? this->area->height : this->Bounds().Height();
	p.ClipX = update.left;
	p.ClipY = update.top;
	p.ClipWidth = update.Width();
	p.ClipHeight = update.Height();
	this->area->ah->Draw(this->area->ah, this->area, &p);
}

void UiAreaView::doMouse(BPoint where, int down, int up)
{
	uiArea *a = this->area;
	uiAreaMouseEvent e;
	e.X = where.x;
	e.Y = where.y;
	e.AreaWidth = a->scrolling ? a->width : this->Bounds().Width();
	e.AreaHeight = a->scrolling ? a->height : this->Bounds().Height();
	e.Down = down;
	e.Up = up;

	int32 clicks = 1;
	uint32 buttons = 0;
	BMessage *m = this->Window() ? this->Window()->CurrentMessage() : NULL;
	if (m != NULL) {
		m->FindInt32("clicks", &clicks);
		m->FindInt32("buttons", (int32 *) &buttons);
	}
	e.Count = (int) clicks;
	e.Modifiers = modsFrom(modifiers());

	uint64_t held = 0;
	if (buttons & B_PRIMARY_MOUSE_BUTTON)   held |= (uint64_t) 1 << 0;
	if (buttons & B_SECONDARY_MOUSE_BUTTON) held |= (uint64_t) 1 << 1;
	if (buttons & B_TERTIARY_MOUSE_BUTTON)  held |= (uint64_t) 1 << 2;
	e.Held1To64 = held;

	a->ah->MouseEvent(a->ah, a, &e);
}

void UiAreaView::MouseDown(BPoint where)
{
	uint32 buttons = 0;
	BMessage *m = this->Window() ? this->Window()->CurrentMessage() : NULL;
	if (m != NULL) m->FindInt32("buttons", (int32 *) &buttons);
	int down = buttonNum(buttons & ~this->prevButtons);
	this->prevButtons = buttons;
	SetMouseEventMask(B_POINTER_EVENTS);	// keep getting moves/up during a drag
	doMouse(where, down, 0);
}

void UiAreaView::MouseUp(BPoint where)
{
	uint32 buttons = 0;
	BMessage *m = this->Window() ? this->Window()->CurrentMessage() : NULL;
	if (m != NULL) m->FindInt32("buttons", (int32 *) &buttons);
	int up = buttonNum(this->prevButtons & ~buttons);
	this->prevButtons = buttons;
	doMouse(where, 0, up);
}

void UiAreaView::MouseMoved(BPoint where, uint32 transit, const BMessage *drag)
{
	(void) drag;
	if (transit == B_ENTERED_VIEW)
		this->area->ah->MouseCrossed(this->area->ah, this->area, 0);
	else if (transit == B_EXITED_VIEW)
		this->area->ah->MouseCrossed(this->area->ah, this->area, 1);
	doMouse(where, 0, 0);
}

void UiAreaView::doKey(const char *bytes, int32 numBytes, int up)
{
	uiArea *a = this->area;
	uiAreaKeyEvent e;
	e.Key = 0;
	e.ExtKey = (uiExtKey) 0;
	e.Modifier = (uiModifiers) 0;
	e.Modifiers = modsFrom(modifiers());
	e.Up = up;

	if (numBytes == 1) {
		switch (bytes[0]) {
		case B_ESCAPE:      e.ExtKey = uiExtKeyEscape; break;
		case B_INSERT:      e.ExtKey = uiExtKeyInsert; break;
		case B_DELETE:      e.ExtKey = uiExtKeyDelete; break;
		case B_HOME:        e.ExtKey = uiExtKeyHome; break;
		case B_END:         e.ExtKey = uiExtKeyEnd; break;
		case B_PAGE_UP:     e.ExtKey = uiExtKeyPageUp; break;
		case B_PAGE_DOWN:   e.ExtKey = uiExtKeyPageDown; break;
		case B_UP_ARROW:    e.ExtKey = uiExtKeyUp; break;
		case B_DOWN_ARROW:  e.ExtKey = uiExtKeyDown; break;
		case B_LEFT_ARROW:  e.ExtKey = uiExtKeyLeft; break;
		case B_RIGHT_ARROW: e.ExtKey = uiExtKeyRight; break;
		default:            e.Key = bytes[0]; break;
		}
	}

	int handled = a->ah->KeyEvent(a->ah, a, &e);
	if (!handled) {
		if (up)
			BView::KeyUp(bytes, numBytes);
		else
			BView::KeyDown(bytes, numBytes);
	}
}

void UiAreaView::KeyDown(const char *bytes, int32 numBytes) { doKey(bytes, numBytes, 0); }
void UiAreaView::KeyUp(const char *bytes, int32 numBytes) { doKey(bytes, numBytes, 1); }

static uiArea *finishNewArea(uiAreaHandler *ah, int scrolling, int width, int height)
{
	uiArea *a;

	uiHaikuNewControl(uiArea, a);
	a->ah = ah;
	a->scrolling = scrolling;
	a->width = width;
	a->height = height;

	a->canvas = new UiAreaView();
	a->canvas->area = a;

	if (scrolling) {
		a->canvas->SetExplicitMinSize(BSize(width, height));
		a->view = new BScrollView("uiAreaScroll", a->canvas, 0, true, true);
	} else
		a->view = a->canvas;

	return a;
}

uiArea *uiNewArea(uiAreaHandler *ah)
{
	return finishNewArea(ah, 0, 0, 0);
}

uiArea *uiNewScrollingArea(uiAreaHandler *ah, int width, int height)
{
	return finishNewArea(ah, 1, width, height);
}

void uiAreaSetSize(uiArea *a, int width, int height)
{
	a->width = width;
	a->height = height;
	BWindow *win = a->canvas->Window();
	if (win != NULL) win->Lock();
	a->canvas->SetExplicitMinSize(BSize(width, height));
	a->canvas->Invalidate();
	if (win != NULL) win->Unlock();
}

void uiAreaQueueRedrawAll(uiArea *a)
{
	BWindow *win = a->canvas->Window();
	if (win != NULL) win->Lock();
	a->canvas->Invalidate();
	if (win != NULL) win->Unlock();
}

void uiAreaScrollTo(uiArea *a, double x, double y, double width, double height)
{
	(void) width; (void) height;
	BWindow *win = a->canvas->Window();
	if (win != NULL) win->Lock();
	a->canvas->ScrollTo(BPoint(x, y));
	if (win != NULL) win->Unlock();
}

// Window move/resize-by-drag from inside an area: not wired up yet.
void uiAreaBeginUserWindowMove(uiArea *a) { (void) a; }
void uiAreaBeginUserWindowResize(uiArea *a, uiWindowResizeEdge edge) { (void) a; (void) edge; }
