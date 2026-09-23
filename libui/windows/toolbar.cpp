// 23 september 2026 -- uiToolbar on Windows: a ToolbarWindow32 the window keeps at the
// top of its client area (window.cpp's relayout subtracts its height). The HWND needs a
// parent, so items appended before uiWindowSetToolbar() are recorded and replayed when the
// window attaches; after that every change rebuilds the buttons from the record, which is
// a handful of TB_ messages on a control that never holds more than a few items.
#include "uipriv_windows.hpp"

struct tbItem {
	WCHAR *label;		// NULL for a separator
	uiImage *icon;
	WCHAR *tooltip;
	BOOL enabled;
};

struct uiToolbar {
	HWND hwnd;				// NULL until attached
	uiWindow *window;
	HIMAGELIST imagelist;
	std::vector<struct tbItem> *items;
	void (*onClicked)(uiToolbar *, int, void *);
	void *onClickedData;
};

// command IDs: the item index plus this, so that a click reports the index and the
// window's own WM_COMMAND arm (which reserves the low IDs for menus) never sees them
#define toolbarIDBase 40000

static void defaultOnClicked(uiToolbar *t, int index, void *data)
{
	// do nothing
}

// clicks come through WM_NOTIFY: NM_CLICK carries the command ID, where runWM_COMMAND()
// hands a handler only the notification code. Tooltips likewise: TBN_GETINFOTIP is sent by
// the toolbar itself (TTN_GETDISPINFO comes from the tooltip window, which is not registered).
static BOOL onWM_NOTIFY(uiControl *c, HWND hwnd, NMHDR *nm, LRESULT *lResult)
{
	uiToolbar *t = (uiToolbar *) c;
	NMTBGETINFOTIPW *tip;
	int index;

	switch (nm->code) {
	case NM_CLICK:
		// NMMOUSE.dwItemSpec is the command ID of the button clicked (or -1)
		{
			NMMOUSE *nmm = (NMMOUSE *) nm;

			if (nmm->dwItemSpec == (DWORD_PTR) -1)
				return FALSE;
			index = (int) (nmm->dwItemSpec - toolbarIDBase);
			if (index < 0 || (size_t) index >= t->items->size())
				return FALSE;
			if (!(*(t->items))[index].enabled)
				return FALSE;
			(*(t->onClicked))(t, index, t->onClickedData);
			*lResult = 0;
			return TRUE;
		}
	case TBN_GETINFOTIPW:
		tip = (NMTBGETINFOTIPW *) nm;
		index = tip->iItem - toolbarIDBase;
		if (index < 0 || (size_t) index >= t->items->size())
			return FALSE;
		if ((*(t->items))[index].tooltip == NULL)
			return FALSE;
		wcsncpy(tip->pszText, (*(t->items))[index].tooltip, tip->cchTextMax);
		tip->pszText[tip->cchTextMax - 1] = L'\0';
		*lResult = 0;
		return TRUE;
	}
	return FALSE;
}

uiToolbar *uiNewToolbar(void)
{
	uiToolbar *t;

	t = uiprivNew(uiToolbar);
	t->hwnd = NULL;
	t->window = NULL;
	t->imagelist = NULL;
	t->items = new std::vector<struct tbItem>;
	t->onClicked = defaultOnClicked;
	t->onClickedData = NULL;
	return t;
}

static void freeItems(uiToolbar *t)
{
	for (auto &it : *(t->items)) {
		if (it.label != NULL)
			uiprivFree(it.label);
		if (it.tooltip != NULL)
			uiprivFree(it.tooltip);
	}
	t->items->clear();
}

// rebuild the control's buttons and image list from the record; a no-op until attached
static void sync(uiToolbar *t)
{
	int n, i;
	HDC dc;
	int sz;
	std::vector<TBBUTTON> buttons;

	if (t->hwnd == NULL)
		return;
	n = (int) SendMessageW(t->hwnd, TB_BUTTONCOUNT, 0, 0);
	while (n > 0) {
		SendMessageW(t->hwnd, TB_DELETEBUTTON, 0, 0);
		n--;
	}
	// a fresh image list each time: icons are added in item order below
	dc = GetDC(t->hwnd);
	sz = GetSystemMetrics(SM_CXSMICON);
	if (t->imagelist != NULL) {
		SendMessageW(t->hwnd, TB_SETIMAGELIST, 0, (LPARAM) NULL);
		ImageList_Destroy(t->imagelist);
		t->imagelist = NULL;
	}
	t->imagelist = ImageList_Create(sz, sz, ILC_COLOR32 | ILC_MASK, 4, 4);
	if (t->imagelist != NULL)
		SendMessageW(t->hwnd, TB_SETIMAGELIST, 0, (LPARAM) t->imagelist);
	i = 0;
	for (auto &it : *(t->items)) {
		TBBUTTON b;

		ZeroMemory(&b, sizeof (TBBUTTON));
		b.idCommand = toolbarIDBase + i;
		if (it.label == NULL) {
			b.fsStyle = BTNS_SEP;
			b.iBitmap = 8;		// separator width
		} else {
			b.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
			b.fsState = it.enabled ? TBSTATE_ENABLED : 0;
			b.iBitmap = I_IMAGENONE;
			b.iString = (INT_PTR) it.label;
			if (it.icon != NULL && t->imagelist != NULL) {
				IWICBitmap *wb;
				HBITMAP hb;

				wb = uiprivImageAppropriateForDC(it.icon, dc);
				if (wb != NULL && uiprivWICToGDI(wb, dc, sz, sz, &hb) == S_OK && hb != NULL) {
					b.iBitmap = ImageList_Add(t->imagelist, hb, NULL);
					DeleteObject(hb);
				}
			}
		}
		buttons.push_back(b);
		i++;
	}
	ReleaseDC(t->hwnd, dc);
	if (!buttons.empty())
		SendMessageW(t->hwnd, TB_ADDBUTTONSW, (WPARAM) buttons.size(), (LPARAM) buttons.data());
	SendMessageW(t->hwnd, TB_AUTOSIZE, 0, 0);
	if (t->window != NULL)
		uiWindowsControlMinimumSizeChanged(uiWindowsControl(t->window));
}

