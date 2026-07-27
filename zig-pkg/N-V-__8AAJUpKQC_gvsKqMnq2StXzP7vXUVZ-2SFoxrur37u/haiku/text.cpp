// libui-ng Haiku backend — text helpers required by the common attributed-string code.
//   uiprivStricmp     — case-insensitive compare (used by attribute matching).
//   grapheme breaker  — common/attrstr.c needs a backend to map code points <-> grapheme clusters.
//
// This is a SIMPLE breaker: one code point == one grapheme. That is correct for Latin/precomposed
// text (which is what uiDrawText currently targets); combining marks and complex scripts would need
// a real Unicode grapheme algorithm (a future improvement, like the Pango-backed unix one).
#include <strings.h>
#include "uipriv_haiku.h"
extern "C" {
#include "../common/attrstr.h"
}

extern "C" int uiprivStricmp(const char *a, const char *b)
{
	return strcasecmp(a, b);
}

extern "C" int uiprivGraphemesTakesUTF16(void)
{
	return 0;	// we work in UTF-8 code points
}

extern "C" uiprivGraphemes *uiprivNewGraphemes(void *s, size_t len)
{
	(void) s;
	uiprivGraphemes *g = uiprivNew(uiprivGraphemes);
	g->len = len;
	g->pointsToGraphemes = (size_t *) uiprivAlloc((len + 1) * sizeof (size_t), "size_t[] (graphemes)");
	g->graphemesToPoints = (size_t *) uiprivAlloc((len + 1) * sizeof (size_t), "size_t[] (graphemes)");
	for (size_t i = 0; i <= len; i++) {
		g->pointsToGraphemes[i] = i;
		g->graphemesToPoints[i] = i;
	}
	return g;
}
