// 22 september 2026 -- uiTree on SysTreeView32 (torial fork).
//
// Lazy: a node whose HasChildren says yes gets ONE placeholder child so the expander
// shows; TVN_ITEMEXPANDING replaces the placeholder with the real children the first
// time. HTREEITEM <-> app node is a map both ways; lParam also carries the app node.
// Model notifications touch only parents that have been populated.
#include "uipriv_windows.hpp"
#include "../common/tree.h"
#include <map>
#include <vector>

// the model: the handler plus the views showing it (a uiTreeModel can back several uiTrees)
struct uiTreeModel {
	uiTreeModelHandler *mh;
	std::vector<uiTree *> *trees;
};

static void treeNodeInserted(uiTree *t, void *parent, int index);
static void treeNodeDeleted(uiTree *t, void *parent, int index);
static void treeNodeChanged(uiTree *t, void *node);

uiTreeModel *uiNewTreeModel(uiTreeModelHandler *mh)
{
	uiTreeModel *m;

	m = uiprivNew(uiTreeModel);
	m->mh = mh;
	m->trees = new std::vector<uiTree *>;
	return m;
}

void uiFreeTreeModel(uiTreeModel *m)
{
	if (m->trees->size() != 0)
		uiprivUserBug("You cannot free a uiTreeModel while uiTrees are using it.");
	delete m->trees;
	uiprivFree(m);
}

uiTreeModelHandler *uiprivTreeModelHandler(uiTreeModel *m)
{
	return m->mh;
}

void uiTreeModelNodeInserted(uiTreeModel *m, void *parent, int index)
{
	for (auto t : *(m->trees))
		treeNodeInserted(t, parent, index);
}

void uiTreeModelNodeDeleted(uiTreeModel *m, void *parent, int index)
{
	for (auto t : *(m->trees))
		treeNodeDeleted(t, parent, index);
}

void uiTreeModelNodeChanged(uiTreeModel *m, void *node)
{
	for (auto t : *(m->trees))
		treeNodeChanged(t, node);
}

struct uiTree {
	uiWindowsControl c;
	uiTreeModel *model;
	HWND hwnd;
	std::map<void *, HTREEITEM> *items;      // app node -> item
	std::map<HTREEITEM, void *> *nodes;      // item -> app node
	std::map<void *, bool> *populated;       // app node -> children inserted?
	BOOL inSet;                              // a programmatic change; do not fire callbacks
	HIMAGELIST himl;                         // icons (2026-09-23): built lazily from the handler's uiImages
	std::map<uiImage *, int> *icons;         // uiImage -> image list index
	void (*onSelectionChanged)(uiTree *, void *);
	void *onSelectionChangedData;
	void (*onNodeActivated)(uiTree *, void *, void *);
	void *onNodeActivatedData;
	void (*onNodeExpanded)(uiTree *, void *, int, void *);
	void *onNodeExpandedData;
};

#define treePlaceholder ((void *) (intptr_t) (-1))

// -1 when the node has no icon. Images are converted once (16x16 logical, at the DC's DPI
// via uiprivWICToGDI) and cached by uiImage pointer; TVSIL_NORMAL is set on the first one.
static int iconIndex(uiTree *t, void *node)
{
	uiImage *img;
	IWICBitmap *wb;
	HBITMAP hb = NULL;
	HDC dc;
	int idx;
	int sz;

	img = uiprivTreeModelIcon(t->model, node);
	if (img == NULL)
		return -1;
	auto it = t->icons->find(img);
	if (it != t->icons->end())
		return it->second;
	dc = GetDC(t->hwnd);
	sz = GetSystemMetrics(SM_CXSMICON);
	if (t->himl == NULL) {
		t->himl = ImageList_Create(sz, sz, ILC_COLOR32 | ILC_MASK, 4, 4);
		if (t->himl == NULL) {
			ReleaseDC(t->hwnd, dc);
			return -1;
		}
		TreeView_SetImageList(t->hwnd, t->himl, TVSIL_NORMAL);
	}
	wb = uiprivImageAppropriateForDC(img, dc);
	if (uiprivWICToGDI(wb, dc, sz, sz, &hb) != S_OK || hb == NULL) {
		ReleaseDC(t->hwnd, dc);
		return -1;
	}
	idx = ImageList_Add(t->himl, hb, NULL);
	DeleteObject(hb);
	ReleaseDC(t->hwnd, dc);
	(*(t->icons))[img] = idx;
	return idx;
}

