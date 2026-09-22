// 22 september 2026 -- uiTree on GTK (torial fork)
#include "../common/tree.h"

// treemodel.c: uiTreeModel is a GObject implementing GtkTreeModel over the handler.
// An iter carries the app's node pointer in user_data (NULL never appears: the root
// is "no iter"). GtkTreePath <-> node goes through the handler's Child/NumChildren.
#define uiTreeModelType (uiTreeModel_get_type())
#define uiTreeModel(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), uiTreeModelType, uiTreeModel))
typedef struct uiTreeModelClass uiTreeModelClass;
struct uiTreeModel {
	GObject parent_instance;
	gint stamp;
	uiTreeModelHandler *mh;
	GPtrArray *trees;
};
struct uiTreeModelClass {
	GObjectClass parent_class;
};
extern GType uiTreeModel_get_type(void);
extern GtkTreePath *uiprivTreeModelPathFor(uiTreeModel *m, void *node);
extern void *uiprivTreeModelParentOf(uiTreeModel *m, void *node, int *index);
