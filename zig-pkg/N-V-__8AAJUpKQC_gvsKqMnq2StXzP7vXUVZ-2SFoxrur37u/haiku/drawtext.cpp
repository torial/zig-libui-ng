// libui-ng Haiku backend — uiDrawText with per-run attributes.
//
// At layout creation we flatten the attributed string's attribute runs into a per-byte style array
// (family/size/weight/italic/color/underline overrides over the default font). Drawing word-wraps to
// the layout width, then renders each line as a sequence of same-style segments with the right BFont
// and color (and an underline rule when set). Attribute positions are UTF-8 byte offsets.
//
// Limitations: word wrapping measures each word in the font of its first byte (words rarely cross a
// style boundary); background/underline-color/stretch/features attributes aren't applied; the simple
// grapheme model (text.cpp) means complex scripts aren't shaped.
#include <string.h>
#include "uipriv_haiku.h"

struct charStyle {
	const char *family;	// NULL = default
	double size;		// < 0 = default
	int weight;		// < 0 = default
	int italic;		// < 0 = default
	bool hasColor;
	rgb_color color;
	int underline;		// uiUnderlineNone = none
	bool hasBg;
	rgb_color bg;
	bool hasUlColor;
	rgb_color ulColor;
};

struct uiDrawTextLayout {
	char *text;
	size_t len;
	uiFontDescriptor font;	// default font (Family strdup'd)
	double width;
	int align;
	charStyle *styles;	// [len], or NULL when len == 0
	BList *ownedFamilies;	// char* strdup'd from family attributes
};

struct lineRange { size_t start, end; };	// byte range [start, end)

static uiForEach applyAttr(const uiAttributedString *s, const uiAttribute *a,
	size_t start, size_t end, void *data)
{
	uiDrawTextLayout *tl = (uiDrawTextLayout *) data;
	(void) s;
	if (end > tl->len)
		end = tl->len;
	uiAttributeType t = uiAttributeGetType(a);

	if (t == uiAttributeTypeFamily) {
		char *fam = uiHaikuStrdupText(uiAttributeFamily(a));
		tl->ownedFamilies->AddItem(fam);
		for (size_t i = start; i < end; i++)
			tl->styles[i].family = fam;
		return uiForEachContinue;
	}

	for (size_t i = start; i < end; i++) {
		charStyle *cs = &tl->styles[i];
		switch (t) {
		case uiAttributeTypeSize:
			cs->size = uiAttributeSize(a);
			break;
		case uiAttributeTypeWeight:
			cs->weight = uiAttributeWeight(a);
			break;
		case uiAttributeTypeItalic:
			cs->italic = uiAttributeItalic(a);
			break;
		case uiAttributeTypeColor: {
			double r, g, b, al;
			uiAttributeColor(a, &r, &g, &b, &al);
			cs->hasColor = true;
			cs->color = make_color((uint8) (r * 255), (uint8) (g * 255),
				(uint8) (b * 255), (uint8) (al * 255));
			break;
		}
		case uiAttributeTypeUnderline:
			cs->underline = uiAttributeUnderline(a);
			break;
		case uiAttributeTypeBackground: {
			double r, g, b, al;
			uiAttributeColor(a, &r, &g, &b, &al);
			cs->hasBg = true;
			cs->bg = make_color((uint8) (r * 255), (uint8) (g * 255),
				(uint8) (b * 255), (uint8) (al * 255));
			break;
		}
		case uiAttributeTypeUnderlineColor: {
			uiUnderlineColor u;
			double r, g, b, al;
			uiAttributeUnderlineColor(a, &u, &r, &g, &b, &al);
			cs->hasUlColor = true;
			if (u == uiUnderlineColorCustom)
				cs->ulColor = make_color((uint8) (r * 255), (uint8) (g * 255),
					(uint8) (b * 255), (uint8) (al * 255));
			else if (u == uiUnderlineColorSpelling)
				cs->ulColor = make_color(220, 40, 40, 255);
			else if (u == uiUnderlineColorGrammar)
				cs->ulColor = make_color(40, 160, 40, 255);
			else
				cs->ulColor = make_color(40, 80, 220, 255);
			break;
		}
		default:
			break;	// stretch / OpenType features not applied (no BFont equivalent)
		}
	}
	return uiForEachContinue;
}

