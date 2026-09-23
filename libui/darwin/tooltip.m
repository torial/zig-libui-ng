// 23 september 2026 -- tooltips and the clipboard on macOS (torial fork). WRITTEN BLIND.
#import "uipriv_darwin.h"

void uiControlSetTooltip(uiControl *c, const char *text)
{
	NSView *view = (NSView *) uiControlHandle(c);

	if (text == NULL || *text == '\0')
		[view setToolTip:nil];
	else
		[view setToolTip:[NSString stringWithUTF8String:text]];
}

char *uiClipboardText(void)
{
	NSPasteboard *pb = [NSPasteboard generalPasteboard];
	NSString *s;

	s = [pb stringForType:NSPasteboardTypeString];
	if (s == nil)
		return NULL;
	return uiDarwinNSStringToText(s);
}

void uiClipboardSetText(const char *text)
{
	NSPasteboard *pb = [NSPasteboard generalPasteboard];

	[pb clearContents];
	[pb setString:[NSString stringWithUTF8String:(text == NULL ? "" : text)] forType:NSPasteboardTypeString];
}
