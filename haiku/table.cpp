// libui-ng Haiku backend — uiTable (BColumnListView).
//
// SCOPE: text columns are fully supported. The other column kinds (image, image+text, checkbox,
// checkbox+text, progress bar, button) are rendered best-effort as text for now (BColumnListView's
// richer column types need dedicated BColumn subclasses) — they keep the column/field indexing
// consistent and display the model value as a string. Inline cell editing is not wired up.
//
// libui's model is pull-based but BColumnListView is push-based, so we keep a BList of BRows parallel
// to the view (for index<->BRow mapping) and (re)fill fields from the model as columns are appended
// and as row notifications arrive.
#include <stdint.h>
#include <stdlib.h>
#include <Bitmap.h>
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include "uipriv_haiku.h"

enum { colText, colImage, colImageText, colCheckbox, colCheckboxText, colProgress, colButton };

// Custom display columns: progress bar and checkbox. Both read the cell's BStringField (which holds
// the model int as text) and draw — display-only (interactive toggling/clicking is a follow-up, as
// BColumn doesn't expose per-cell click routing simply). DrawField is BColumn's drawing hook.
class ProgressColumn : public BTitledColumn {
public:
	ProgressColumn(const char *title, float w, float minW, float maxW)
		: BTitledColumn(title, w, minW, maxW, B_ALIGN_LEFT) {}
	virtual void DrawField(BField *field, BRect rect, BView *parent)
	{
		BStringField *f = dynamic_cast<BStringField *>(field);
		int v = (f != NULL) ? atoi(f->String()) : 0;
		if (v < 0) v = 0; if (v > 100) v = 100;
		BRect bar = rect;
		bar.InsetBy(4, (rect.Height() - 12) / 2);
		parent->SetHighColor(make_color(150, 150, 150, 255));
		parent->StrokeRect(bar);
		BRect fill = bar;
		fill.InsetBy(1, 1);
		fill.right = fill.left + fill.Width() * v / 100.0;
		parent->SetHighColor(make_color(90, 160, 90, 255));
		parent->FillRect(fill);
	}
};

class CheckColumn : public BTitledColumn {
public:
	CheckColumn(const char *title, float w, float minW, float maxW)
		: BTitledColumn(title, w, minW, maxW, B_ALIGN_CENTER) {}
	virtual void DrawField(BField *field, BRect rect, BView *parent)
	{
		BStringField *f = dynamic_cast<BStringField *>(field);
		int v = (f != NULL) ? atoi(f->String()) : 0;
		BRect box(0, 0, 13, 13);
		box.OffsetTo(rect.left + 5, rect.top + (rect.Height() - 13) / 2);
		parent->SetHighColor(make_color(120, 120, 120, 255));
		parent->StrokeRect(box);
		if (v != 0) {
			parent->SetHighColor(make_color(40, 120, 40, 255));
			parent->StrokeLine(BPoint(box.left + 2, box.top + 6), BPoint(box.left + 5, box.top + 10));
			parent->StrokeLine(BPoint(box.left + 5, box.top + 10), BPoint(box.left + 11, box.top + 2));
		}
	}
};

struct tableColumn {
	int modelColumn;	// primary data column in the model
	int kind;
	BColumn *col;		// BStringColumn for text kinds, BBitmapColumn for image
};

struct uiTable {
	uiHaikuControl c;
	BColumnListView *view;
	uiTableModel *model;
	BList *columns;		// tableColumn*
	BList *rows;		// BRow* parallel to the view's rows
	int selectionMode;
	void (*onSelectionChanged)(uiTable *, void *);
	void *onSelectionChangedData;
	void (*onRowClicked)(uiTable *, int, void *);
	void *onRowClickedData;
	void (*onRowDoubleClicked)(uiTable *, int, void *);
	void *onRowDoubleClickedData;
};

static void uiTableDestroy(uiControl *cc)
{
	uiTable *t = (uiTable *) cc;
	t->model->tables->RemoveItem(t);
	for (int32 i = 0; i < t->columns->CountItems(); i++)
		delete (tableColumn *) t->columns->ItemAt(i);
	delete t->columns;
	delete t->rows;		// the BRows themselves are owned by the view
	if (t->view->Parent() != NULL)
		t->view->RemoveSelf();
	delete t->view;
	uiFreeControl(cc);
}

