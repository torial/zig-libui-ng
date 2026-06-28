// libui-ng Haiku backend — uiColorButton.
// Haiku has no native "color button", so this is a color swatch (a small custom BView) that opens a
// modal BColorControl dialog on click. The dialog is run synchronously via a semaphore (like the
// file panels): show it, block the caller until OK/Cancel, read the result.
#include <OS.h>
#include "uipriv_haiku.h"

struct uiColorButton;

class ColorSwatch : public BView {
public:
	uiColorButton *btn;
	ColorSwatch() : BView("uiColorButton", B_WILL_DRAW | B_NAVIGABLE)
	{
		this->btn = NULL;
		SetExplicitMinSize(BSize(56, 22));
	}
	virtual void Draw(BRect update);
	virtual void MouseDown(BPoint where);
};

struct uiColorButton {
	uiHaikuControl c;
	ColorSwatch *view;
	double r, g, b, a;
	void (*onChanged)(uiColorButton *, void *);
	void *onChangedData;
};

uiHaikuControlAllDefaults(uiColorButton)

static void defaultOnChanged(uiColorButton *b, void *d) { (void) b; (void) d; }

// Modal dialog wrapping a BColorControl.
class ColorDialog : public BWindow {
public:
	BColorControl *picker;
	sem_id sem;
	bool ok;
	rgb_color result;

	ColorDialog(rgb_color initial)
		: BWindow(BRect(150, 150, 450, 400), "Choose a color",
			B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
			B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS)
	{
		this->sem = create_sem(0, "uiColorDialog");
		this->ok = false;
		this->picker = new BColorControl(BPoint(0, 0), B_CELLS_32x8, 8.0, "picker");
		this->picker->SetValue(initial);
		BButton *okb = new BButton("ok", "OK", new BMessage('ok__'));
		BButton *cnb = new BButton("cn", "Cancel", new BMessage('cncl'));
		BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
			.SetInsets(B_USE_WINDOW_SPACING)
			.Add(this->picker)
			.AddGroup(B_HORIZONTAL)
				.AddGlue()
				.Add(cnb)
				.Add(okb)
			.End()
		.End();
	}
	~ColorDialog() { delete_sem(this->sem); }

	virtual void MessageReceived(BMessage *m)
	{
		if (m->what == 'ok__') {
			this->result = this->picker->ValueAsColor();
			this->ok = true;
			release_sem(this->sem);
			return;
		}
		if (m->what == 'cncl') {
			this->ok = false;
			release_sem(this->sem);
			return;
		}
		BWindow::MessageReceived(m);
	}
	virtual bool QuitRequested()
	{
		this->ok = false;
		release_sem(this->sem);
		return true;
	}
};

void ColorSwatch::Draw(BRect update)
{
	(void) update;
	BRect r = Bounds();
	rgb_color fill = make_color((uint8) (this->btn->r * 255), (uint8) (this->btn->g * 255),
		(uint8) (this->btn->b * 255), (uint8) (this->btn->a * 255));
	SetHighColor(fill);
	FillRect(r);
	SetHighColor(make_color(100, 100, 100, 255));
	StrokeRect(r);
}

void ColorSwatch::MouseDown(BPoint where)
{
	(void) where;
	rgb_color init = make_color((uint8) (this->btn->r * 255), (uint8) (this->btn->g * 255),
		(uint8) (this->btn->b * 255), (uint8) (this->btn->a * 255));
	ColorDialog *d = new ColorDialog(init);
	d->Show();
	acquire_sem(d->sem);
	if (d->ok) {
		this->btn->r = d->result.red / 255.0;
		this->btn->g = d->result.green / 255.0;
		this->btn->b = d->result.blue / 255.0;
		this->btn->a = d->result.alpha / 255.0;
		Invalidate();
		(*(this->btn->onChanged))(this->btn, this->btn->onChangedData);
	}
	if (d->Lock())
		d->Quit();
}

void uiColorButtonColor(uiColorButton *b, double *r, double *g, double *bl, double *a)
{
	*r = b->r; *g = b->g; *bl = b->b; *a = b->a;
}

void uiColorButtonSetColor(uiColorButton *b, double r, double g, double bl, double a)
{
	b->r = r; b->g = g; b->b = bl; b->a = a;
	BWindow *win = b->view->Window();
	if (win != NULL) win->Lock();
	b->view->Invalidate();
	if (win != NULL) win->Unlock();
}

void uiColorButtonOnChanged(uiColorButton *b, void (*f)(uiColorButton *, void *), void *data)
{
	b->onChanged = f;
	b->onChangedData = data;
}

uiColorButton *uiNewColorButton(void)
{
	uiColorButton *b;

	uiHaikuNewControl(uiColorButton, b);
	b->r = 0.2; b->g = 0.4; b->b = 0.8; b->a = 1.0;
	b->onChanged = defaultOnChanged;
	b->view = new ColorSwatch();
	b->view->btn = b;

	return b;
}
