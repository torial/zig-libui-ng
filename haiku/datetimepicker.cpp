// libui-ng Haiku backend — uiDateTimePicker.
// Haiku has no native date/time picker widget, so this is a row of BSpinners (year/month/day and/or
// hour/minute/second). The value round-trips through struct tm.
#include <time.h>
#include <Spinner.h>		// "shared" private kit
#include "uipriv_haiku.h"

enum { dtDate = 1, dtTime = 2, dtDateTime = 3 };

struct uiDateTimePicker {
	uiHaikuControl c;
	BGroupView *view;
	int kind;
	BSpinner *year, *mon, *day, *hour, *min, *sec;
	void (*onChanged)(uiDateTimePicker *, void *);
	void *onChangedData;
};

uiHaikuControlAllDefaults(uiDateTimePicker)

static void defaultOnChanged(uiDateTimePicker *d, void *u) { (void) d; (void) u; }

static void dtDispatch(void *control)
{
	uiDateTimePicker *d = (uiDateTimePicker *) control;
	(*(d->onChanged))(d, d->onChangedData);
}

static BSpinner *addSpinner(uiDateTimePicker *d, int min, int max, int val)
{
	BSpinner *s = new BSpinner("dtfield", NULL, uiprivNewEventMessage(dtDispatch, d));
	s->SetRange(min, max);
	s->SetValue(val);
	d->view->GroupLayout()->AddView(s);
	return s;
}

static uiDateTimePicker *finishNew(int kind)
{
	uiDateTimePicker *d;

	uiHaikuNewControl(uiDateTimePicker, d);
	d->kind = kind;
	d->onChanged = defaultOnChanged;
	d->year = d->mon = d->day = d->hour = d->min = d->sec = NULL;
	d->view = new BGroupView(B_HORIZONTAL);

	time_t now = time(NULL);
	struct tm lt;
	struct tm *p = localtime(&now);
	if (p != NULL) lt = *p; else memset(&lt, 0, sizeof lt);

	if (kind & dtDate) {
		d->year = addSpinner(d, 1900, 2200, lt.tm_year + 1900);
		d->mon  = addSpinner(d, 1, 12, lt.tm_mon + 1);
		d->day  = addSpinner(d, 1, 31, lt.tm_mday);
	}
	if (kind & dtTime) {
		d->hour = addSpinner(d, 0, 23, lt.tm_hour);
		d->min  = addSpinner(d, 0, 59, lt.tm_min);
		d->sec  = addSpinner(d, 0, 59, lt.tm_sec);
	}

	return d;
}

void uiDateTimePickerTime(uiDateTimePicker *d, struct tm *time)
{
	memset(time, 0, sizeof (struct tm));
	time->tm_mday = 1;	// sane default if this picker has no date part
	if (d->year != NULL) time->tm_year = d->year->Value() - 1900;
	if (d->mon  != NULL) time->tm_mon  = d->mon->Value() - 1;
	if (d->day  != NULL) time->tm_mday = d->day->Value();
	if (d->hour != NULL) time->tm_hour = d->hour->Value();
	if (d->min  != NULL) time->tm_min  = d->min->Value();
	if (d->sec  != NULL) time->tm_sec  = d->sec->Value();
	time->tm_isdst = -1;
}

void uiDateTimePickerSetTime(uiDateTimePicker *d, const struct tm *time)
{
	BWindow *win = d->view->Window();
	if (win != NULL) win->Lock();
	if (d->year != NULL) d->year->SetValue(time->tm_year + 1900);
	if (d->mon  != NULL) d->mon->SetValue(time->tm_mon + 1);
	if (d->day  != NULL) d->day->SetValue(time->tm_mday);
	if (d->hour != NULL) d->hour->SetValue(time->tm_hour);
	if (d->min  != NULL) d->min->SetValue(time->tm_min);
	if (d->sec  != NULL) d->sec->SetValue(time->tm_sec);
	if (win != NULL) win->Unlock();
}

void uiDateTimePickerOnChanged(uiDateTimePicker *d, void (*f)(uiDateTimePicker *, void *), void *data)
{
	d->onChanged = f;
	d->onChangedData = data;
}

uiDateTimePicker *uiNewDateTimePicker(void) { return finishNew(dtDateTime); }
uiDateTimePicker *uiNewDatePicker(void) { return finishNew(dtDate); }
uiDateTimePicker *uiNewTimePicker(void) { return finishNew(dtTime); }