uiHaikuControlAllDefaultsExceptDestroy(uiTable)

static void defaultSelChanged(uiTable *t, void *d) { (void) t; (void) d; }
static void defaultRowClicked(uiTable *t, int r, void *d) { (void) t; (void) r; (void) d; }

// Convert the model's value at (row, modelColumn) to a display string (text columns proper; other
// kinds rendered best-effort).
static BString cellString(uiTable *t, int row, int modelColumn)
{
	BString out;
	uiTableValue *v = (*(t->model->mh->CellValue))(t->model->mh, t->model, row, modelColumn);
	if (v != NULL) {
		switch (uiTableValueGetType(v)) {
		case uiTableValueTypeString: out = uiTableValueString(v); break;
		case uiTableValueTypeInt: out.SetToFormat("%d", uiTableValueInt(v)); break;
		default: out = ""; break;	// image/color have no text form here
		}
		uiFreeTableValue(v);
	}
	return out;
}

// Build the BField for one cell. Image columns produce a BBitmapField (owning a copy of the model's
// bitmap); everything else produces a BStringField.
static BField *makeField(uiTable *t, tableColumn *tc, int row)
{
	if (tc->kind == colImage) {
		BBitmap *bmp = NULL;
		uiTableValue *v = (*(t->model->mh->CellValue))(t->model->mh, t->model, row, tc->modelColumn);
		if (v != NULL) {
			if (uiTableValueGetType(v) == uiTableValueTypeImage) {
				uiImage *img = uiTableValueImage(v);
				BBitmap *src = (img != NULL) ? uiprivImageBestBitmap(img) : NULL;
				if (src != NULL)
					bmp = new BBitmap(src);		// the field takes ownership
			}
			uiFreeTableValue(v);
		}
		return new BBitmapField(bmp);
	}
	BString s = cellString(t, row, tc->modelColumn);
	return new BStringField(s.String());
}

static void fillRow(uiTable *t, BRow *r, int row)
{
	for (int32 k = 0; k < t->columns->CountItems(); k++) {
		tableColumn *tc = (tableColumn *) t->columns->ItemAt(k);
		r->SetField(makeField(t, tc, row), k);
	}
}

static int selectedIndex(uiTable *t)
{
	BRow *r = t->view->CurrentSelection(NULL);
	if (r == NULL)
		return -1;
	return (int) t->rows->IndexOf(r);
}

static void selDispatch(void *tbl)
{
	uiTable *t = (uiTable *) tbl;
	(*(t->onSelectionChanged))(t, t->onSelectionChangedData);
	int idx = selectedIndex(t);
	if (idx >= 0)
		(*(t->onRowClicked))(t, idx, t->onRowClickedData);
}

static void invokeDispatch(void *tbl)
{
	uiTable *t = (uiTable *) tbl;
	int idx = selectedIndex(t);
	if (idx >= 0)
		(*(t->onRowDoubleClicked))(t, idx, t->onRowDoubleClickedData);
}

uiTable *uiNewTable(uiTableParams *params)
{
	uiTable *t;

	uiHaikuNewControl(uiTable, t);
	uiControl(t)->Destroy = uiTableDestroy;

	t->model = params->Model;
	t->columns = new BList();
	t->rows = new BList();
	t->selectionMode = uiTableSelectionModeZeroOrOne;
	t->onSelectionChanged = defaultSelChanged;
	t->onRowClicked = defaultRowClicked;
	t->onRowDoubleClicked = defaultRowClicked;

	t->view = new BColumnListView("uiTable", 0);
	t->view->SetSelectionMode(B_SINGLE_SELECTION_LIST);
	t->view->SetSelectionMessage(uiprivNewEventMessage(selDispatch, t));
	t->view->SetInvocationMessage(uiprivNewEventMessage(invokeDispatch, t));

	// Create one (empty) BRow per model row; columns fill their fields when appended.
	int n = (*(t->model->mh->NumRows))(t->model->mh, t->model);
	for (int i = 0; i < n; i++) {
		BRow *r = new BRow();
		t->view->AddRow(r);
		t->rows->AddItem(r);
	}

	t->model->tables->AddItem(t);
	return t;
}

