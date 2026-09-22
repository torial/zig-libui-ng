// libui-ng Haiku backend — uiTree (BOutlineListView inside a BScrollView), torial fork 2026-09-22.
// WRITTEN BLIND: no Haiku build has compiled or run this yet, like the darwin leg.
// Push-based like the Haiku table: the outline view keeps its own BStringItems, one per
// app node, kept in a parallel map so notifications can find them. Children are
// populated eagerly (BOutlineListView has no lazy expand hook), collapsed where the app
// has not expanded them.
#include "uipriv_haiku.h"
#include "../common/tree.h"
#include <OutlineListView.h>
#include <ScrollView.h>
#include <StringItem.h>
#include <map>

struct uiTreeModel {
	uiTreeModelHandler *mh;
	BList *trees;		// uiTree*
};

struct uiTreeItem : public BStringItem {
	void *node;
	uiTreeItem(const char *text, uint32 level, void *n) : BStringItem(text, level, false), node(n) {}
};

struct uiTree {
	uiHaikuControl c;
	BScrollView *view;		// outer (added to parents); macros operate on this
	BOutlineListView *list;
	uiTreeModel *m;
	std::map<void *, uiTreeItem *> *items;
	bool inSet;
	void (*onSelectionChanged)(uiTree *, void *);
	void *onSelectionChangedData;
	void (*onNodeActivated)(uiTree *, void *, void *);
	void *onNodeActivatedData;
	void (*onNodeExpanded)(uiTree *, void *, int, void *);
	void *onNodeExpandedData;
};

uiHaikuControlAllDefaults(uiTree)

uiTreeModel *uiNewTreeModel(uiTreeModelHandler *mh)
{
	uiTreeModel *m = uiprivNew(uiTreeModel);
	m->mh = mh;
	m->trees = new BList();
	return m;
}

void uiFreeTreeModel(uiTreeModel *m)
{
	delete m->trees;
	uiprivFree(m);
}

uiTreeModelHandler *uiprivTreeModelHandler(uiTreeModel *m)
{
	return m->mh;
}

static void lockWin(uiTree *t, BWindow **win)
{
	*win = t->list->Window();
	if (*win != NULL) (*win)->Lock();
}
static void unlockWin(BWindow *win)
{
	if (win != NULL) win->Unlock();
}

// the full-list row at which the index-th child of parentItem (NULL = top level) sits or
// would sit: walk the siblings at `level`, skipping each one's subtree
static int32 childRow(uiTree *t, uiTreeItem *parentItem, int index, uint32 level)
{
	int32 row = parentItem == NULL ? 0 : t->list->FullListIndexOf(parentItem) + 1;
	int32 seen = 0;
	BListItem *cur;

	while (seen < index && (cur = t->list->FullListItemAt(row)) != NULL) {
		if (cur->OutlineLevel() < level) break;
		row++;
		while ((cur = t->list->FullListItemAt(row)) != NULL && cur->OutlineLevel() > level) row++;
		seen++;
	}
	return row;
}

// insert node (and, eagerly, its subtree) as the index-th child under parentItem
static uiTreeItem *insertNode(uiTree *t, void *node, uiTreeItem *parentItem, int index, uint32 level)
{
	uiTreeItem *item = new uiTreeItem(uiprivTreeModelText(t->m, node), level, node);

	t->list->AddItem(item, childRow(t, parentItem, index, level));
	(*(t->items))[node] = item;
	int n = uiprivTreeModelNumChildren(t->m, node);
	for (int i = 0; i < n; i++)
		insertNode(t, uiprivTreeModelChild(t->m, node, i), item, i, level + 1);
	if (n > 0)
		t->list->Collapse(item);
	return item;
}

static void removeSubtree(uiTree *t, uiTreeItem *item)
{
	int32 row = t->list->FullListIndexOf(item);
	uint32 level = item->OutlineLevel();
	BListItem *next;

	while ((next = t->list->FullListItemAt(row + 1)) != NULL && next->OutlineLevel() > level) {
		uiTreeItem *ci = (uiTreeItem *) next;
		t->items->erase(ci->node);
		t->list->RemoveItem(row + 1);
		delete ci;
	}
	t->items->erase(item->node);
	t->list->RemoveItem(item);
	delete item;
}

static void treeNodeInserted(uiTree *t, void *parent, int index)
{
	BWindow *win;
	uiTreeItem *pi = NULL;

	if (parent != NULL) {
		std::map<void *, uiTreeItem *>::iterator it = t->items->find(parent);
		if (it == t->items->end()) return;
		pi = it->second;
	}
	lockWin(t, &win);
	insertNode(t, uiprivTreeModelChild(t->m, parent, index), pi, index, pi == NULL ? 0 : pi->OutlineLevel() + 1);
	unlockWin(win);
}

static void treeNodeDeleted(uiTree *t, void *parent, int index)
{
	BWindow *win;
	uiTreeItem *pi = NULL;
	uint32 level = 0;

	if (parent != NULL) {
		std::map<void *, uiTreeItem *>::iterator it = t->items->find(parent);
		if (it == t->items->end()) return;
		pi = it->second;
		level = pi->OutlineLevel() + 1;
	}
	BListItem *cur = t->list->FullListItemAt(childRow(t, pi, index, level));
	if (cur == NULL || cur->OutlineLevel() != level) return;
	lockWin(t, &win);
	removeSubtree(t, (uiTreeItem *) cur);
	unlockWin(win);
}

