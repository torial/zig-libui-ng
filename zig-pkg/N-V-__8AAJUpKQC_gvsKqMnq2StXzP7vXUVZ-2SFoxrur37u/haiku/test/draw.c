// libui-ng on Haiku — uiArea drawing: solid fill, stroked path with round caps/joins, a
// gradient-filled circle (flattened arc), and a rotated (transformed) rectangle.
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../../ui.h"

static void hDraw(uiAreaHandler *ah, uiArea *area, uiAreaDrawParams *p)
{
	uiDrawContext *ctx = p->Context;
	uiDrawPath *path;
	uiDrawBrush b;
	uiDrawStrokeParams sp;

	(void) ah; (void) area;

	// 1) solid-filled rectangle
	path = uiDrawNewPath(uiDrawFillModeWinding);
	uiDrawPathAddRectangle(path, 20, 20, 110, 60);
	uiDrawPathEnd(path);
	memset(&b, 0, sizeof b);
	b.Type = uiDrawBrushTypeSolid; b.R = 0.20; b.G = 0.50; b.B = 0.90; b.A = 1.0;
	uiDrawFill(ctx, path, &b);
	uiDrawFreePath(path);

	// 2) stroked triangle, round caps/joins
	path = uiDrawNewPath(uiDrawFillModeWinding);
	uiDrawPathNewFigure(path, 170, 20);
	uiDrawPathLineTo(path, 280, 25);
	uiDrawPathLineTo(path, 225, 90);
	uiDrawPathCloseFigure(path);
	uiDrawPathEnd(path);
	memset(&sp, 0, sizeof sp);
	sp.Cap = uiDrawLineCapRound; sp.Join = uiDrawLineJoinRound;
	sp.Thickness = 4.0; sp.MiterLimit = uiDrawDefaultMiterLimit;
	memset(&b, 0, sizeof b);
	b.Type = uiDrawBrushTypeSolid; b.R = 0.85; b.G = 0.15; b.B = 0.15; b.A = 1.0;
	uiDrawStroke(ctx, path, &b, &sp);
	uiDrawFreePath(path);

	// 3) gradient-filled circle (full-sweep arc)
	path = uiDrawNewPath(uiDrawFillModeWinding);
	uiDrawPathNewFigureWithArc(path, 80, 190, 55, 0, 2 * M_PI, 0);
	uiDrawPathCloseFigure(path);
	uiDrawPathEnd(path);
	memset(&b, 0, sizeof b);
	b.Type = uiDrawBrushTypeLinearGradient;
	b.X0 = 25; b.Y0 = 135; b.X1 = 135; b.Y1 = 245;
	uiDrawBrushGradientStop stops[2];
	stops[0].Pos = 0.0; stops[0].R = 1.0; stops[0].G = 0.9; stops[0].B = 0.1; stops[0].A = 1.0;
	stops[1].Pos = 1.0; stops[1].R = 0.9; stops[1].G = 0.1; stops[1].B = 0.5; stops[1].A = 1.0;
	b.Stops = stops; b.NumStops = 2;
	uiDrawFill(ctx, path, &b);
	uiDrawFreePath(path);

	// 4) rotated (transformed) filled rectangle
	uiDrawSave(ctx);
	uiDrawMatrix m;
	uiDrawMatrixSetIdentity(&m);
	uiDrawMatrixRotate(&m, 230, 190, 0.5);
	uiDrawTransform(ctx, &m);
	path = uiDrawNewPath(uiDrawFillModeWinding);
	uiDrawPathAddRectangle(path, 185, 155, 90, 70);
	uiDrawPathEnd(path);
	memset(&b, 0, sizeof b);
	b.Type = uiDrawBrushTypeSolid; b.R = 0.20; b.G = 0.70; b.B = 0.35; b.A = 1.0;
	uiDrawFill(ctx, path, &b);
	uiDrawFreePath(path);
	uiDrawRestore(ctx);
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

	w = uiNewWindow("libui-ng on Haiku — uiArea drawing", 320, 280, 0);
	uiWindowOnClosing(w, onClosing, NULL);

	area = uiNewArea(&handler);
	uiWindowSetChild(w, uiControl(area));
	uiControlShow(uiControl(w));

	uiMain();
	uiUninit();
	return 0;
}
