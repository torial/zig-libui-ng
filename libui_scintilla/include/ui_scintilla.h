#ifdef __cplusplus
extern "C" {
#endif

typedef struct uiScintilla uiScintilla;
_UI_EXTERN void uiScintillaGetRange(uiScintilla *s, unsigned int start, unsigned int end, char *text);
_UI_EXTERN void uiScintillaSetText(uiScintilla *s, const char *text, unsigned int len);
_UI_EXTERN void uiScintillaAppend(uiScintilla *s, const char *text);
_UI_EXTERN unsigned int uiScintillaGetLength(uiScintilla *s);
_UI_EXTERN uiScintilla *uiNewScintilla();
_UI_EXTERN char *uiScintillaText(uiScintilla *s);
_UI_EXTERN uintptr_t uiScintillaSendMessage(uiScintilla *s, uint32_t code, uintptr_t w, uintptr_t l);
// Scintilla notifications (SCN_MODIFIED, SCN_CHARADDED, SCN_MARGINCLICK, SCN_DWELLSTART, ...).
// `n` is the SCNotification (Scintilla.h); it is valid only for the duration of the call.
// Delivered on the UI thread from libui's WM_NOTIFY routing; one handler per control.
struct SCNotification;
_UI_EXTERN void uiScintillaOnNotify(uiScintilla *s, void (*f)(uiScintilla *s, struct SCNotification *n, void *data), void *data);
// Keys, before Scintilla sees them. `vk` is the Windows virtual-key code (VK_F5 = 0x74,
// letters are their ASCII uppercase), `mods` a bitmask: 1 Ctrl, 2 Shift, 4 Alt. Return
// nonzero to consume the key (Scintilla never sees it, nor the WM_CHAR after it); zero
// to let it through. One handler per control; UI thread.
_UI_EXTERN void uiScintillaOnKey(uiScintilla *s, int (*f)(uiScintilla *s, int vk, int mods, void *data), void *data);

#ifdef __cplusplus
}
#endif
