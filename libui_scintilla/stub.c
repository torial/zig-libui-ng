// Non-Windows placeholder so the `sci` static library has an object to link on
// platforms without the Scintilla build (the shim is Windows-only for now).
int uiScintillaStubUnavailable(void) { return 1; }
