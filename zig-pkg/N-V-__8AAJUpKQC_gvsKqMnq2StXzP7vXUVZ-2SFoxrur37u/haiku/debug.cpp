// libui-ng Haiku backend — uiprivRealBug (the OS-specific half of common/debug.c).
#include <stdio.h>
#include <stdlib.h>
#include "uipriv_haiku.h"

extern "C" void uiprivRealBug(const char *file, const char *line, const char *func,
	const char *prefix, const char *format, va_list ap)
{
	fprintf(stderr, "[libui] %s:%s %s()\n", file, line, func);
	fprintf(stderr, "%s", prefix);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	abort();
}