static void addColumn(uiTable *t, const char *name, int modelColumn, int kind)
{
	BWindow *win = t->view->Window();
	if (win != NULL) win->Lock();

	int32 k = t->columns->CountItems();
	BColumn *col;
	if (kind == colImage)
		col = new BBitmapColumn(name, 48, 20, 200, B_ALIGN_CENTER);
	else if (kind == colProgress)
		col = new ProgressColumn(name, 130, 50, 400);
	else if (kind == colCheckbox)
		col = new CheckColumn(name, 60, 30, 200);
	else
		col = new BStringColumn(name, 150, 30, 2000, B_TRUNCATE_END);
	t->view->AddColumn(col, k);

	tableColumn *tc = new tableColumn();
	tc->modelColumn = modelColumn;
	tc->kind = kind;
	tc->col = col;
	t->columns->AddItem(tc);

	for (int32 i = 0; i < t->rows->CountItems(); i++) {
		BRow *r = (BRow *) t->rows->ItemAt(i);
		r->SetField(makeField(t, tc, (int) i), k);
	}

	if (win != NULL) win->Unlock();
}

void uiTableAppendTextColumn(uiTable *t, const char *name, int textModelColumn,
	int textEditableModelColumn, uiTableTextColumnOptionalParams *textParams)
{
	(void) textEditableModelColumn; (void) textParams;
	addColumn(t, name, textModelColumn, colText);
}

void uiTableAppendImageColumn(uiTable *t, const char *name, int imageModelColumn)
{
	addColumn(t, name, imageModelColumn, colImage);
}

void uiTableAppendImageTextColumn(uiTable *t, const char *name, int imageModelColumn,
	int textModelColumn, int textEditableModelColumn, uiTableTextColumnOptionalParams *textParams)
{
	(void) imageModelColumn; (void) textEditableModelColumn; (void) textParams;
	addColumn(t, name, textModelColumn, colImageText);
}

void uiTableAppendCheckboxColumn(uiTable *t, const char *name, int checkboxModelColumn,
	int checkboxEditableModelColumn)
{
	(void) checkboxEditableModelColumn;
	addColumn(t, name, checkboxModelColumn, colCheckbox);
}

void uiTableAppendCheckboxTextColumn(uiTable *t, const char *name, int checkboxModelColumn,
	int checkboxEditableModelColumn, int textModelColumn, int textEditableModelColumn,
	uiTableTextColumnOptionalParams *textParams)
{
	(void) checkboxModelColumn; (void) checkboxEditableModelColumn;
	(void) textEditableModelColumn; (void) textParams;
	addColumn(t, name, textModelColumn, colCheckboxText);
}

void uiTableAppendProgressBarColumn(uiTable *t, const char *name, int progressModelColumn)
{
	addColumn(t, name, progressModelColumn, colProgress);
}

void uiTableAppendButtonColumn(uiTable *t, const char *name, int buttonModelColumn,
	int buttonClickableModelColumn)
{
	(void) buttonClickableModelColumn;
	addColumn(t, name, buttonModelColumn, colButton);
}

int uiTableHeaderVisible(uiTable *t) { (void) t; return 1; }
void uiTableHeaderSetVisible(uiTable *t, int visible) { (void) t; (void) visible; }

void uiTableOnRowClicked(uiTable *t, void (*f)(uiTable *, int, void *), void *data)
{
	t->onRowClicked = f;
	t->onRowClickedData = data;
}

void uiTableOnRowDoubleClicked(uiTable *t, void (*f)(uiTable *, int, void *), void *data)
{
	t->onRowDoubleClicked = f;
	t->onRowDoubleClickedData = data;
}

void uiTableHeaderSetSortIndicator(uiTable *t, int column, uiSortIndicator indicator)
{
	(void) t; (void) column; (void) indicator;
}

uiSortIndicator uiTableHeaderSortIndicator(uiTable *t, int column)
{
	(void) t; (void) column;
	return uiSortIndicatorNone;
}

