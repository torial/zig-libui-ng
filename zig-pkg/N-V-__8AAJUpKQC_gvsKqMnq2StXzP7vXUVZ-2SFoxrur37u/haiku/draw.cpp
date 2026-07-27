// libui-ng Haiku backend — drawing operations over a BView (the uiDrawContext).
// All of these run inside uiArea's BView::Draw(). Paths are BShapes; the BView's own graphics state
// (transform, clip, colors, pen) provides save/restore via PushState/PopState.
#include "uipriv_haiku.h"

static rgb_color colorOf(double r, double g, double b, double a)
{
	rgb_color c;
	c.red   = (uint8) (r * 255.0 + 0.5);
	c.green = (uint8) (g * 255.0 + 0.5);
	c.blue  = (uint8) (b * 255.0 + 0.5);
	c.alpha = (uint8) (a * 255.0 + 0.5);
	return c;
}

static void addStops(BGradient &grad, uiDrawBrush *b)
{
	for (size_t i = 0; i < b->NumStops; i++) {
		uiDrawBrushGradientStop *s = &b->Stops[i];
		grad.AddColor(colorOf(s->R, s->G, s->B, s->A), (float) (s->Pos * 255.0));
	}
}

void uiDrawStroke(uiDrawContext *c, uiDrawPath *path, uiDrawBrush *b, uiDrawStrokeParams *p)
{
	BView *v = c->view;

	cap_mode cap = B_BUTT_CAP;
	if (p->Cap == uiDrawLineCapRound) cap = B_ROUND_CAP;
	else if (p->Cap == uiDrawLineCapSquare) cap = B_SQUARE_CAP;

	join_mode join = B_MITER_JOIN;
	if (p->Join == uiDrawLineJoinRound) join = B_ROUND_JOIN;
	else if (p->Join == uiDrawLineJoinBevel) join = B_BEVEL_JOIN;

	v->PushState();
	v->SetPenSize((float) p->Thickness);
	v->SetLineMode(cap, join, (float) p->MiterLimit);
	// Stroking is solid-color only (gradient strokes aren't exposed by BView); use the brush color.
	v->SetHighColor(colorOf(b->R, b->G, b->B, b->A));
	v->StrokeShape(path->shape);
	v->PopState();
}

void uiDrawFill(uiDrawContext *c, uiDrawPath *path, uiDrawBrush *b)
{
	BView *v = c->view;

	v->PushState();
	switch (b->Type) {
	case uiDrawBrushTypeLinearGradient: {
		BGradientLinear grad(BPoint(b->X0, b->Y0), BPoint(b->X1, b->Y1));
		addStops(grad, b);
		v->FillShape(path->shape, grad);
		break;
	}
	case uiDrawBrushTypeRadialGradient: {
		BGradientRadial grad(BPoint(b->X1, b->Y1), (float) b->OuterRadius);
		addStops(grad, b);
		v->FillShape(path->shape, grad);
		break;
	}
	default:	// solid (and unsupported image brushes fall back to solid)
		v->SetHighColor(colorOf(b->R, b->G, b->B, b->A));
		v->FillShape(path->shape);
		break;
	}
	v->PopState();
}

void uiDrawTransform(uiDrawContext *c, uiDrawMatrix *m)
{
	// Compose m with the view's current transform (m applied first, in user space).
	BAffineTransform incoming(m->M11, m->M12, m->M21, m->M22, m->M31, m->M32);
	BAffineTransform cur = c->view->Transform();
	cur.PreMultiply(incoming);
	c->view->SetTransform(cur);
}

void uiDrawClip(uiDrawContext *c, uiDrawPath *path)
{
	c->view->ClipToShape(path->shape);
}

void uiDrawSave(uiDrawContext *c)
{
	c->view->PushState();
}

void uiDrawRestore(uiDrawContext *c)
{
	c->view->PopState();
}