static void treeNodeChanged(uiTree *t, void *node)
{
	BWindow *win;
	std::map<void *, uiTreeItem *>::iterator it = t->items->find(node);

	if (it == t->items->end()) return;
	lockWin(t, &win);
	it->second->SetText(uiprivTreeModelText(t->m, node));
	t->list->InvalidateItem(t->list->IndexOf(it->second));
	unlockWin(win);
}

void uiTreeModelNodeInserted(uiTreeModel *m, void *parent, int index)
{
	for (int32 i = 0; i < m->trees->CountItems(); i++)
		treeNodeInserted((uiTree *) m->trees->ItemAt(i), parent, index);
}

void uiTreeModelNodeDeleted(uiTreeModel *m, void *parent, int index)
{
	for (int32 i = 0; i < m->trees->CountItems(); i++)
		treeNodeDeleted((uiTree *) m->trees->ItemAt(i), parent, index);
}

void uiTreeModelNodeChanged(uiTreeModel *m, void *node)
{
	for (int32 i = 0; i < m->trees->CountItems(); i++)
		treeNodeChanged((uiTree *) m->trees->ItemAt(i), node);
}

static void selectionDispatch(void *control)
{
	uiTree *t = (uiTree *) control;
	if (t->inSet) return;
	(*(t->onSelectionChanged))(t, t->onSelectionChangedData);
}

static void invokeDispatch(void *control)
{
	uiTree *t = (uiTree *) control;
	int32 row = t->list->CurrentSelection();
	if (row < 0) return;
	uiTreeItem *item = (uiTreeItem *) t->list->ItemAt(row);
	if (item != NULL)
		(*(t->onNodeActivated))(t, item->node, t->onNodeActivatedData);
}

static void defaultOnSelectionChanged(uiTree *t, void *data) { (void) t; (void) data; }
static void defaultOnNodeActivated(uiTree *t, void *node, void *data) { (void) t; (void) node; (void) data; }
static void defaultOnNodeExpanded(uiTree *t, void *node, int expanded, void *data) { (void) t; (void) node; (void) expanded; (void) data; }

void uiTreeOnSelectionChanged(uiTree *t, void (*f)(uiTree *, void *), void *data) { t->onSelectionChanged = f; t->onSelectionChangedData = data; }
void uiTreeOnNodeActivated(uiTree *t, void (*f)(uiTree *, void *, void *), void *data) { t->onNodeActivated = f; t->onNodeActivatedData = data; }
void uiTreeOnNodeExpanded(uiTree *t, void (*f)(uiTree *, void *, int, void *), void *data) { t->onNodeExpanded = f; t->onNodeExpandedData = data; }
// NOTE: BOutlineListView reports expand/collapse only through the latch click, which has no
// message hook in the public API; onNodeExpanded never fires on Haiku in this cut.

void uiTreeSetExpanded(uiTree *t, void *node, int expanded)
{
	BWindow *win;
	std::map<void *, uiTreeItem *>::iterator it = t->items->find(node);

	if (it == t->items->end()) return;
	lockWin(t, &win);
	t->inSet = true;
	if (expanded) t->list->Expand(it->second); else t->list->Collapse(it->second);
	t->inSet = false;
	unlockWin(win);
}

int uiTreeExpanded(uiTree *t, void *node)
{
	std::map<void *, uiTreeItem *>::iterator it = t->items->find(node);

	if (it == t->items->end()) return 0;
	return it->second->IsExpanded() ? 1 : 0;
}

void *uiTreeSelection(uiTree *t)
{
	int32 row = t->list->CurrentSelection();

	if (row < 0) return NULL;
	uiTreeItem *item = (uiTreeItem *) t->list->ItemAt(row);
	return item == NULL ? NULL : item->node;
}

void uiTreeSetSelection(uiTree *t, void *node)
{
	BWindow *win;

	lockWin(t, &win);
	t->inSet = true;
	if (node == NULL)
		t->list->DeselectAll();
	else {
		std::map<void *, uiTreeItem *>::iterator it = t->items->find(node);
		if (it != t->items->end())
			t->list->Select(t->list->IndexOf(it->second));
	}
	t->inSet = false;
	unlockWin(win);
}

uiTree *uiNewTree(uiTreeModel *m)
{
	uiTree *t;

	uiHaikuNewControl(uiTree, t);
	t->m = m;
	t->items = new std::map<void *, uiTreeItem *>;
	t->inSet = false;
	t->list = new BOutlineListView("uiTree", B_SINGLE_SELECTION_LIST);
	t->list->SetSelectionMessage(uiprivNewEventMessage(selectionDispatch, t));
	t->list->SetInvocationMessage(uiprivNewEventMessage(invokeDispatch, t));
	t->view = new BScrollView("uiTreeScroll", t->list, 0, false, true);
	t->onSelectionChanged = defaultOnSelectionChanged;
	t->onNodeActivated = defaultOnNodeActivated;
	t->onNodeExpanded = defaultOnNodeExpanded;
	m->trees->AddItem(t);
	int n = uiprivTreeModelNumChildren(m, NULL);
	for (int i = 0; i < n; i++)
		insertNode(t, uiprivTreeModelChild(m, NULL, i), NULL, i, 0);
	return t;
}
