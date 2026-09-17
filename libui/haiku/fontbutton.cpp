// libui-ng Haiku backend — uiFontButton.
// A BButton labelled "Family Size"; clicking opens a modal dialog with the system font-family list
// (BListView) and a size spinner. Weight/italic/stretch aren't exposed in this first version (they
// default to normal). Also provides uiLoadControlFont / uiFreeFontDescriptor / uiFreeFontButtonFont.
#include <OS.h>
#include <Spinner.h>		// "shared" private kit
#include "uipriv_haiku.h"

struct uiFontButton {
	uiHaikuControl c;
	BButton *view;
	uiFontDescriptor desc;	// Family is strdup'd
	void (*onChanged)(uiFontButton *, void *);
	void *onChangedData;
};

static void uiFontButtonDestroy(uiControl *cc)
{
	uiFontButton *b = (uiFontButton *) cc;
	if (b->desc.Family != NULL)
		uiprivFree(b->desc.Family);
	if (b->view->Parent() != NULL)
		b->view->RemoveSelf();
	delete b->view;
	uiFreeControl(cc);
}

uiHaikuControlAllDefaultsExceptDestroy(uiFontButton)

static void defaultOnChanged(uiFontButton *b, void *d) { (void) b; (void) d; }

class FontDialog : public BWindow {
public:
	BListView *list;
	BSpinner *size;
	BCheckBox *boldCb;
	BCheckBox *italicCb;
	sem_id sem;
	bool ok;
	BString family;
	int sz;
	int outWeight;
	int outItalic;

	FontDialog(const char *curFamily, int curSize, int curWeight, int curItalic)
		: BWindow(BRect(150, 150, 450, 480), "Choose a font",
			B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, B_AUTO_UPDATE_SIZE_LIMITS)
	{
		this->sem = create_sem(0, "uiFontDialog");
		this->ok = false;

		this->list = new BListView("families");
		int32 count = count_font_families();
		int32 selectIdx = -1;
		for (int32 i = 0; i < count; i++) {
			font_family fam;
			uint32 flags;
			if (get_font_family(i, &fam, &flags) == B_OK) {
				this->list->AddItem(new BStringItem(fam));
				if (curFamily != NULL && strcmp(fam, curFamily) == 0)
					selectIdx = i;
			}
		}
		if (selectIdx >= 0)
			this->list->Select(selectIdx);

		this->size = new BSpinner("size", NULL, NULL);
		this->size->SetRange(6, 96);
		this->size->SetValue(curSize);

		this->boldCb = new BCheckBox("bold", "Bold", NULL);
		if (curWeight >= 700) this->boldCb->SetValue(B_CONTROL_ON);
		this->italicCb = new BCheckBox("italic", "Italic", NULL);
		if (curItalic != uiTextItalicNormal) this->italicCb->SetValue(B_CONTROL_ON);

		BButton *okb = new BButton("ok", "OK", new BMessage('ok__'));
		BButton *cnb = new BButton("cn", "Cancel", new BMessage('cncl'));

		BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
			.SetInsets(B_USE_WINDOW_SPACING)
			.Add(new BScrollView("listScroll", this->list, 0, false, true))
			.AddGroup(B_HORIZONTAL)
				.Add(new BStringView(NULL, "Size:"))
				.Add(this->size)
				.Add(this->boldCb)
				.Add(this->italicCb)
				.AddGlue()
			.End()
			.AddGroup(B_HORIZONTAL)
				.AddGlue()
				.Add(cnb)
				.Add(okb)
			.End()
		.End();
	}
	~FontDialog() { delete_sem(this->sem); }

	virtual void MessageReceived(BMessage *m)
	{
		if (m->what == 'ok__') {
			int32 sel = this->list->CurrentSelection();
			if (sel >= 0) {
				BStringItem *it = (BStringItem *) this->list->ItemAt(sel);
				this->family = it->Text();
			}
			this->sz = (int) this->size->Value();
			this->outWeight = (this->boldCb->Value() == B_CONTROL_ON) ? 700 : 400;
			this->outItalic = (this->italicCb->Value() == B_CONTROL_ON)
				? uiTextItalicItalic : uiTextItalicNormal;
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

static void updateLabel(uiFontButton *b)
{
	BString s;
	s.SetToFormat("%s %d", b->desc.Family ? b->desc.Family : "", (int) b->desc.Size);
	b->view->SetLabel(s.String());
}

static void fontDispatch(void *control)
{
	uiFontButton *b = (uiFontButton *) control;
	FontDialog *d = new FontDialog(b->desc.Family, (int) b->desc.Size,
		b->desc.Weight, b->desc.Italic);
	d->Show();
	acquire_sem(d->sem);
	if (d->ok && d->family.Length() > 0) {
		if (b->desc.Family != NULL)
			uiprivFree(b->desc.Family);
		b->desc.Family = uiHaikuStrdupText(d->family.String());
		b->desc.Size = d->sz;
		b->desc.Weight = (uiTextWeight) d->outWeight;
		b->desc.Italic = (uiTextItalic) d->outItalic;
		updateLabel(b);
		(*(b->onChanged))(b, b->onChangedData);
	}
	if (d->Lock())
		d->Quit();
}

void uiFontButtonFont(uiFontButton *b, uiFontDescriptor *desc)
{
	desc->Family = uiHaikuStrdupText(b->desc.Family ? b->desc.Family : "");
	desc->Size = b->desc.Size;
	desc->Weight = b->desc.Weight;
	desc->Italic = b->desc.Italic;
	desc->Stretch = b->desc.Stretch;
}

void uiFontButtonOnChanged(uiFontButton *b, void (*f)(uiFontButton *, void *), void *data)
{
	b->onChanged = f;
	b->onChangedData = data;
}

uiFontButton *uiNewFontButton(void)
{
	uiFontButton *b;

	uiHaikuNewControl(uiFontButton, b);
	uiControl(b)->Destroy = uiFontButtonDestroy;
	b->onChanged = defaultOnChanged;

	uiFontDescriptor d;
	uiLoadControlFont(&d);		// fills from the system plain font (Family is strdup'd)
	b->desc = d;

	b->view = new BButton("uiFontButton", "", uiprivNewEventMessage(fontDispatch, b));
	updateLabel(b);

	return b;
}

// ---- font descriptor helpers (backend-provided; cf. unix/drawtext.c) ----

void uiLoadControlFont(uiFontDescriptor *f)
{
	font_family fam;
	font_style sty;
	be_plain_font->GetFamilyAndStyle(&fam, &sty);
	f->Family = uiHaikuStrdupText(fam);
	f->Size = be_plain_font->Size();
	f->Weight = uiTextWeightNormal;
	f->Italic = uiTextItalicNormal;
	f->Stretch = uiTextStretchNormal;
}

void uiFreeFontDescriptor(uiFontDescriptor *desc)
{
	uiprivFree(desc->Family);
}

void uiFreeFontButtonFont(uiFontDescriptor *desc)
{
	uiprivFree(desc->Family);
}
