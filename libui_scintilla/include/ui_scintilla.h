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

#ifdef __cplusplus
}
#endif
