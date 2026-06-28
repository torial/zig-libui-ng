// libui-ng Haiku backend — uiTableModel (wraps the user's uiTableModelHandler).
// Row notifications fan out to every uiTable built from this model (see table.cpp).
#include "uipriv_haiku.h"

uiTableModel *uiNewTableModel(uiTableModelHandler *mh)
{
	uiTableModel *m = uiprivNew(uiTableModel);
	m->mh = mh;
	m->tables = new BList();
	return m;
}

void uiFreeTableModel(uiTableModel *m)
{
	delete m->tables;
	uiprivFree(m);
}

void uiTableModelRowInserted(uiTableModel *m, int newIndex)
{
	for (int32 i = 0; i < m->tables->CountItems(); i++)
		uiprivTableRowInserted((uiTable *) m->tables->ItemAt(i), newIndex);
}

void uiTableModelRowChanged(uiTableModel *m, int index)
{
	for (int32 i = 0; i < m->tables->CountItems(); i++)
		uiprivTableRowChanged((uiTable *) m->tables->ItemAt(i), index);
}

void uiTableModelRowDeleted(uiTableModel *m, int oldIndex)
{
	for (int32 i = 0; i < m->tables->CountItems(); i++)
		uiprivTableRowDeleted((uiTable *) m->tables->ItemAt(i), oldIndex);
}
