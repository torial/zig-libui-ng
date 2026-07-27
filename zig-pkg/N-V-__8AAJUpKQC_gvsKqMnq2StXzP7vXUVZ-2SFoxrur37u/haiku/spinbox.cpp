// libui-ng Haiku backend — uiSpinbox (BSpinner for integers, BDecimalSpinner for doubles).
// BSpinner/BDecimalSpinner live in Haiku's "shared" private kit: headers under
// develop/headers/private/interface, implementation in libshared.a (-lshared). These are the only
// native spinner widgets Haiku provides, so the private dependency is expected and localized here.
#include <stdio.h>
#include <Spinner.h>
#include <DecimalSpinner.h>
#include "uipriv_haiku.h"

struct uiSpinbox {
	uiHaikuControl c;
	BView *view;		// BSpinner or BDecimalSpinner (both BView-derived; macros use `view`)
	int isDouble;
	void (*onChanged)(uiSpinbox *, void *);
	void *onChangedData;
};

uiHaikuControlAllDefaults(uiSpinbox)

static void spinboxDispatch(void *control)
{
	uiSpinbox *s = (uiSpinbox *) control;
	(*(s->onChanged))(s, s->onChangedData);
}

static void defaultOnChanged(uiSpinbox *s, void *data) { (void) s; (void) data; }

int uiSpinboxValue(uiSpinbox *s)
{
	if (s->isDouble)
		return (int) ((BDecimalSpinner *) s->view)->Value();
	return ((BSpinner *) s->view)->Value();
}

double uiSpinboxValueDouble(uiSpinbox *s)
{
	if (s->isDouble)
		return ((BDecimalSpinner *) s->view)->Value();
	return (double) ((BSpinner *) s->view)->Value();
}

char *uiSpinboxValueText(uiSpinbox *s)
{
	char buf[64];
	if (s->isDouble)
		snprintf(buf, sizeof buf, "%f", uiSpinboxValueDouble(s));
	else
		snprintf(buf, sizeof buf, "%d", uiSpinboxValue(s));
	return uiHaikuStrdupText(buf);
}

void uiSpinboxSetValue(uiSpinbox *s, int value)
{
	BWindow *win = s->view->Window();
	if (win != NULL) win->Lock();
	if (s->isDouble)
		((BDecimalSpinner *) s->view)->SetValue((double) value);
	else
		((BSpinner *) s->view)->SetValue(value);
	if (win != NULL) win->Unlock();
}

void uiSpinboxSetValueDouble(uiSpinbox *s, double value)
{
	BWindow *win = s->view->Window();
	if (win != NULL) win->Lock();
	if (s->isDouble)
		((BDecimalSpinner *) s->view)->SetValue(value);
	else
		((BSpinner *) s->view)->SetValue((int32) value);
	if (win != NULL) win->Unlock();
}

void uiSpinboxOnChanged(uiSpinbox *s, void (*f)(uiSpinbox *, void *), void *data)
{
	s->onChanged = f;
	s->onChangedData = data;
}

uiSpinbox *uiNewSpinbox(int min, int max)
{
	uiSpinbox *s;

	uiHaikuNewControl(uiSpinbox, s);
	s->isDouble = 0;
	BSpinner *sp = new BSpinner("uiSpinbox", NULL, uiprivNewEventMessage(spinboxDispatch, s));
	sp->SetRange(min, max);
	s->view = sp;
	s->onChanged = defaultOnChanged;

	return s;
}

uiSpinbox *uiNewSpinboxDouble(double min, double max, int precision)
{
	uiSpinbox *s;

	uiHaikuNewControl(uiSpinbox, s);
	s->isDouble = 1;
	BDecimalSpinner *sp = new BDecimalSpinner("uiSpinbox", NULL,
		uiprivNewEventMessage(spinboxDispatch, s));
	sp->SetPrecision(precision);
	sp->SetRange(min, max);
	s->view = sp;
	s->onChanged = defaultOnChanged;

	return s;
}
