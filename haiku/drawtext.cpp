// libui-ng Haiku backend — uiDrawText (text rendering inside uiArea).
// SCOPE: renders the attributed string's plain text with the layout's default font, word-wrapped to
// the layout width, horizontally aligned, in black. Per-run attributes (color, per-span weight/
// italic/size, underline) are not applied yet — that needs walking the attribute runs and is a
// follow-up. Family/size/bold/italic from the default font ARE honored.
#include <string.h>
#include "uipriv_haiku.h"

struct uiDrawTextLayout {
	char *text;
	uiFontDescriptor font;	// Family strdup'd
	double width;
	int align;
};

uiDrawTextLayout *uiDrawNewTextLayout(uiDrawTextLayoutParams *p)
{
	uiDrawTextLayout *tl = uiprivNew(uiDrawTextLayout);
	tl->text = uiHaikuStrdupText(uiAttributedStringString(p->String));
	tl->font.Family = uiHaikuStrdupText(p->DefaultFont->Family ? p->DefaultFont->Family : "");
	tl->font.Size = p->DefaultFont->Size;
	tl->font.Weight = p->DefaultFont->Weight;
	tl->font.Italic = p->DefaultFont->Italic;
	tl->font.Stretch = p->DefaultFont->Stretch;
	tl->width = p->Width;
	tl->align = p->Align;
	return tl;
}

void uiDrawFreeTextLayout(uiDrawTextLayout *tl)
{
	uiprivFree(tl->text);
	uiprivFree(tl->font.Family);
	uiprivFree(tl);
}

static BFont makeFont(const uiFontDescriptor *d)
{
	BFont f(be_plain_font);
	if (d->Family != NULL && d->Family[0] != '\0')
		f.SetFamilyAndStyle(d->Family, NULL);
	f.SetSize(d->Size);
	uint16 face = 0;
	if (d->Weight >= 700) face |= B_BOLD_FACE;
	if (d->Italic != uiTextItalicNormal) face |= B_ITALIC_FACE;
	if (face == 0) face = B_REGULAR_FACE;
	f.SetFace(face);
	return f;
}

// Word-wrap tl->text into `lines` (BString*); returns the widest line's width. width<=0 => no wrap.
static float computeLines(uiDrawTextLayout *tl, BFont &f, BList &lines)
{
	float maxw = 0;
	BString cur, word;
	const char *s = tl->text;
	size_t n = strlen(s);
	for (size_t i = 0; i <= n; i++) {
		char ch = (i < n) ? s[i] : '\0';
		if (ch == ' ' || ch == '\n' || ch == '\0') {
			if (word.Length() > 0) {
				BString cand = cur;
				if (cand.Length() > 0) cand << " ";
				cand << word;
				if (tl->width > 0 && cur.Length() > 0
					&& f.StringWidth(cand.String()) > tl->width) {
					float w = f.StringWidth(cur.String());
					if (w > maxw) maxw = w;
					lines.AddItem(new BString(cur));
					cur = word;
				} else
					cur = cand;
				word = "";
			}
			if (ch == '\n' || ch == '\0') {
				float w = f.StringWidth(cur.String());
				if (w > maxw) maxw = w;
				lines.AddItem(new BString(cur));
				cur = "";
			}
		} else
			word << ch;
	}
	return maxw;
}

static void freeLines(BList &lines)
{
	for (int32 i = 0; i < lines.CountItems(); i++)
		delete (BString *) lines.ItemAt(i);
}

void uiDrawText(uiDrawContext *c, uiDrawTextLayout *tl, double x, double y)
{
	BView *v = c->view;
	BFont f = makeFont(&tl->font);

	BList lines;
	computeLines(tl, f, lines);

	font_height fh;
	f.GetHeight(&fh);
	float lineH = fh.ascent + fh.descent + fh.leading;
	float baseline = (float) y + fh.ascent;

	v->PushState();
	v->SetFont(&f);
	v->SetHighColor(make_color(0, 0, 0, 255));
	for (int32 i = 0; i < lines.CountItems(); i++) {
		BString *line = (BString *) lines.ItemAt(i);
		float lw = f.StringWidth(line->String());
		float off = 0;
		if (tl->align == uiDrawTextAlignCenter) off = (tl->width - lw) / 2;
		else if (tl->align == uiDrawTextAlignRight) off = tl->width - lw;
		if (off < 0) off = 0;
		v->DrawString(line->String(), BPoint((float) x + off, baseline));
		baseline += lineH;
	}
	v->PopState();

	freeLines(lines);
}

void uiDrawTextLayoutExtents(uiDrawTextLayout *tl, double *width, double *height)
{
	BFont f = makeFont(&tl->font);
	BList lines;
	float maxw = computeLines(tl, f, lines);
	font_height fh;
	f.GetHeight(&fh);
	float lineH = fh.ascent + fh.descent + fh.leading;
	*width = maxw;
	*height = lineH * lines.CountItems();
	freeLines(lines);
}