int uiToolbarAppendItem(uiToolbar *t, const char *label, uiImage *icon, const char *tooltip)
{
	struct tbItem it;

	it.label = toUTF16(label);
	it.icon = icon;
	it.tooltip = tooltip != NULL ? toUTF16(tooltip) : NULL;
	it.enabled = TRUE;
	t->items->push_back(it);
	sync(t);
	return (int) t->items->size() - 1;
}

int uiToolbarAppendSeparator(uiToolbar *t)
{
	struct tbItem it;

	it.label = NULL;
	it.icon = NULL;
	it.tooltip = NULL;
	it.enabled = TRUE;
	t->items->push_back(it);
	sync(t);
	return (int) t->items->size() - 1;
}

int uiToolbarNumItems(uiToolbar *t)
{
	return (int) t->items->size();
}

void uiToolbarClear(uiToolbar *t)
{
	freeItems(t);
	sync(t);
}

void uiToolbarSetItemEnabled(uiToolbar *t, int index, int enabled)
{
	if (index < 0 || (size_t) index >= t->items->size())
		return;
	(*(t->items))[index].enabled = enabled ? TRUE : FALSE;
	if (t->hwnd != NULL)
		SendMessageW(t->hwnd, TB_ENABLEBUTTON, toolbarIDBase + index, MAKELPARAM(enabled ? TRUE : FALSE, 0));
}

void uiToolbarOnClicked(uiToolbar *t, void (*f)(uiToolbar *, int, void *), void *data)
{
	t->onClicked = f;
	t->onClickedData = data;
}

// window.cpp's half

void uiprivToolbarAttach(uiToolbar *t, uiWindow *w, HWND parent)
{
	t->window = w;
	t->hwnd = CreateWindowExW(0,
		TOOLBARCLASSNAMEW, L"",
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS | CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN,
		0, 0, 0, 0,
		parent, NULL, hInstance, NULL);
	if (t->hwnd == NULL) {
		logLastError(L"error creating toolbar");
		return;
	}
	SendMessageW(t->hwnd, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof (TBBUTTON), 0);
	SendMessageW(t->hwnd, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS);
	SendMessageW(t->hwnd, WM_SETFONT, (WPARAM) hMessageFont, TRUE);
	uiWindowsRegisterWM_NOTIFYHandler(t->hwnd, onWM_NOTIFY, (uiControl *) t);
	sync(t);
}

// place it at the top of the client area at the given width; returns the height it took
int uiprivToolbarLayout(uiToolbar *t, int width)
{
	SIZE sz;
	int h;

	if (t->hwnd == NULL)
		return 0;
	if (t->items->empty()) {
		uiWindowsEnsureMoveWindowDuringResize(t->hwnd, 0, 0, width, 0);
		return 0;
	}
	SendMessageW(t->hwnd, TB_AUTOSIZE, 0, 0);
	if (SendMessageW(t->hwnd, TB_GETMAXSIZE, 0, (LPARAM) &sz) == 0)
		sz.cy = 0;
	h = (int) sz.cy;
	if (h <= 0) {
		RECT r;

		GetWindowRect(t->hwnd, &r);
		h = r.bottom - r.top;
	}
	uiWindowsEnsureMoveWindowDuringResize(t->hwnd, 0, 0, width, h);
	return h;
}

int uiprivToolbarMinimumWidth(uiToolbar *t)
{
	SIZE sz;

	if (t->hwnd == NULL || t->items->empty())
		return 0;
	if (SendMessageW(t->hwnd, TB_GETMAXSIZE, 0, (LPARAM) &sz) == 0)
		return 0;
	return (int) sz.cx;
}

void uiprivFreeToolbar(uiToolbar *t)
{
	if (t->hwnd != NULL) {
		uiWindowsUnregisterWM_NOTIFYHandler(t->hwnd);
		uiWindowsEnsureDestroyWindow(t->hwnd);
		t->hwnd = NULL;
	}
	if (t->imagelist != NULL)
		ImageList_Destroy(t->imagelist);
	freeItems(t);
	delete t->items;
	uiprivFree(t);
}
