// 23 september 2026 -- tooltips and the clipboard on Windows (torial fork)
#include "uipriv_windows.hpp"
#include <map>

// One TOOLTIPS_CLASS window per control, keyed by the control's HWND. TTF_SUBCLASS lets
// the tooltip control subclass the target to see its mouse messages, so nothing here
// has to relay them. Owner is the control's top-level window so it stays above it.
static std::map<HWND, HWND> *tips = NULL;

void uiControlSetTooltip(uiControl *c, const char *text)
{
	HWND target = (HWND) uiControlHandle(c);
	HWND tip = NULL;
	TOOLINFOW ti;
	WCHAR *wtext;

	if (tips == NULL)
		tips = new std::map<HWND, HWND>;
	auto it = tips->find(target);
	if (it != tips->end())
		tip = it->second;

	ZeroMemory(&ti, sizeof (TOOLINFOW));
	ti.cbSize = sizeof (TOOLINFOW);
	ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	ti.hwnd = GetParent(target);
	ti.uId = (UINT_PTR) target;

	if (text == NULL || *text == '\0') {
		if (tip != NULL) {
			SendMessageW(tip, TTM_DELTOOLW, 0, (LPARAM) &ti);
			DestroyWindow(tip);
			tips->erase(it);
		}
		return;
	}

	wtext = toUTF16(text);
	if (tip == NULL) {
		tip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL,
			WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			GetAncestor(target, GA_ROOT), NULL, hInstance, NULL);
		if (tip == NULL) {
			logLastError(L"error creating tooltip window");
			uiprivFree(wtext);
			return;
		}
		ti.lpszText = wtext;
		SendMessageW(tip, TTM_ADDTOOLW, 0, (LPARAM) &ti);
		SendMessageW(tip, TTM_SETMAXTIPWIDTH, 0, 400);		// wrap long text
		(*tips)[target] = tip;
	} else {
		ti.lpszText = wtext;
		SendMessageW(tip, TTM_UPDATETIPTEXTW, 0, (LPARAM) &ti);
	}
	uiprivFree(wtext);
}

char *uiClipboardText(void)
{
	HANDLE h;
	WCHAR *w;
	char *out = NULL;

	if (!IsClipboardFormatAvailable(CF_UNICODETEXT))
		return NULL;
	if (!OpenClipboard(NULL))
		return NULL;
	h = GetClipboardData(CF_UNICODETEXT);
	if (h != NULL) {
		w = (WCHAR *) GlobalLock(h);
		if (w != NULL) {
			out = toUTF8(w);
			GlobalUnlock(h);
		}
	}
	CloseClipboard();
	return out;
}

void uiClipboardSetText(const char *text)
{
	WCHAR *w;
	size_t n;
	HGLOBAL h;
	WCHAR *dst;

	w = toUTF16(text == NULL ? "" : text);
	n = (wcslen(w) + 1) * sizeof (WCHAR);
	if (!OpenClipboard(NULL)) {
		uiprivFree(w);
		return;
	}
	EmptyClipboard();
	h = GlobalAlloc(GMEM_MOVEABLE, n);
	if (h != NULL) {
		dst = (WCHAR *) GlobalLock(h);
		if (dst != NULL) {
			memcpy(dst, w, n);
			GlobalUnlock(h);
			if (SetClipboardData(CF_UNICODETEXT, h) == NULL)
				GlobalFree(h);
		} else
			GlobalFree(h);
	}
	CloseClipboard();
	uiprivFree(w);
}
