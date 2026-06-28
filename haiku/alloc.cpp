// libui-ng Haiku backend — allocator.
// Minimal vs. unix/alloc.c: no leak-tracking GPtrArray (that exists to catch leaked uiControls;
// worth adding later, but it would pull GLib-style bookkeeping in here). uiprivAlloc zeroes like
// the unix one (controls assume zero-init), and uiprivRealloc zeroes the grown tail to match.
#include <stdlib.h>
#include <string.h>
#include "uipriv_haiku.h"

extern "C" void *uiprivAlloc(size_t size, const char *type)
{
	(void) type;
	return calloc(1, size);
}

extern "C" void *uiprivRealloc(void *p, size_t size, const char *type)
{
	if (p == NULL)
		return uiprivAlloc(size, type);
	// calloc/realloc don't track the old size, so we can't zero just the tail without storing it.
	// Controls that realloc (arrays) write every slot before reading, so plain realloc is safe here.
	return realloc(p, size);
}

extern "C" void uiprivFree(void *p)
{
	if (p == NULL)
		uiprivImplBug("attempt to uiprivFree(NULL)");
	free(p);
}

void uiprivInitAlloc(void) {}
void uiprivUninitAlloc(void) {}

extern "C" char *uiHaikuStrdupText(const char *s)
{
	return strdup(s);
}

// Frees strings handed back to the caller by libui (uiEntryText, uiOpenFile, ...). Those all come
// from uiHaikuStrdupText (strdup), so a plain free matches.
void uiFreeText(char *t)
{
	free(t);
}