static HTREEITEM insertItem(uiTree *t, HTREEITEM parent, HTREEITEM after, void *node)
{
	TVINSERTSTRUCTW tvi;
	HTREEITEM h;
	const char *text;
	WCHAR *wtext;

	ZeroMemory(&tvi, sizeof (TVINSERTSTRUCTW));
	tvi.hParent = parent;
	tvi.hInsertAfter = after;
	tvi.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
	text = uiprivTreeModelText(t->model, node);
	wtext = toUTF16(text);
	tvi.item.pszText = wtext;
	tvi.item.lParam = (LPARAM) node;
	tvi.item.cChildren = uiprivTreeModelHasChildren(t->model, node) ? 1 : 0;
	tvi.item.iImage = iconIndex(t, node);
	if (tvi.item.iImage >= 0) {
		tvi.item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvi.item.iSelectedImage = tvi.item.iImage;
	}
	h = TreeView_InsertItem(t->hwnd, &tvi);
	uiprivFree(wtext);
	if (h == NULL) {
		logLastError(L"error calling TreeView_InsertItem in uiTree");
		return NULL;
	}
	(*(t->items))[node] = h;
	(*(t->nodes))[h] = node;
	return h;
}

static HTREEITEM itemAt(uiTree *t, HTREEITEM parent, int index)
{
	HTREEITEM h;

	h = TreeView_GetChild(t->hwnd, parent);
	while (index > 0 && h != NULL) {
		h = TreeView_GetNextSibling(t->hwnd, h);
		index--;
	}
	return h;
}

// insert the real children of node (parent item h); called once, on first expand or
// at creation for the root
static void populate(uiTree *t, void *node, HTREEITEM h)
{
	int n, i;
	HTREEITEM after = TVI_LAST;

	if ((*(t->populated))[node])
		return;
	(*(t->populated))[node] = true;
	n = uiprivTreeModelNumChildren(t->model, node);
	for (i = 0; i < n; i++)
		insertItem(t, h, after, uiprivTreeModelChild(t->model, node, i));
}

static void forgetSubtree(uiTree *t, HTREEITEM h)
{
	HTREEITEM child;
	std::map<HTREEITEM, void *>::iterator it;

	for (child = TreeView_GetChild(t->hwnd, h); child != NULL; child = TreeView_GetNextSibling(t->hwnd, child))
		forgetSubtree(t, child);
	it = t->nodes->find(h);
	if (it != t->nodes->end()) {
		t->items->erase(it->second);
		t->populated->erase(it->second);
		t->nodes->erase(it);
	}
}

static BOOL onWM_NOTIFY(uiControl *c, HWND hwnd, NMHDR *nmhdr, LRESULT *lResult)
{
	uiTree *t = uiTree(c);
	NMTREEVIEWW *nm = (NMTREEVIEWW *) nmhdr;

	switch (nmhdr->code) {
	case TVN_ITEMEXPANDINGW:
		if (nm->action == TVE_EXPAND)
			populate(t, (void *) nm->itemNew.lParam, nm->itemNew.hItem);
		*lResult = 0;
		return TRUE;
	case TVN_ITEMEXPANDEDW:
		if (!t->inSet)
			(*(t->onNodeExpanded))(t, (void *) nm->itemNew.lParam, nm->action == TVE_EXPAND, t->onNodeExpandedData);
		*lResult = 0;
		return TRUE;
	case TVN_SELCHANGEDW:
		if (!t->inSet)
			(*(t->onSelectionChanged))(t, t->onSelectionChangedData);
		*lResult = 0;
		return TRUE;
	case NM_DBLCLK:
	case NM_RETURN:
		{
			HTREEITEM sel = TreeView_GetSelection(t->hwnd);
			if (sel != NULL) {
				std::map<HTREEITEM, void *>::iterator it = t->nodes->find(sel);
				if (it != t->nodes->end())
					(*(t->onNodeActivated))(t, it->second, t->onNodeActivatedData);
			}
		}
		*lResult = 0;
		return TRUE;
	}
	return FALSE;
}

