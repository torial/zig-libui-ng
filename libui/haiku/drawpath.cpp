// libui-ng Haiku backend — uiDrawPath (backed by a BShape).
// BShape supports move/line/bezier/close but has no arc primitive, so arcs are flattened into short
// line segments. libui arc angles are in radians, measured from +X, counterclockwise.
#include <math.h>
#include "uipriv_haiku.h"

uiDrawPath *uiDrawNewPath(uiDrawFillMode fillMode)
{
	uiDrawPath *p = uiprivNew(uiDrawPath);
	p->shape = new BShape();
	p->fillMode = fillMode;
	p->ended = false;
	p->inFigure = false;
	p->startX = p->startY = 0;
	p->curX = p->curY = 0;
	return p;
}

void uiDrawFreePath(uiDrawPath *p)
{
	delete p->shape;
	uiprivFree(p);
}

void uiDrawPathNewFigure(uiDrawPath *p, double x, double y)
{
	p->shape->MoveTo(BPoint(x, y));
	p->inFigure = true;
	p->startX = p->curX = x;
	p->startY = p->curY = y;
}

void uiDrawPathLineTo(uiDrawPath *p, double x, double y)
{
	p->shape->LineTo(BPoint(x, y));
	p->curX = x; p->curY = y;
}

void uiDrawPathBezierTo(uiDrawPath *p, double c1x, double c1y, double c2x, double c2y,
	double endX, double endY)
{
	BPoint pts[3] = { BPoint(c1x, c1y), BPoint(c2x, c2y), BPoint(endX, endY) };
	p->shape->BezierTo(pts);
	p->curX = endX; p->curY = endY;
}

void uiDrawPathCloseFigure(uiDrawPath *p)
{
	p->shape->Close();
	p->inFigure = false;
	p->curX = p->startX; p->curY = p->startY;
}

// Flatten an arc into line segments. If `moveFirst`, start a new figure at the arc's first point;
// otherwise draw a line from the current point to it (matching uiDrawPathArcTo semantics).
static void arc(uiDrawPath *p, double xc, double yc, double r, double start, double sweep,
	int negative, bool moveFirst)
{
	if (negative)
		sweep = -sweep;
	int steps = (int) (fabs(sweep) / (M_PI / 24.0)) + 1;	// ~7.5 degrees per segment
	if (steps < 1) steps = 1;
	for (int i = 0; i <= steps; i++) {
		double a = start + sweep * ((double) i / (double) steps);
		double x = xc + r * cos(a);
		double y = yc + r * sin(a);
		if (i == 0) {
			if (moveFirst)
				uiDrawPathNewFigure(p, x, y);
			else
				uiDrawPathLineTo(p, x, y);
		} else
			uiDrawPathLineTo(p, x, y);
	}
}

void uiDrawPathNewFigureWithArc(uiDrawPath *p, double xCenter, double yCenter, double radius,
	double startAngle, double sweep, int negative)
{
	arc(p, xCenter, yCenter, radius, startAngle, sweep, negative, true);
}

void uiDrawPathArcTo(uiDrawPath *p, double xCenter, double yCenter, double radius,
	double startAngle, double sweep, int negative)
{
	arc(p, xCenter, yCenter, radius, startAngle, sweep, negative, false);
}

void uiDrawPathAddRectangle(uiDrawPath *p, double x, double y, double width, double height)
{
	p->shape->MoveTo(BPoint(x, y));
	p->shape->LineTo(BPoint(x + width, y));
	p->shape->LineTo(BPoint(x + width, y + height));
	p->shape->LineTo(BPoint(x, y + height));
	p->shape->Close();
}

int uiDrawPathEnded(uiDrawPath *p) { return p->ended ? 1 : 0; }

void uiDrawPathEnd(uiDrawPath *p) { p->ended = true; }