uiDrawTextLayout *uiDrawNewTextLayout(uiDrawTextLayoutParams *p)
{
	uiDrawTextLayout *tl = uiprivNew(uiDrawTextLayout);
	tl->text = uiHaikuStrdupText(uiAttributedStringString(p->String));
	tl->len = strlen(tl->text);
	tl->font.Family = uiHaikuStrdupText(p->DefaultFont->Family ? p->DefaultFont->Family : "");
	tl->font.Size = p->DefaultFont->Size;
	tl->font.Weight = p->DefaultFont->Weight;
	tl->font.Italic = p->DefaultFont->Italic;
	tl->font.Stretch = p->DefaultFont->Stretch;
	tl->width = p->Width;
	tl->align = p->Align;
	tl->ownedFamilies = new BList();

	tl->styles = NULL;
	if (tl->len > 0) {
		tl->styles = (charStyle *) uiprivAlloc(tl->len * sizeof (charStyle), "charStyle[]");
		for (size_t i = 0; i < tl->len; i++) {
			tl->styles[i].family = NULL;
			tl->styles[i].size = -1;
			tl->styles[i].weight = -1;
			tl->styles[i].italic = -1;
			tl->styles[i].hasColor = false;
			tl->styles[i].underline = uiUnderlineNone;
			tl->styles[i].hasBg = false;
			tl->styles[i].hasUlColor = false;
		}
		uiAttributedStringForEachAttribute(p->String, applyAttr, tl);
	}
	return tl;
}

void uiDrawFreeTextLayout(uiDrawTextLayout *tl)
{
	for (int32 i = 0; i < tl->ownedFamilies->CountItems(); i++)
		uiprivFree(tl->ownedFamilies->ItemAt(i));
	delete tl->ownedFamilies;
	if (tl->styles != NULL)
		uiprivFree(tl->styles);
	uiprivFree(tl->font.Family);
	uiprivFree(tl->text);
	uiprivFree(tl);
}

static BFont fontAt(uiDrawTextLayout *tl, size_t i)
{
	const char *fam = tl->font.Family;
	double size = tl->font.Size;
	int weight = tl->font.Weight;
	int italic = tl->font.Italic;
	if (tl->styles != NULL && i < tl->len) {
		charStyle *cs = &tl->styles[i];
		if (cs->family != NULL) fam = cs->family;
		if (cs->size > 0) size = cs->size;
		if (cs->weight >= 0) weight = cs->weight;
		if (cs->italic >= 0) italic = cs->italic;
	}
	BFont f(be_plain_font);
	if (fam != NULL && fam[0] != '\0')
		f.SetFamilyAndStyle(fam, NULL);
	f.SetSize(size);
	uint16 face = 0;
	if (weight >= 700) face |= B_BOLD_FACE;
	if (italic != uiTextItalicNormal) face |= B_ITALIC_FACE;
	if (face == 0) face = B_REGULAR_FACE;
	f.SetFace(face);
	return f;
}

static rgb_color colorAt(uiDrawTextLayout *tl, size_t i)
{
	if (tl->styles != NULL && i < tl->len && tl->styles[i].hasColor)
		return tl->styles[i].color;
	return make_color(0, 0, 0, 255);
}

static bool underlineAt(uiDrawTextLayout *tl, size_t i)
{
	return tl->styles != NULL && i < tl->len && tl->styles[i].underline != uiUnderlineNone;
}

static bool sameStyle(uiDrawTextLayout *tl, size_t a, size_t b)
{
	if (tl->styles == NULL)
		return true;
	charStyle *x = &tl->styles[a], *y = &tl->styles[b];
	if (x->family != y->family || x->size != y->size || x->weight != y->weight
		|| x->italic != y->italic || x->hasColor != y->hasColor || x->underline != y->underline
		|| x->hasBg != y->hasBg || x->hasUlColor != y->hasUlColor)
		return false;
	if (x->hasColor && (x->color.red != y->color.red || x->color.green != y->color.green
		|| x->color.blue != y->color.blue || x->color.alpha != y->color.alpha))
		return false;
	if (x->hasBg && (x->bg.red != y->bg.red || x->bg.green != y->bg.green
		|| x->bg.blue != y->bg.blue || x->bg.alpha != y->bg.alpha))
		return false;
	if (x->hasUlColor && (x->ulColor.red != y->ulColor.red || x->ulColor.green != y->ulColor.green
		|| x->ulColor.blue != y->ulColor.blue || x->ulColor.alpha != y->ulColor.alpha))
		return false;
	return true;
}

static void computeLineRanges(uiDrawTextLayout *tl, BList &lines)
{
	const char *s = tl->text;
	size_t len = tl->len;
	size_t lineStart = 0, lineEnd = 0;
	bool haveLine = false;
	float lineW = 0;
	size_t i = 0;

	while (i <= len) {
		if (i < len && s[i] == '\n') {
			lineRange *lr = new lineRange;
			lr->start = haveLine ? lineStart : i;
			lr->end = haveLine ? lineEnd : i;
			lines.AddItem(lr);
			haveLine = false; lineW = 0;
			i++; lineStart = i;
			continue;
		}
		if (i < len && s[i] == ' ') { i++; continue; }
		if (i >= len) break;

		size_t ws = i;
		while (i < len && s[i] != ' ' && s[i] != '\n') i++;
		size_t we = i;

		BFont wf = fontAt(tl, ws);
		float ww = wf.StringWidth(s + ws, (int32) (we - ws));
		if (!haveLine) {
			lineStart = ws; lineEnd = we; lineW = ww; haveLine = true;
		} else {
			float sw = wf.StringWidth(" ", 1);
			if (tl->width > 0 && lineW + sw + ww > tl->width) {
				lineRange *lr = new lineRange;
				lr->start = lineStart; lr->end = lineEnd;
				lines.AddItem(lr);
				lineStart = ws; lineEnd = we; lineW = ww;
			} else {
				lineEnd = we; lineW += sw + ww;
			}
		}
	}
	if (haveLine) {
		lineRange *lr = new lineRange;
		lr->start = lineStart; lr->end = lineEnd;
		lines.AddItem(lr);
	} else if (lines.CountItems() == 0) {
		lineRange *lr = new lineRange;
		lr->start = 0; lr->end = 0;
		lines.AddItem(lr);
	}
}

