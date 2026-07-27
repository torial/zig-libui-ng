// libui-ng on Haiku — text rendering inside a uiArea via uiDrawText: a large title, a wrapped
// paragraph, and center/right-aligned lines.
#include <stdio.h>
#include <string.h>
#include "../../ui.h"

static uiFontDescriptor titleFont;
static uiFontDescriptor bodyFont;

static void drawText(uiDrawContext *ctx, const char *text, uiFontDescriptor *font,
	double x, double y, double width, uiDrawTextAlign align)
{
	uiAttributedString *as = uiNewAttributedString(text);
	uiDrawTextLayoutParams p;
	p.String = as;
	p.DefaultFont = font;
	p.Width = width;
	p.Align = align;
	uiDrawTextLayout *tl = uiDrawNewTextLayout(&p);
	uiDrawText(ctx, tl, x, y);
	uiDrawFreeTextLayout(tl);
	uiFreeAttributedString(as);
}

// Demonstrates per-run attributes: color / bold / italic / underline / larger size spans.
static void drawStyled(uiDrawContext *ctx, double x, double y, double width)
{
	//                0   4    9      16        26    32
	const char *txt = "Red bold italic underline big - styled per run.";
	uiAttributedString *as = uiNewAttributedString(txt);
	uiAttributedStringSetAttribute(as, uiNewColorAttribute(0.85, 0.10, 0.10, 1.0), 0, 3);   // Red
	uiAttributedStringSetAttribute(as, uiNewWeightAttribute((uiTextWeight) 700), 4, 8);      // bold
	uiAttributedStringSetAttribute(as, uiNewItalicAttribute(uiTextItalicItalic), 9, 15);     // italic
	uiAttributedStringSetAttribute(as, uiNewUnderlineAttribute(uiUnderlineSingle), 16, 25);  // underline
	uiAttributedStringSetAttribute(as,
		uiNewUnderlineColorAttribute(uiUnderlineColorCustom, 0.85, 0.1, 0.1, 1.0), 16, 25);  // ...in red
	uiAttributedStringSetAttribute(as, uiNewSizeAttribute(24), 26, 29);                      // big
	uiAttributedStringSetAttribute(as, uiNewColorAttribute(0.1, 0.4, 0.85, 1.0), 26, 29);    // big in blue
	uiAttributedStringSetAttribute(as, uiNewBackgroundAttribute(1.0, 0.93, 0.3, 1.0), 32, 38); // "styled" highlight

	uiDrawTextLayoutParams p;
	p.String = as;
	p.DefaultFont = &bodyFont;
	p.Width = width;
	p.Align = uiDrawTextAlignLeft;
	uiDrawTextLayout *tl = uiDrawNewTextLayout(&p);
	uiDrawText(ctx, tl, x, y);
	uiDrawFreeTextLayout(tl);
	uiFreeAttributedString(as);
}

static void hDraw(uiAreaHandler *ah, uiArea *area, uiAreaDrawParams *p)
{
	uiDrawContext *ctx = p->Context;
	(void) ah; (void) area;

	drawText(ctx, "Drawing text on Haiku", &titleFont, 16, 14, 380, uiDrawTextAlignLeft);
	drawText(ctx,
		"This paragraph is rendered by uiDrawText: the attributed string's text is laid out "
		"with the default font and word-wrapped to the layout width, then drawn line by line.",
		&bodyFont, 16, 52, 360, uiDrawTextAlignLeft);
	drawStyled(ctx, 16, 150, 370);
	drawText(ctx, "Centered line", &bodyFont, 16, 200, 360, uiDrawTextAlignCenter);
	drawText(ctx, "Right-aligned line", &bodyFont, 16, 222, 360, uiDrawTextAlignRight);
}

static void hMouse(uiAreaHandler *ah, uiArea *a, uiAreaMouseEvent *e) { (void) ah; (void) a; (void) e; }
static void hCrossed(uiAreaHandler *ah, uiArea *a, int left) { (void) ah; (void) a; (void) left; }
static void hDragBroken(uiAreaHandler *ah, uiArea *a) { (void) ah; (void) a; }
static int hKey(uiAreaHandler *ah, uiArea *a, uiAreaKeyEvent *e) { (void) ah; (void) a; (void) e; return 0; }

static uiAreaHandler handler = { hDraw, hMouse, hCrossed, hDragBroken, hKey };

static int onClosing(uiWindow *w, void *data)
{
	(void) w; (void) data;
	uiQuit();
	return 1;
}

int main(void)
{
	uiInitOptions o;
	const char *err;
	uiWindow *w;
	uiArea *area;

	memset(&o, 0, sizeof o);
	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "error initializing libui: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	uiLoadControlFont(&bodyFont);
	uiLoadControlFont(&titleFont);
	titleFont.Size = 22;
	titleFont.Weight = 700;		// bold

	w = uiNewWindow("libui-ng on Haiku — uiDrawText", 400, 270, 0);
	uiWindowOnClosing(w, onClosing, NULL);

	area = uiNewArea(&handler);
	uiWindowSetChild(w, uiControl(area));
	uiControlShow(uiControl(w));

	uiMain();
	uiFreeFontDescriptor(&bodyFont);
	uiFreeFontDescriptor(&titleFont);
	uiUninit();
	return 0;
}
