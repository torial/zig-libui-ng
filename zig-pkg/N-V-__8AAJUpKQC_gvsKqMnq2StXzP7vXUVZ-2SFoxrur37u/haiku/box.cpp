// libui-ng Haiku backend — uiBox (BGroupView + BGroupLayout).
// BGroupView::AddChild routes into its group layout, so the macro SetContainer (parent->AddChild)
// lays children out automatically. We track children in a BList so Destroy/Delete can detach them
// without double-freeing (deleting a BView also deletes its BView children — children's own
// uiControls must be detached first).
#include "uipriv_haiku.h"

struct uiBox {
	uiHaikuControl c;
	BGroupView *view;
	BList *children;	// uiControl* — owned references, in layout order
	int vertical;
	int padded;
};

// Custom Destroy (detaches + destroys children first); the rest of the vtable is the default.
static void uiBoxDestroy(uiControl *c)
{
	uiBox *b = (uiBox *) c;
	while (b->children->CountItems() > 0) {
		uiControl *child = (uiControl *) b->children->ItemAt(0);
		uiHaikuControlSetContainer(uiHaikuControl(child), b->view, 1);
		uiControlSetParent(child, NULL);
		uiControlDestroy(child);
		b->children->RemoveItem((int32) 0);
	}
	delete b->children;
	if (b->view->Parent() != NULL)
		b->view->RemoveSelf();
	delete b->view;
	uiFreeControl(c);
}

uiHaikuControlAllDefaultsExceptDestroy(uiBox)

void uiBoxAppend(uiBox *b, uiControl *child, int stretchy)
{
	BWindow *win = b->view->Window();
	if (win != NULL) win->Lock();
	uiControlSetParent(child, uiControl(b));
	uiHaikuControlSetContainer(uiHaikuControl(child), b->view, 0);
	b->children->AddItem(child);
	// stretchy controls keep the default layout weight (1); non-stretchy ones are pinned to their
	// preferred size so they don't grow. (A finer per-axis policy can come later.)
	if (!stretchy) {
		int32 i = b->children->CountItems() - 1;
		BLayoutItem *it = b->view->GroupLayout()->ItemAt(i);
		if (it != NULL)
			it->SetExplicitMaxSize(it->PreferredSize());
	}
	if (win != NULL) win->Unlock();
}

void uiBoxDelete(uiBox *b, int index)
{
	uiControl *child = (uiControl *) b->children->ItemAt(index);
	if (child == NULL)
		return;
	BWindow *win = b->view->Window();
	if (win != NULL) win->Lock();
	uiHaikuControlSetContainer(uiHaikuControl(child), b->view, 1);
	uiControlSetParent(child, NULL);
	b->children->RemoveItem(index);
	if (win != NULL) win->Unlock();
}

int uiBoxPadded(uiBox *b) { return b->padded; }

void uiBoxSetPadded(uiBox *b, int padded)
{
	b->padded = padded;
	BWindow *win = b->view->Window();
	if (win != NULL) win->Lock();
	b->view->GroupLayout()->SetSpacing(padded ? B_USE_DEFAULT_SPACING : 0);
	if (win != NULL) win->Unlock();
}

static uiBox *finishNewBox(int vertical)
{
	uiBox *b;

	uiHaikuNewControl(uiBox, b);
	uiControl(b)->Destroy = uiBoxDestroy;	// override the macro default with the detaching one

	b->vertical = vertical;
	b->view = new BGroupView(vertical ? B_VERTICAL : B_HORIZONTAL, 0);
	b->children = new BList();
	b->padded = 0;

	return b;
}

uiBox *uiNewHorizontalBox(void) { return finishNewBox(0); }
uiBox *uiNewVerticalBox(void) { return finishNewBox(1); }