static void uiTreeDestroy(uiControl *c)
{
	uiTree *t = uiTree(c);

	uiWindowsUnregisterWM_NOTIFYHandler(t->hwnd);
	uiWindowsEnsureDestroyWindow(t->hwnd);
	for (auto it = t->model->trees->begin(); it != t->model->trees->end(); it++)
		if (*it == t) {
			t->model->trees->erase(it);
			break;
		}
	delete t->items;
	delete t->nodes;
	delete t->populated;
	delete t->icons;
	if (t->himl != NULL)
		ImageList_Destroy(t->himl);
	uiFreeControl(uiControl(t));
}

uiWindowsControlAllDefaultsExceptDestroy(uiTree)

#define treeMinWidth 107
#define treeMinHeight (14 * 3)

static void uiTreeMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	uiTree *t = uiTree(c);
	uiWindowsSizing sizing;
	int x, y;

	x = treeMinWidth;
	y = treeMinHeight;
	uiWindowsGetSizing(t->hwnd, &sizing);
	uiWindowsSizingDlgUnitsToPixels(&sizing, &x, &y);
	*width = x;
	*height = y;
}

static void defaultOnSelectionChanged(uiTree *t, void *data) {}
static void defaultOnNodeActivated(uiTree *t, void *node, void *data) {}
static void defaultOnNodeExpanded(uiTree *t, void *node, int expanded, void *data) {}

void uiTreeOnSelectionChanged(uiTree *t, void (*f)(uiTree *, void *), void *data) { t->onSelectionChanged = f; t->onSelectionChangedData = data; }
void uiTreeOnNodeActivated(uiTree *t, void (*f)(uiTree *, void *, void *), void *data) { t->onNodeActivated = f; t->onNodeActivatedData = data; }
void uiTreeOnNodeExpanded(uiTree *t, void (*f)(uiTree *, void *, int, void *), void *data) { t->onNodeExpanded = f; t->onNodeExpandedData = data; }

void uiTreeSetExpanded(uiTree *t, void *node, int expanded)
{
	std::map<void *, HTREEITEM>::iterator it = t->items->find(node);

	if (it == t->items->end())
		return;
	t->inSet = TRUE;
	if (expanded)
		populate(t, node, it->second);
	TreeView_Expand(t->hwnd, it->second, expanded ? TVE_EXPAND : TVE_COLLAPSE);
	t->inSet = FALSE;
}

int uiTreeExpanded(uiTree *t, void *node)
{
	std::map<void *, HTREEITEM>::iterator it = t->items->find(node);

	if (it == t->items->end())
		return 0;
	return (TreeView_GetItemState(t->hwnd, it->second, TVIS_EXPANDED) & TVIS_EXPANDED) != 0;
}

void *uiTreeSelection(uiTree *t)
{
	HTREEITEM sel = TreeView_GetSelection(t->hwnd);
	std::map<HTREEITEM, void *>::iterator it;

	if (sel == NULL)
		return NULL;
	it = t->nodes->find(sel);
	if (it == t->nodes->end())
		return NULL;
	return it->second;
}

void uiTreeSetSelection(uiTree *t, void *node)
{
	std::map<void *, HTREEITEM>::iterator it;

	t->inSet = TRUE;
	if (node == NULL)
		TreeView_SelectItem(t->hwnd, NULL);
	else {
		it = t->items->find(node);
		if (it != t->items->end())
			TreeView_SelectItem(t->hwnd, it->second);
	}
	t->inSet = FALSE;
}

