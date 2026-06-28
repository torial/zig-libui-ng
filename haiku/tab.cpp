// libui-ng Haiku backend — uiTab (BTabView).
// Each page's content is wrapped in a BGroupView so a page can be independently margined. BTabView
// has no public "insert at index", so uiTabInsertAt removes the trailing tabs (preserving their
// BTab+view), appends the new one, then re-adds the saved tabs. OnSelected fires from a Select()
// override (guarded so programmatic uiTabSetSelected doesn't notify).
#include "uipriv_haiku.h"

struct uiTab;

class UiTabView : public BTabView {
public:
	uiTab *owner;
	bool suppress;
	UiTabView() : BTabView("uiTab", B_WIDTH_AS_USUAL) { this->owner = NULL; this->suppress = false; }
	void notifySelected();
	virtual void Select(int32 index)
	{
		BTabView::Select(index);
		if (!this->suppress) notifySelected();
	}
};

struct tabPage {
	uiControl *child;
	BGroupView *container;	// holds the child; its layout gives per-page margins
	int margined;
};

struct uiTab {
	uiHaikuControl c;
	UiTabView *view;
	BList *pages;		// tabPage*
	void (*onSelected)(uiTab *, void *);
	void *onSelectedData;
};

void UiTabView::notifySelected()
{
	if (this->owner != NULL)
		(*(this->owner->onSelected))(this->owner, this->owner->onSelectedData);
}

static void defaultOnSelected(uiTab *t, void *data) { (void) t; (void) data; }

static void uiTabDestroy(uiControl *cc)
{
	uiTab *t = (uiTab *) cc;
	for (int32 i = 0; i < t->pages->CountItems(); i++) {
		tabPage *pg = (tabPage *) t->pages->ItemAt(i);
		uiHaikuControlSetContainer(uiHaikuControl(pg->child), pg->container, 1);
		uiControlSetParent(pg->child, NULL);
		uiControlDestroy(pg->child);
		delete pg;
	}
	delete t->pages;
	if (t->view->Parent() != NULL)
		t->view->RemoveSelf();
	delete t->view;
	uiFreeControl(cc);
}

uiHaikuControlAllDefaultsExceptDestroy(uiTab)

static tabPage *makePage(uiTab *t, uiControl *c)
{
	tabPage *pg = new tabPage();
	pg->child = c;
	pg->margined = 0;
	pg->container = new BGroupView(B_VERTICAL);
	uiControlSetParent(c, uiControl(t));
	uiHaikuControlSetContainer(uiHaikuControl(c), pg->container, 0);
	return pg;
}

void uiTabAppend(uiTab *t, const char *name, uiControl *c)
{
	BWindow *win = t->view->Window();
	if (win != NULL) win->Lock();
	tabPage *pg = makePage(t, c);
	t->view->AddTab(pg->container);
	t->view->TabAt(t->view->CountTabs() - 1)->SetLabel(name);
	t->pages->AddItem(pg);
	if (win != NULL) win->Unlock();
}

void uiTabInsertAt(uiTab *t, const char *name, int index, uiControl *c)
{
	int32 n = t->view->CountTabs();
	if (index >= n) {
		uiTabAppend(t, name, c);
		return;
	}
	BWindow *win = t->view->Window();
	if (win != NULL) win->Lock();

	BList saved;	// BTab*, removed from index..end (views preserved)
	for (int32 i = n - 1; i >= index; i--)
		saved.AddItem(t->view->RemoveTab(i));

	tabPage *pg = makePage(t, c);
	t->view->AddTab(pg->container);
	t->view->TabAt(t->view->CountTabs() - 1)->SetLabel(name);

	for (int32 i = saved.CountItems() - 1; i >= 0; i--) {
		BTab *tb = (BTab *) saved.ItemAt(i);
		t->view->AddTab(tb->View(), tb);
	}
	t->pages->AddItem(pg, index);
	if (win != NULL) win->Unlock();
}

void uiTabDelete(uiTab *t, int index)
{
	tabPage *pg = (tabPage *) t->pages->ItemAt(index);
	if (pg == NULL)
		return;
	BWindow *win = t->view->Window();
	if (win != NULL) win->Lock();
	BTab *tb = t->view->RemoveTab(index);
	// detach the child first so deleting the tab/container doesn't take the (caller-owned) child
	uiHaikuControlSetContainer(uiHaikuControl(pg->child), pg->container, 1);
	uiControlSetParent(pg->child, NULL);
	delete tb;		// deletes the BTab and its view (pg->container)
	t->pages->RemoveItem(index);
	delete pg;
	if (win != NULL) win->Unlock();
}

int uiTabNumPages(uiTab *t)
{
	return (int) t->pages->CountItems();
}

int uiTabSelected(uiTab *t)
{
	return t->view->Selection();
}

void uiTabSetSelected(uiTab *t, int index)
{
	BWindow *win = t->view->Window();
	if (win != NULL) win->Lock();
	t->view->suppress = true;
	t->view->Select(index);
	t->view->suppress = false;
	if (win != NULL) win->Unlock();
}

void uiTabOnSelected(uiTab *t, void (*f)(uiTab *, void *), void *data)
{
	t->onSelected = f;
	t->onSelectedData = data;
}

int uiTabMargined(uiTab *t, int index)
{
	tabPage *pg = (tabPage *) t->pages->ItemAt(index);
	if (pg == NULL)
		return 0;
	return pg->margined;
}

void uiTabSetMargined(uiTab *t, int index, int margined)
{
	tabPage *pg = (tabPage *) t->pages->ItemAt(index);
	if (pg == NULL)
		return;
	pg->margined = margined;
	BWindow *win = t->view->Window();
	if (win != NULL) win->Lock();
	float m = margined ? B_USE_DEFAULT_SPACING : 0;
	pg->container->GroupLayout()->SetInsets(m, m, m, m);
	if (win != NULL) win->Unlock();
}

uiTab *uiNewTab(void)
{
	uiTab *t;

	uiHaikuNewControl(uiTab, t);
	uiControl(t)->Destroy = uiTabDestroy;
	t->view = new UiTabView();
	t->view->owner = t;
	t->pages = new BList();
	t->onSelected = defaultOnSelected;

	return t;
}
