// libui-ng on Haiku — uiTable with an image column plus three text columns, backed by a uiTableModel.
#include <stdio.h>
#include <string.h>
#include "../../ui.h"

// Column 0 = image, 1-3 = text, 4 = int (progress), 5 = int (checkbox), 6 = string (button).
#define NCOLS 7

static const char *rowdata[][3] = {
	{ "Alice",   "Engineering", "Active" },
	{ "Bob",     "Design",      "Active" },
	{ "Carol",   "Research",    "On leave" },
	{ "Dave",    "Operations",  "Active" },
	{ "Eve",     "Security",    "Active" },
};
#define NROWS ((int) (sizeof rowdata / sizeof rowdata[0]))

static const int loadPct[NROWS] = { 100, 60, 0, 80, 45 };
static int doneFlag[NROWS] = { 1, 1, 0, 1, 0 };	// writable: toggled by the checkbox column
static char nameBuf[NROWS][32];			// writable: edited in place via the Name column

static uiImage *swatches[NROWS];

static uiImage *makeSwatch(unsigned char r, unsigned char g, unsigned char b)
{
	uiImage *img = uiNewImage(24, 24);
	unsigned char px[24 * 24 * 4];
	for (int i = 0; i < 24 * 24; i++) {
		px[i * 4 + 0] = r;
		px[i * 4 + 1] = g;
		px[i * 4 + 2] = b;
		px[i * 4 + 3] = 255;
	}
	uiImageAppend(img, px, 24, 24, 24 * 4);
	return img;
}

static int modelNumColumns(uiTableModelHandler *mh, uiTableModel *m)
{
	(void) mh; (void) m;
	return NCOLS;
}

static uiTableValueType modelColumnType(uiTableModelHandler *mh, uiTableModel *m, int column)
{
	(void) mh; (void) m;
	if (column == 0)
		return uiTableValueTypeImage;
	if (column == 4 || column == 5)
		return uiTableValueTypeInt;
	return uiTableValueTypeString;
}

static int modelNumRows(uiTableModelHandler *mh, uiTableModel *m)
{
	(void) mh; (void) m;
	return NROWS;
}

static uiTableValue *modelCellValue(uiTableModelHandler *mh, uiTableModel *m, int row, int column)
{
	(void) mh; (void) m;
	if (column == 0)
		return uiNewTableValueImage(swatches[row]);
	if (column == 4)
		return uiNewTableValueInt(loadPct[row]);
	if (column == 5)
		return uiNewTableValueInt(doneFlag[row]);
	if (column == 6)
		return uiNewTableValueString("Ping");
	if (column == 1)
		return uiNewTableValueString(nameBuf[row]);	// editable
	return uiNewTableValueString(rowdata[row][column - 1]);
}

static void modelSetCellValue(uiTableModelHandler *mh, uiTableModel *m, int row, int column,
	const uiTableValue *value)
{
	(void) mh; (void) m;
	if (column == 1) {	// Name edited in place
		if (value != NULL) {
			strncpy(nameBuf[row], uiTableValueString(value), sizeof nameBuf[row] - 1);
			nameBuf[row][sizeof nameBuf[row] - 1] = '\0';
		}
	} else if (column == 5)	// checkbox toggled
		doneFlag[row] = (value != NULL && uiTableValueInt(value)) ? 1 : 0;
	else if (column == 6) {	// button clicked (value is NULL)
		printf("button clicked on row %d (%s)\n", row, rowdata[row][0]);
		fflush(stdout);
	}
}

static uiTableModelHandler handler = {
	modelNumColumns,
	modelColumnType,
	modelNumRows,
	modelCellValue,
	modelSetCellValue,
};

static int onClosing(uiWindow *w, void *data)
{
	(void) w; (void) data;
	uiQuit();
	return 1;
}

static void onRowClicked(uiTable *t, int row, void *userdata)
{
	(void) t; (void) userdata;
	if (row >= 0 && row < NROWS)
		printf("row clicked: %d (%s)\n", row, rowdata[row][0]);
}

int main(void)
{
	uiInitOptions o;
	const char *err;
	uiWindow *w;
	uiTableModel *model;
	uiTable *table;
	uiTableParams p;

	memset(&o, 0, sizeof o);
	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "error initializing libui: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	for (int i = 0; i < NROWS; i++) {
		strncpy(nameBuf[i], rowdata[i][0], sizeof nameBuf[i] - 1);
		nameBuf[i][sizeof nameBuf[i] - 1] = '\0';
	}

	swatches[0] = makeSwatch(80, 160, 240);
	swatches[1] = makeSwatch(120, 200, 120);
	swatches[2] = makeSwatch(230, 180, 60);
	swatches[3] = makeSwatch(200, 110, 200);
	swatches[4] = makeSwatch(220, 90, 90);

	w = uiNewWindow("libui-ng on Haiku — uiTable", 880, 240, 0);
	uiWindowOnClosing(w, onClosing, NULL);
	uiWindowSetMargined(w, 1);

	model = uiNewTableModel(&handler);

	memset(&p, 0, sizeof p);
	p.Model = model;
	p.RowBackgroundColorModelColumn = -1;
	table = uiNewTable(&p);
	uiTableAppendImageColumn(table, "",          0);
	uiTableAppendTextColumn(table, "Name",       1, uiTableModelColumnAlwaysEditable, NULL);
	uiTableAppendTextColumn(table, "Department", 2, uiTableModelColumnNeverEditable, NULL);
	uiTableAppendTextColumn(table, "Status",     3, uiTableModelColumnNeverEditable, NULL);
	uiTableAppendProgressBarColumn(table, "Load", 4);
	uiTableAppendCheckboxColumn(table, "Done", 5, uiTableModelColumnAlwaysEditable);
	uiTableAppendButtonColumn(table, "Action", 6, uiTableModelColumnAlwaysEditable);
	uiTableOnRowClicked(table, onRowClicked, NULL);

	uiWindowSetChild(w, uiControl(table));
	uiControlShow(uiControl(w));

	uiMain();
	uiFreeTableModel(model);
	for (int i = 0; i < NROWS; i++)
		uiFreeImage(swatches[i]);
	uiUninit();
	return 0;
}