static void treeNodeInserted(uiTree *t, void *parent, int index)
{
	HTREEITEM hparent = TVI_ROOT;
	HTREEITEM after;

	if (parent != NULL) {
		std::map<void *, HTREEITEM>::iterator it = t->items->find(parent);
		if (it == t->items->end())
			return;
		hparent = it->second;
		if (!(*(t->populated))[parent]) {
			// not opened yet: the expander state may need refreshing, nothing else
			TVITEMW tvi;
			ZeroMemory(&tvi, sizeof (TVITEMW));
			tvi.mask = TVIF_HANDLE | TVIF_CHILDREN;
			tvi.hItem = hparent;
			tvi.cChildren = 1;
			TreeView_SetItem(t->hwnd, &tvi);
			return;
		}
	}
	after = index == 0 ? TVI_FIRST : itemAt(t, hparent, index - 1);
	if (after == NULL)
		after = TVI_LAST;
	insertItem(t, hparent, after, uiprivTreeModelChild(t->model, parent, index));
}

static void treeNodeDeleted(uiTree *t, void *parent, int index)
{
	HTREEITEM hparent = TVI_ROOT;
	HTREEITEM h;

	if (parent != NULL) {
		std::map<void *, HTREEITEM>::iterator it = t->items->find(parent);
		if (it == t->items->end() || !(*(t->populated))[parent])
			return;
		hparent = it->second;
	}
	h = itemAt(t, hparent, index);
	if (h == NULL)
		return;
	t->inSet = TRUE;
	forgetSubtree(t, h);
	TreeView_DeleteItem(t->hwnd, h);
	t->inSet = FALSE;
}

static void treeNodeChanged(uiTree *t, void *node)
{
	std::map<void *, HTREEITEM>::iterator it = t->items->find(node);
	TVITEMW tvi;
	const char *text;
	WCHAR *wtext;

	if (it == t->items->end())
		return;
	text = uiprivTreeModelText(t->model, node);
	wtext = toUTF16(text);
	ZeroMemory(&tvi, sizeof (TVITEMW));
	tvi.mask = TVIF_HANDLE | TVIF_TEXT | TVIF_CHILDREN;
	tvi.hItem = it->second;
	tvi.pszText = wtext;
	tvi.cChildren = uiprivTreeModelHasChildren(t->model, node) ? 1 : 0;
	tvi.iImage = iconIndex(t, node);
	if (tvi.iImage >= 0) {
		tvi.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvi.iSelectedImage = tvi.iImage;
	}
	TreeView_SetItem(t->hwnd, &tvi);
	uiprivFree(wtext);
}

uiTree *uiNewTree(uiTreeModel *m)
{
	uiTree *t;

	uiWindowsNewControl(uiTree, t);

	t->model = m;
	t->items = new std::map<void *, HTREEITEM>;
	t->nodes = new std::map<HTREEITEM, void *>;
	t->populated = new std::map<void *, bool>;
	t->icons = new std::map<uiImage *, int>;
	t->himl = NULL;
	t->inSet = FALSE;
	uiTreeOnSelectionChanged(t, defaultOnSelectionChanged, NULL);
	uiTreeOnNodeActivated(t, defaultOnNodeActivated, NULL);
	uiTreeOnNodeExpanded(t, defaultOnNodeExpanded, NULL);

	t->hwnd = uiWindowsEnsureCreateControlHWND(WS_EX_CLIENTEDGE,
		WC_TREEVIEWW, L"",
		TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS | WS_TABSTOP | WS_HSCROLL | WS_VSCROLL,
		hInstance, NULL,
		TRUE);
	m->trees->push_back(t);
	uiWindowsRegisterWM_NOTIFYHandler(t->hwnd, onWM_NOTIFY, uiControl(t));
	populate(t, NULL, TVI_ROOT);
	return t;
}
