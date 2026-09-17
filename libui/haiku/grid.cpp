// libui-ng Haiku backend — uiGrid (BGridView with explicit cell placement + spans + alignment).
#include "uipriv_haiku.h"

struct gridChild {
	uiControl *child;
	int left, top;
};

struct uiGrid {
	uiHaikuControl c;
	BGridView *view;
	BList *children;	// gridChild*
	int padded;
};

static alignment haAlign(uiAlign a)
{
	switch (a) {
	case uiAlignFill:   return B_ALIGN_USE_FULL_WIDTH;
	case uiAlignStart:  return B_ALIGN_LEFT;
	case uiAlignCenter: return B_ALIGN_HORIZONTAL_CENTER;
	case uiAlignEnd:    return B_ALIGN_RIGHT;
	}
	return B_ALIGN_LEFT;
}

static vertical_alignment vaAlign(uiAlign a)
{
	switch (a) {
	case uiAlignFill:   return B_ALIGN_USE_FULL_HEIGHT;
	case uiAlignStart:  return B_ALIGN_TOP;
	case uiAlignCenter: return B_ALIGN_VERTICAL_CENTER;
	case uiAlignEnd:    return B_ALIGN_BOTTOM;
	}
	return B_ALIGN_TOP;
}

static void uiGridDestroy(uiControl *cc)
{
	uiGrid *g = (uiGrid *) cc;
	for (int32 i = 0; i < g->children->CountItems(); i++) {
		gridChild *gc = (gridChild *) g->children->ItemAt(i);
		BView *cv = (BView *) uiControlHandle(gc->child);
		if (cv->Parent() != NULL) cv->RemoveSelf();
		uiControlSetParent(gc->child, NULL);
		uiControlDestroy(gc->child);
		delete gc;
	}
	delete g->children;
	if (g->view->Parent() != NULL)
		g->view->RemoveSelf();
	delete g->view;
	uiFreeControl(cc);
}

uiHaikuControlAllDefaultsExceptDestroy(uiGrid)

static void place(uiGrid *g, uiControl *c, int left, int top, int xspan, int yspan,
	int hexpand, uiAlign halign, int vexpand, uiAlign valign)
{
	BWindow *win = g->view->Window();
	if (win != NULL) win->Lock();

	BGridLayout *gl = g->view->GridLayout();
	BView *cv = (BView *) uiControlHandle(c);
	uiControlSetParent(c, uiControl(g));
	BLayoutItem *it = gl->AddView(cv, left, top, xspan, yspan);
	if (it != NULL)
		it->SetExplicitAlignment(BAlignment(haAlign(halign), vaAlign(valign)));
	if (hexpand)
		gl->SetColumnWeight(left, 1.0f);
	if (vexpand)
		gl->SetRowWeight(top, 1.0f);

	gridChild *gc = new gridChild();
	gc->child = c; gc->left = left; gc->top = top;
	g->children->AddItem(gc);

	if (win != NULL) win->Unlock();
}

void uiGridAppend(uiGrid *g, uiControl *c, int left, int top, int xspan, int yspan,
	int hexpand, uiAlign halign, int vexpand, uiAlign valign)
{
	place(g, c, left, top, xspan, yspan, hexpand, halign, vexpand, valign);
}

void uiGridInsertAt(uiGrid *g, uiControl *c, uiControl *existing, uiAt at, int xspan, int yspan,
	int hexpand, uiAlign halign, int vexpand, uiAlign valign)
{
	// Place relative to `existing` by computing the adjacent cell. This does not shift other cells
	// (BGridLayout has no row/column insertion); callers should leave the target cell free.
	int left = 0, top = 0;
	for (int32 i = 0; i < g->children->CountItems(); i++) {
		gridChild *gc = (gridChild *) g->children->ItemAt(i);
		if (gc->child == existing) { left = gc->left; top = gc->top; break; }
	}
	switch (at) {
	case uiAtLeading:  left -= 1; break;
	case uiAtTrailing: left += 1; break;
	case uiAtTop:      top -= 1; break;
	case uiAtBottom:   top += 1; break;
	}
	if (left < 0) left = 0;
	if (top < 0) top = 0;
	place(g, c, left, top, xspan, yspan, hexpand, halign, vexpand, valign);
}

int uiGridPadded(uiGrid *g) { return g->padded; }

void uiGridSetPadded(uiGrid *g, int padded)
{
	g->padded = padded;
	BWindow *win = g->view->Window();
	if (win != NULL) win->Lock();
	float s = padded ? B_USE_DEFAULT_SPACING : 0;
	g->view->GridLayout()->SetSpacing(s, s);
	if (win != NULL) win->Unlock();
}

uiGrid *uiNewGrid(void)
{
	uiGrid *g;

	uiHaikuNewControl(uiGrid, g);
	uiControl(g)->Destroy = uiGridDestroy;
	g->view = new BGridView();
	g->view->GridLayout()->SetSpacing(0, 0);
	g->children = new BList();
	g->padded = 0;

	return g;
}
