// 23 september 2026 -- tooltips and the clipboard on Haiku (torial fork): the minimum that
// links. Haiku support is being reconsidered (2026-09-23); nothing new is verified here.
#include "uipriv_haiku.h"
#include <Clipboard.h>
#include <String.h>
#include <stdlib.h>
#include <string.h>

void uiControlSetTooltip(uiControl *c, const char *text)
{
	BView *v = (BView *) uiControlHandle(c);

	if (v == NULL)
		return;
	v->SetToolTip((text == NULL || *text == '\0') ? NULL : text);
}

char *uiClipboardText(void)
{
	const char *text = NULL;
	ssize_t len = 0;
	char *out = NULL;
	BMessage *clip;

	if (!be_clipboard->Lock())
		return NULL;
	clip = be_clipboard->Data();
	if (clip != NULL && clip->FindData("text/plain", B_MIME_TYPE, (const void **) &text, &len) == B_OK && text != NULL) {
		out = (char *) malloc(len + 1);		// uiFreeText() is free() here
		memcpy(out, text, len);
		out[len] = '\0';
	}
	be_clipboard->Unlock();
	return out;
}

void uiClipboardSetText(const char *text)
{
	BMessage *clip;

	if (text == NULL)
		text = "";
	if (!be_clipboard->Lock())
		return;
	be_clipboard->Clear();
	clip = be_clipboard->Data();
	if (clip != NULL) {
		clip->AddData("text/plain", B_MIME_TYPE, text, strlen(text));
		be_clipboard->Commit();
	}
	be_clipboard->Unlock();
}
