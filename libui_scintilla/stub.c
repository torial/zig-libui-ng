// Placeholder so the `sci` static library has an object to link on platforms
// without a Scintilla build (darwin; Windows and GTK have shims).
int uiScintillaStubUnavailable(void) { return 1; }