void uiTableHeaderOnClicked(uiTable *t, void (*f)(uiTable *, int, void *), void *data)
{
	(void) t; (void) f; (void) data;
}

int uiTableColumnWidth(uiTable *t, int column)
{
	tableColumn *tc = (tableColumn *) t->columns->ItemAt(column);
	if (tc == NULL)
		return 0;
	return (int) tc->col->Width();
}

void uiTableColumnSetWidth(uiTable *t, int column, int width)
{
	tableColumn *tc = (tableColumn *) t->columns->ItemAt(column);
	if (tc == NULL)
		return;
	BWindow *win = t->view->Window();
	if (win != NULL) win->Lock();
	tc->col->SetWidth((float) width);
	if (win != NULL) win->Unlock();
}

uiTableSelectionMode uiTableGetSelectionMode(uiTable *t)
{
	return (uiTableSelectionMode) t->selectionMode;
}

void uiTableSetSelectionMode(uiTable *t, uiTableSelectionMode mode)
{
	t->selectionMode = mode;
	BWindow *win = t->view->Window();
	if (win != NULL) win->Lock();
	list_view_type lt = (mode == uiTableSelectionModeZeroOrMany)
		? B_MULTIPLE_SELECTION_LIST : B_SINGLE_SELECTION_LIST;
	t->view->SetSelectionMode(lt);
	if (win != NULL) win->Unlock();
}

void uiTableOnSelectionChanged(uiTable *t, void (*f)(uiTable *, void *), void *data)
{
	t->onSelectionChanged = f;
	t->onSelectionChangedData = data;
}

uiTableSelection *uiTableGetSelection(uiTable *t)
{
	uiTableSelection *s = uiprivNew(uiTableSelection);
	BList idx;
	BRow *r = NULL;
	while ((r = t->view->CurrentSelection(r)) != NULL) {
		int i = (int) t->rows->IndexOf(r);
		if (i >= 0)
			idx.AddItem((void *) (intptr_t) i);
	}
	s->NumRows = (int) idx.CountItems();
	if (s->NumRows > 0) {
		s->Rows = (int *) uiprivAlloc(s->NumRows * sizeof (int), "int[]");
		for (int k = 0; k < s->NumRows; k++)
			s->Rows[k] = (int) (intptr_t) idx.ItemAt(k);
	} else
		s->Rows = NULL;
	return s;
}

void uiTableSetSelection(uiTable *t, uiTableSelection *sel)
{
	BWindow *win = t->view->Window();
	if (win != NULL) win->Lock();
	t->view->DeselectAll();
	for (int k = 0; k < sel->NumRows; k++) {
		BRow *r = (BRow *) t->rows->ItemAt(sel->Rows[k]);
		if (r != NULL)
			t->view->AddToSelection(r);
	}
	if (win != NULL) win->Unlock();
}

void uiFreeTableSelection(uiTableSelection *s)
{
	if (s->Rows != NULL)
		uiprivFree(s->Rows);
	uiprivFree(s);
}

// ---- notifications from the model (tablemodel.cpp) ----

void uiprivTableRowInserted(uiTable *t, int index)
{
	BWindow *win = t->view->Window();
	if (win != NULL) win->Lock();
	BRow *r = new BRow();
	fillRow(t, r, index);
	t->view->AddRow(r, index);
	t->rows->AddItem(r, index);
	if (win != NULL) win->Unlock();
}

void uiprivTableRowChanged(uiTable *t, int index)
{
	BRow *r = (BRow *) t->rows->ItemAt(index);
	if (r == NULL)
		return;
	BWindow *win = t->view->Window();
	if (win != NULL) win->Lock();
	fillRow(t, r, index);
	t->view->UpdateRow(r);
	if (win != NULL) win->Unlock();
}

void uiprivTableRowDeleted(uiTable *t, int index)
{
	BRow *r = (BRow *) t->rows->ItemAt(index);
	if (r == NULL)
		return;
	BWindow *win = t->view->Window();
	if (win != NULL) win->Lock();
	t->view->RemoveRow(r);
	t->rows->RemoveItem(index);
	delete r;
	if (win != NULL) win->Unlock();
}
