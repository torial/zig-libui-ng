// libui-ng Haiku backend — uiDrawMatrix (2D affine matrix math).
// Convention (row-vector, matching the public struct): a point transforms as
//   x' = x*M11 + y*M21 + M31,  y' = x*M12 + y*M22 + M32.
// Composition p * (A*B) applies A first then B; the *Translate/Scale/Rotate/Skew helpers compose
// the new operation BEFORE the existing matrix (cairo-style "in the current user space").
#include <math.h>
#include "uipriv_haiku.h"

static void setAll(uiDrawMatrix *m, double a, double b, double c, double d, double e, double f)
{
	m->M11 = a; m->M12 = b; m->M21 = c; m->M22 = d; m->M31 = e; m->M32 = f;
}

// dest = a * b  (apply a first, then b)
static uiDrawMatrix mul(const uiDrawMatrix *a, const uiDrawMatrix *b)
{
	uiDrawMatrix r;
	r.M11 = a->M11 * b->M11 + a->M12 * b->M21;
	r.M12 = a->M11 * b->M12 + a->M12 * b->M22;
	r.M21 = a->M21 * b->M11 + a->M22 * b->M21;
	r.M22 = a->M21 * b->M12 + a->M22 * b->M22;
	r.M31 = a->M31 * b->M11 + a->M32 * b->M21 + b->M31;
	r.M32 = a->M31 * b->M12 + a->M32 * b->M22 + b->M32;
	return r;
}

// m = op * m  (op applied first in the current user space)
static void composeBefore(uiDrawMatrix *m, const uiDrawMatrix *op)
{
	uiDrawMatrix r = mul(op, m);
	*m = r;
}

void uiDrawMatrixSetIdentity(uiDrawMatrix *m) { setAll(m, 1, 0, 0, 1, 0, 0); }

void uiDrawMatrixTranslate(uiDrawMatrix *m, double x, double y)
{
	uiDrawMatrix t; setAll(&t, 1, 0, 0, 1, x, y);
	composeBefore(m, &t);
}

void uiDrawMatrixScale(uiDrawMatrix *m, double xCenter, double yCenter, double x, double y)
{
	uiDrawMatrix tneg, s, tpos;
	setAll(&tneg, 1, 0, 0, 1, -xCenter, -yCenter);
	setAll(&s, x, 0, 0, y, 0, 0);
	setAll(&tpos, 1, 0, 0, 1, xCenter, yCenter);
	uiDrawMatrix op = mul(&tneg, &s);
	op = mul(&op, &tpos);
	composeBefore(m, &op);
}

void uiDrawMatrixRotate(uiDrawMatrix *m, double x, double y, double amount)
{
	double c = cos(amount), s = sin(amount);
	uiDrawMatrix tneg, rot, tpos;
	setAll(&tneg, 1, 0, 0, 1, -x, -y);
	setAll(&rot, c, s, -s, c, 0, 0);
	setAll(&tpos, 1, 0, 0, 1, x, y);
	uiDrawMatrix op = mul(&tneg, &rot);
	op = mul(&op, &tpos);
	composeBefore(m, &op);
}

void uiDrawMatrixSkew(uiDrawMatrix *m, double x, double y, double xamount, double yamount)
{
	uiDrawMatrix tneg, sk, tpos;
	setAll(&tneg, 1, 0, 0, 1, -x, -y);
	setAll(&sk, 1, tan(yamount), tan(xamount), 1, 0, 0);
	setAll(&tpos, 1, 0, 0, 1, x, y);
	uiDrawMatrix op = mul(&tneg, &sk);
	op = mul(&op, &tpos);
	composeBefore(m, &op);
}

void uiDrawMatrixMultiply(uiDrawMatrix *dest, uiDrawMatrix *src)
{
	uiDrawMatrix r = mul(dest, src);
	*dest = r;
}

static double determinant(const uiDrawMatrix *m)
{
	return m->M11 * m->M22 - m->M12 * m->M21;
}

int uiDrawMatrixInvertible(uiDrawMatrix *m)
{
	return determinant(m) != 0.0 ? 1 : 0;
}

int uiDrawMatrixInvert(uiDrawMatrix *m)
{
	double det = determinant(m);
	if (det == 0.0)
		return 0;
	double id = 1.0 / det;
	double a = m->M11, b = m->M12, c = m->M21, d = m->M22, e = m->M31, f = m->M32;
	m->M11 = d * id;
	m->M12 = -b * id;
	m->M21 = -c * id;
	m->M22 = a * id;
	m->M31 = (c * f - d * e) * id;
	m->M32 = (b * e - a * f) * id;
	return 1;
}

void uiDrawMatrixTransformPoint(uiDrawMatrix *m, double *x, double *y)
{
	double px = *x, py = *y;
	*x = px * m->M11 + py * m->M21 + m->M31;
	*y = px * m->M12 + py * m->M22 + m->M32;
}

void uiDrawMatrixTransformSize(uiDrawMatrix *m, double *x, double *y)
{
	double px = *x, py = *y;
	*x = px * m->M11 + py * m->M21;
	*y = px * m->M12 + py * m->M22;
}
