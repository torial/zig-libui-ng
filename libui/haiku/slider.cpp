// libui-ng Haiku backend — uiSlider (BSlider).
// BSlider's modification message fires continuously during a drag (-> OnChanged); its invocation
// message fires on release (-> OnReleased).
#include "uipriv_haiku.h"

struct uiSlider {
	uiHaikuControl c;
	BSlider *view;
	void (*onChanged)(uiSlider *, void *);
	void *onChangedData;
	void (*onReleased)(uiSlider *, void *);
	void *onReleasedData;
};

uiHaikuControlAllDefaults(uiSlider)

static void sliderChangedDispatch(void *control)
{
	uiSlider *s = (uiSlider *) control;
	(*(s->onChanged))(s, s->onChangedData);
}

static void sliderReleasedDispatch(void *control)
{
	uiSlider *s = (uiSlider *) control;
	(*(s->onReleased))(s, s->onReleasedData);
}

static void defaultOnChanged(uiSlider *s, void *data) { (void) s; (void) data; }
static void defaultOnReleased(uiSlider *s, void *data) { (void) s; (void) data; }

int uiSliderValue(uiSlider *s)
{
	return s->view->Value();
}

void uiSliderSetValue(uiSlider *s, int value)
{
	BWindow *win = s->view->Window();
	if (win != NULL) win->Lock();
	s->view->SetValue(value);
	if (win != NULL) win->Unlock();
}

int uiSliderHasToolTip(uiSlider *s)
{
	// BSlider draws a value label while dragging; there's no separate tooltip toggle to report.
	return s->view->UpdateText() != NULL ? 1 : 0;
}

void uiSliderSetHasToolTip(uiSlider *s, int hasToolTip)
{
	(void) s; (void) hasToolTip;	// no direct equivalent on BSlider
}

void uiSliderOnChanged(uiSlider *s, void (*f)(uiSlider *, void *), void *data)
{
	s->onChanged = f;
	s->onChangedData = data;
}

void uiSliderOnReleased(uiSlider *s, void (*f)(uiSlider *, void *), void *data)
{
	s->onReleased = f;
	s->onReleasedData = data;
}

void uiSliderSetRange(uiSlider *s, int min, int max)
{
	BWindow *win = s->view->Window();
	if (win != NULL) win->Lock();
	s->view->SetLimits(min, max);
	if (win != NULL) win->Unlock();
}

uiSlider *uiNewSlider(int min, int max)
{
	uiSlider *s;

	uiHaikuNewControl(uiSlider, s);
	s->view = new BSlider("uiSlider", NULL, NULL, min, max, B_HORIZONTAL);
	s->view->SetModificationMessage(uiprivNewEventMessage(sliderChangedDispatch, s));
	s->view->SetMessage(uiprivNewEventMessage(sliderReleasedDispatch, s));
	s->onChanged = defaultOnChanged;
	s->onReleased = defaultOnReleased;

	return s;
}
