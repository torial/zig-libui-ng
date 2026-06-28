// libui-ng Haiku backend — uiHaikuControl base (mirrors unix/control.c).
#include "uipriv_haiku.h"

// 'Haik' — the OS signature stamped into every Haiku-backed uiControl.
#define uiHaikuControlSignature 0x4861696B

extern "C" uiHaikuControl *uiHaikuAllocControl(size_t n, uint32_t typesig, const char *typenamestr)
{
	return uiHaikuControl(uiAllocControl(n, uiHaikuControlSignature, typesig, typenamestr));
}

extern "C" void uiHaikuControlSetContainer(uiHaikuControl *c, BView *parent, int remove)
{
	(*(c->SetContainer))(c, parent, remove);
}

BMessage *uiprivNewEventMessage(uiprivEventFn fn, void *control)
{
	BMessage *m = new BMessage(uiprivMsgControlEvent);
	m->AddPointer("fn", (void *) fn);
	m->AddPointer("control", control);
	return m;
}
