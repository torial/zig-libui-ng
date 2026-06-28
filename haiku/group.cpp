// libui-ng Haiku backend — uiGroup (BBox with a label + single child).
// BBox draws the labelled border; a BGroupLayout set on it hosts the one child (BView::AddChild
// routes to the layout when one is present), mirroring how uiWindow manages its content.
#include "uipriv_haiku.h"

struct uiGroup {
	uiHaikuControl c;
	BBox *view;
	BGroupLayout *layout;
	uiControl *child;
	int margined;
};

static void uiGroupDestroy(uiControl *cc)
{
	uiGroup *g = (uiGroup *) cc;
	if (g->child != NULL) {
		uiHaikuControlSetContainer(uiHaikuControl(g->child), g->view, 1);
		uiControlSetParent(g->child, NULL);
		uiControlDestroy(g->child);
	}
	if (g->view->Parent() != NULL)
		g->view->RemoveSelf();
	delete g->view;
	uiFreeControl(cc);
}

uiHaikuControlAllDefaultsExceptDestroy(uiGroup)

char *uiGroupTitle(uiGroup *g)
{
	return uiHaikuStrdupText(g->view->Label());
}

void uiGroupSetTitle(uiGroup *g, const char *title)
{
	BWindow *win = g->view->Window();
	if (win != NULL) win->Lock();
	g->view->SetLabel(title);
	if (win != NULL) win->Unlock();
}

void uiGroupSetChild(uiGroup *g, uiControl *c)
{
	BWindow *win = g->view->Window();
	bool locked = (win != NULL) && win->Lock();
	if (g->child != NULL) {
		uiHaikuControlSetContainer(uiHaikuControl(g->child), g->view, 1);
		uiControlSetParent(g->child, NULL);
	}
	g->child = c;
	if (g->child != NULL) {
		uiControlSetParent(g->child, uiControl(g));
		uiHaikuControlSetContainer(uiHaikuControl(g->child), g->view, 0);
	}
	if (locked) win->Unlock();
}

int uiGroupMargined(uiGroup *g) { return g->margined; }

void uiGroupSetMargined(uiGroup *g, int margined)
{
	g->margined = margined;
	BWindow *win = g->view->Window();
	if (win != NULL) win->Lock();
	float m = margined ? B_USE_DEFAULT_SPACING : 0;
	g->layout->SetInsets(m, m, m, m);
	if (win != NULL) win->Unlock();
}

uiGroup *uiNewGroup(const char *title)
{
	uiGroup *g;

	uiHaikuNewControl(uiGroup, g);
	uiControl(g)->Destroy = uiGroupDestroy;

	g->view = new BBox("uiGroup");
	g->view->SetLabel(title);
	g->layout = new BGroupLayout(B_VERTICAL);
	g->view->SetLayout(g->layout);
	g->child = NULL;
	g->margined = 0;

	return g;
}
