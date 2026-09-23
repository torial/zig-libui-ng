// 22 september 2026 -- uiTree (torial fork): what every backend shares.

#ifdef __cplusplus
extern "C" {
#endif

// each backend defines struct uiTreeModel (it is a GObject on GTK) and provides:
extern uiTreeModelHandler *uiprivTreeModelHandler(uiTreeModel *m);
// treemodel.c (common): the handler calls, through the accessor above
extern int uiprivTreeModelNumChildren(uiTreeModel *m, void *parent);
extern void *uiprivTreeModelChild(uiTreeModel *m, void *parent, int index);
extern const char *uiprivTreeModelText(uiTreeModel *m, void *node);
extern int uiprivTreeModelHasChildren(uiTreeModel *m, void *node);
extern uiImage *uiprivTreeModelIcon(uiTreeModel *m, void *node);   // NULL when the handler has no Icon or returns none

#ifdef __cplusplus
}
#endif