static void freeLines(BList &lines)
{
	for (int32 i = 0; i < lines.CountItems(); i++)
		delete (lineRange *) lines.ItemAt(i);
}

static void lineMetrics(uiDrawTextLayout *tl, lineRange *lr, float *ascent, float *height)
{
	if (lr->start >= lr->end) {
		BFont f = fontAt(tl, lr->start < tl->len ? lr->start : 0);
		font_height fh; f.GetHeight(&fh);
		*ascent = fh.ascent;
		*height = fh.ascent + fh.descent + fh.leading;
		return;
	}
	float maxAsc = 0, maxH = 0;
	size_t i = lr->start;
	while (i < lr->end) {
		size_t k = i + 1;
		while (k < lr->end && sameStyle(tl, i, k)) k++;
		BFont f = fontAt(tl, i);
		font_height fh; f.GetHeight(&fh);
		if (fh.ascent > maxAsc) maxAsc = fh.ascent;
		float h = fh.ascent + fh.descent + fh.leading;
		if (h > maxH) maxH = h;
		i = k;
	}
	*ascent = maxAsc;
	*height = maxH;
}

static float lineWidth(uiDrawTextLayout *tl, lineRange *lr)
{
	float w = 0;
	size_t i = lr->start;
	while (i < lr->end) {
		size_t k = i + 1;
		while (k < lr->end && sameStyle(tl, i, k)) k++;
		BFont f = fontAt(tl, i);
		w += f.StringWidth(tl->text + i, (int32) (k - i));
		i = k;
	}
	return w;
}

void uiDrawText(uiDrawContext *c, uiDrawTextLayout *tl, double x, double y)
{
	BView *v = c->view;
	BList lines;
	computeLineRanges(tl, lines);

	v->PushState();
	float penY = (float) y;
	for (int32 li = 0; li < lines.CountItems(); li++) {
		lineRange *lr = (lineRange *) lines.ItemAt(li);
		float asc, h;
		lineMetrics(tl, lr, &asc, &h);
		float lw = lineWidth(tl, lr);
		float off = 0;
		if (tl->align == uiDrawTextAlignCenter) off = (tl->width - lw) / 2;
		else if (tl->align == uiDrawTextAlignRight) off = tl->width - lw;
		if (off < 0) off = 0;

		float baseline = penY + asc;
		float penX = (float) x + off;
		size_t i = lr->start;
		while (i < lr->end) {
			size_t k = i + 1;
			while (k < lr->end && sameStyle(tl, i, k)) k++;
			BFont f = fontAt(tl, i);
			v->SetFont(&f);
			float segW = f.StringWidth(tl->text + i, (int32) (k - i));
			charStyle *cs = (tl->styles != NULL && i < tl->len) ? &tl->styles[i] : NULL;
			// background fill behind the glyphs
			if (cs != NULL && cs->hasBg) {
				font_height fh;
				f.GetHeight(&fh);
				v->SetHighColor(cs->bg);
				v->FillRect(BRect(penX, baseline - fh.ascent, penX + segW, baseline + fh.descent));
			}
			v->SetHighColor(colorAt(tl, i));
			v->DrawString(tl->text + i, (int32) (k - i), BPoint(penX, baseline));
			if (underlineAt(tl, i)) {
				v->SetHighColor((cs != NULL && cs->hasUlColor) ? cs->ulColor : colorAt(tl, i));
				v->StrokeLine(BPoint(penX, baseline + 2), BPoint(penX + segW, baseline + 2));
			}
			penX += segW;
			i = k;
		}
		penY += h;
	}
	v->PopState();
	freeLines(lines);
}

void uiDrawTextLayoutExtents(uiDrawTextLayout *tl, double *width, double *height)
{
	BList lines;
	computeLineRanges(tl, lines);
	float maxw = 0, total = 0;
	for (int32 li = 0; li < lines.CountItems(); li++) {
		lineRange *lr = (lineRange *) lines.ItemAt(li);
		float asc, h;
		lineMetrics(tl, lr, &asc, &h);
		float lw = lineWidth(tl, lr);
		if (lw > maxw) maxw = lw;
		total += h;
	}
	*width = maxw;
	*height = total;
	freeLines(lines);
}
