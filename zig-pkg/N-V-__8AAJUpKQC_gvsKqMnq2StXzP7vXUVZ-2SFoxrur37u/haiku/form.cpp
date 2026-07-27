// libui-ng Haiku backend — uiForm (BGridView: a column of label/control rows).
// Children are placed directly into the grid layout (column 0 = label, column 1 = control) rather
// than through the AddChild SetContainer path, since the form owns explicit cell positions.
#include "uipriv_haiku.h"

struct formChild {
	uiControl *child;
	BStringView *label;
	int stretchy;
};

struct uiForm {
	uiHaikuControl c;
	BGridView *view;
	BList *children;	// formChild*
	int padded;
};

static void uiFormDestroy(uiControl *cc)
{
	uiForm *f = (uiForm *) cc;
	for (int32 i = 0; i < f->children->CountItems(); i++) {
		formChild *fc = (formChild *) f->children->ItemAt(i);
		BView *cv = (BView *) uiControlHandle(fc->child);
		if (cv->Parent() != NULL) cv->RemoveSelf();
		uiControlSetParent(fc->child, NULL);
		uiControlDestroy(fc->child);
		if (fc->label->Parent() != NULL) fc->label->RemoveSelf();
		delete fc->label;
		delete fc;
	}
	delete f->children;
	if (f->view->Parent() != NULL)
		f->view->RemoveSelf();
	delete f->view;
	uiFreeControl(cc);
}

uiHaikuControlAllDefaultsExceptDestroy(uiForm)

void uiFormAppend(uiForm *f, const char *label, uiControl *c, int stretchy)
{
	BWindow *win = f->view->Window();
	if (win != NULL) win->Lock();

	formChild *fc = new formChild();
	fc->child = c;
	fc->stretchy = stretchy;
	fc->label = new BStringView(NULL, label);

	int32 row = f->children->CountItems();
	BGridLayout *g = f->view->GridLayout();
	g->AddView(fc->label, 0, row);
	BView *cv = (BView *) uiControlHandle(c);
	uiControlSetParent(c, uiControl(f));
	g->AddView(cv, 1, row);
	if (stretchy)
		g->SetRowWeight(row, 1.0f);
	f->children->AddItem(fc);

	if (win != NULL) win->Unlock();
}

int uiFormNumChildren(uiForm *f)
{
	return (int) f->children->CountItems();
}

void uiFormDelete(uiForm *f, int index)
{
	formChild *fc = (formChild *) f->children->ItemAt(index);
	if (fc == NULL)
		return;
	BWindow *win = f->view->Window();
	if (win != NULL) win->Lock();
	BView *cv = (BView *) uiControlHandle(fc->child);
	if (cv->Parent() != NULL) cv->RemoveSelf();	// detach (caller keeps ownership of the control)
	uiControlSetParent(fc->child, NULL);
	if (fc->label->Parent() != NULL) fc->label->RemoveSelf();
	delete fc->label;
	f->children->RemoveItem(index);
	delete fc;
	if (win != NULL) win->Unlock();
}

int uiFormPadded(uiForm *f) { return f->padded; }

void uiFormSetPadded(uiForm *f, int padded)
{
	f->padded = padded;
	BWindow *win = f->view->Window();
	if (win != NULL) win->Lock();
	float s = padded ? B_USE_DEFAULT_SPACING : 0;
	f->view->GridLayout()->SetSpacing(s, s);
	if (win != NULL) win->Unlock();
}

uiForm *uiNewForm(void)
{
	uiForm *f;

	uiHaikuNewControl(uiForm, f);
	uiControl(f)->Destroy = uiFormDestroy;
	f->view = new BGridView();
	f->view->GridLayout()->SetSpacing(0, 0);
	f->children = new BList();
	f->padded = 0;

	return f;
}
