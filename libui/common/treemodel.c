// 22 september 2026 -- uiTreeModel (torial fork): the handler calls every backend shares.
#include "../ui.h"
#include "uipriv.h"
#include "tree.h"

int uiprivTreeModelNumChildren(uiTreeModel *m, void *parent)
{
	uiTreeModelHandler *mh = uiprivTreeModelHandler(m);

	return (*(mh->NumChildren))(mh, m, parent);
}

void *uiprivTreeModelChild(uiTreeModel *m, void *parent, int index)
{
	uiTreeModelHandler *mh = uiprivTreeModelHandler(m);

	return (*(mh->Child))(mh, m, parent, index);
}

const char *uiprivTreeModelText(uiTreeModel *m, void *node)
{
	uiTreeModelHandler *mh = uiprivTreeModelHandler(m);

	return (*(mh->Text))(mh, m, node);
}

int uiprivTreeModelHasChildren(uiTreeModel *m, void *node)
{
	uiTreeModelHandler *mh = uiprivTreeModelHandler(m);

	return (*(mh->HasChildren))(mh, m, node);
}

uiImage *uiprivTreeModelIcon(uiTreeModel *m, void *node)
{
	uiTreeModelHandler *mh = uiprivTreeModelHandler(m);

	if (mh->Icon == NULL)
		return NULL;
	return (*(mh->Icon))(mh, m, node);
}
