// 22 september 2026 -- uiTreeModel on GTK (torial fork): a hierarchical GtkTreeModel
// over the app's handler. unix/tablemodel.c is the flat template this follows.
#include "uipriv_unix.h"
#include "tree.h"

static void uiTreeModel_gtk_tree_model_interface_init(GtkTreeModelIface *iface);

G_DEFINE_TYPE_WITH_CODE(uiTreeModel, uiTreeModel, G_TYPE_OBJECT,
	G_IMPLEMENT_INTERFACE(GTK_TYPE_TREE_MODEL, uiTreeModel_gtk_tree_model_interface_init))

static void uiTreeModel_init(uiTreeModel *m)
{
	m->trees = g_ptr_array_new();
}

static void uiTreeModel_dispose(GObject *obj)
{
	G_OBJECT_CLASS(uiTreeModel_parent_class)->dispose(obj);
}

static void uiTreeModel_finalize(GObject *obj)
{
	uiTreeModel *m = uiTreeModel(obj);

	g_ptr_array_free(m->trees, TRUE);
	G_OBJECT_CLASS(uiTreeModel_parent_class)->finalize(obj);
}

static GtkTreeModelFlags uiTreeModel_get_flags(GtkTreeModel *mm)
{
	return GTK_TREE_MODEL_ITERS_PERSIST;
}

static gint uiTreeModel_get_n_columns(GtkTreeModel *mm)
{
	return 1;
}

static GType uiTreeModel_get_column_type(GtkTreeModel *mm, gint index)
{
	return G_TYPE_STRING;
}

// the parent of `node`, found by searching -- the handler has no Parent(); a tree
// walk from the root is O(n) per call and these calls are rare (notifications, paths)
static int findParent(uiTreeModel *m, void *parent, void *node, void **outParent, int *outIndex)
{
	int n, i;
	void *c;

	n = uiprivTreeModelNumChildren(m, parent);
	for (i = 0; i < n; i++) {
		c = uiprivTreeModelChild(m, parent, i);
		if (c == node) {
			*outParent = parent;
			*outIndex = i;
			return 1;
		}
		if (uiprivTreeModelHasChildren(m, c) && findParent(m, c, node, outParent, outIndex))
			return 1;
	}
	return 0;
}

void *uiprivTreeModelParentOf(uiTreeModel *m, void *node, int *index)
{
	void *p = NULL;
	int i = -1;

	if (node == NULL || !findParent(m, NULL, node, &p, &i)) {
		*index = -1;
		return NULL;
	}
	*index = i;
	return p;
}

GtkTreePath *uiprivTreeModelPathFor(uiTreeModel *m, void *node)
{
	GtkTreePath *path;
	void *p;
	int i;
	gint indices[64];
	int depth = 0;

	// climb to the root collecting indices, then reverse
	while (node != NULL && depth < 64) {
		p = uiprivTreeModelParentOf(m, node, &i);
		if (i < 0)
			return NULL;
		indices[depth++] = i;
		node = p;
	}
	path = gtk_tree_path_new();
	while (depth > 0)
		gtk_tree_path_append_index(path, indices[--depth]);
	return path;
}

static gboolean uiTreeModel_get_iter(GtkTreeModel *mm, GtkTreeIter *iter, GtkTreePath *path)
{
	uiTreeModel *m = uiTreeModel(mm);
	gint *indices, depth, d, n;
	void *node = NULL;

	depth = gtk_tree_path_get_depth(path);
	indices = gtk_tree_path_get_indices(path);
	for (d = 0; d < depth; d++) {
		n = uiprivTreeModelNumChildren(m, node);
		if (indices[d] < 0 || indices[d] >= n)
			goto bad;
		node = uiprivTreeModelChild(m, node, indices[d]);
	}
	if (node == NULL)
		goto bad;
	iter->stamp = m->stamp;
	iter->user_data = node;
	return TRUE;
bad:
	iter->stamp = 0;
	return FALSE;
}

static GtkTreePath *uiTreeModel_get_path(GtkTreeModel *mm, GtkTreeIter *iter)
{
	uiTreeModel *m = uiTreeModel(mm);

	g_return_val_if_fail(iter->stamp == m->stamp, NULL);
	return uiprivTreeModelPathFor(m, iter->user_data);
}

static void uiTreeModel_get_value(GtkTreeModel *mm, GtkTreeIter *iter, gint column, GValue *value)
{
	uiTreeModel *m = uiTreeModel(mm);

	g_return_if_fail(iter->stamp == m->stamp);
	g_value_init(value, G_TYPE_STRING);
	g_value_set_string(value, uiprivTreeModelText(m, iter->user_data));
}

static gboolean uiTreeModel_iter_next(GtkTreeModel *mm, GtkTreeIter *iter)
{
	uiTreeModel *m = uiTreeModel(mm);
	void *p;
	int i, n;

	g_return_val_if_fail(iter->stamp == m->stamp, FALSE);
	p = uiprivTreeModelParentOf(m, iter->user_data, &i);
	if (i < 0)
		goto bad;
	n = uiprivTreeModelNumChildren(m, p);
	if (i + 1 >= n)
		goto bad;
	iter->user_data = uiprivTreeModelChild(m, p, i + 1);
	return TRUE;
bad:
	iter->stamp = 0;
	return FALSE;
}

static gboolean uiTreeModel_iter_previous(GtkTreeModel *mm, GtkTreeIter *iter)
{
	uiTreeModel *m = uiTreeModel(mm);
	void *p;
	int i;

	g_return_val_if_fail(iter->stamp == m->stamp, FALSE);
	p = uiprivTreeModelParentOf(m, iter->user_data, &i);
	if (i <= 0) {
		iter->stamp = 0;
		return FALSE;
	}
	iter->user_data = uiprivTreeModelChild(m, p, i - 1);
	return TRUE;
}

static gboolean uiTreeModel_iter_nth_child(GtkTreeModel *mm, GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
	uiTreeModel *m = uiTreeModel(mm);
	void *p = NULL;

	if (parent != NULL) {
		if (parent->stamp != m->stamp)
			goto bad;
		p = parent->user_data;
	}
	if (n < 0 || n >= uiprivTreeModelNumChildren(m, p))
		goto bad;
	iter->stamp = m->stamp;
	iter->user_data = uiprivTreeModelChild(m, p, n);
	return TRUE;
bad:
	iter->stamp = 0;
	return FALSE;
}

static gboolean uiTreeModel_iter_children(GtkTreeModel *mm, GtkTreeIter *iter, GtkTreeIter *parent)
{
	return uiTreeModel_iter_nth_child(mm, iter, parent, 0);
}

static gboolean uiTreeModel_iter_has_child(GtkTreeModel *mm, GtkTreeIter *iter)
{
	uiTreeModel *m = uiTreeModel(mm);

	g_return_val_if_fail(iter->stamp == m->stamp, FALSE);
	return uiprivTreeModelHasChildren(m, iter->user_data) != 0;
}

static gint uiTreeModel_iter_n_children(GtkTreeModel *mm, GtkTreeIter *iter)
{
	uiTreeModel *m = uiTreeModel(mm);

	if (iter == NULL)
		return uiprivTreeModelNumChildren(m, NULL);
	g_return_val_if_fail(iter->stamp == m->stamp, 0);
	return uiprivTreeModelNumChildren(m, iter->user_data);
}

static gboolean uiTreeModel_iter_parent(GtkTreeModel *mm, GtkTreeIter *iter, GtkTreeIter *child)
{
	uiTreeModel *m = uiTreeModel(mm);
	void *p;
	int i;

	g_return_val_if_fail(child->stamp == m->stamp, FALSE);
	p = uiprivTreeModelParentOf(m, child->user_data, &i);
	if (p == NULL) {
		iter->stamp = 0;
		return FALSE;
	}
	iter->stamp = m->stamp;
	iter->user_data = p;
	return TRUE;
}

static void uiTreeModel_class_init(uiTreeModelClass *class)
{
	G_OBJECT_CLASS(class)->dispose = uiTreeModel_dispose;
	G_OBJECT_CLASS(class)->finalize = uiTreeModel_finalize;
}

static void uiTreeModel_gtk_tree_model_interface_init(GtkTreeModelIface *iface)
{
	iface->get_flags = uiTreeModel_get_flags;
	iface->get_n_columns = uiTreeModel_get_n_columns;
	iface->get_column_type = uiTreeModel_get_column_type;
	iface->get_iter = uiTreeModel_get_iter;
	iface->get_path = uiTreeModel_get_path;
	iface->get_value = uiTreeModel_get_value;
	iface->iter_next = uiTreeModel_iter_next;
	iface->iter_previous = uiTreeModel_iter_previous;
	iface->iter_children = uiTreeModel_iter_children;
	iface->iter_has_child = uiTreeModel_iter_has_child;
	iface->iter_n_children = uiTreeModel_iter_n_children;
	iface->iter_nth_child = uiTreeModel_iter_nth_child;
	iface->iter_parent = uiTreeModel_iter_parent;
}

uiTreeModel *uiNewTreeModel(uiTreeModelHandler *mh)
{
	uiTreeModel *m;

	m = uiTreeModel(g_object_new(uiTreeModelType, NULL));
	while ((m->stamp = g_random_int()) == 0) {
	}
	m->mh = mh;
	return m;
}

void uiFreeTreeModel(uiTreeModel *m)
{
	if (m->trees->len != 0)
		uiprivUserBug("You cannot free a uiTreeModel while uiTrees are using it.");
	g_object_unref(m);
}

uiTreeModelHandler *uiprivTreeModelHandler(uiTreeModel *m)
{
	return m->mh;
}

// notifications: the app has ALREADY changed its data when it calls these, so the
// path for an inserted node is (parent path + index) and the node is Child(parent, index)
static GtkTreePath *childPath(uiTreeModel *m, void *parent, int index)
{
	GtkTreePath *path;

	if (parent == NULL)
		path = gtk_tree_path_new();
	else {
		path = uiprivTreeModelPathFor(m, parent);
		if (path == NULL)
			return NULL;
	}
	gtk_tree_path_append_index(path, index);
	return path;
}

void uiTreeModelNodeInserted(uiTreeModel *m, void *parent, int index)
{
	GtkTreePath *path;
	GtkTreeIter iter;

	path = childPath(m, parent, index);
	if (path == NULL)
		return;
	iter.stamp = m->stamp;
	iter.user_data = uiprivTreeModelChild(m, parent, index);
	gtk_tree_model_row_inserted(GTK_TREE_MODEL(m), path, &iter);
	if (parent != NULL && uiprivTreeModelNumChildren(m, parent) == 1) {
		// the parent just gained its first child: tell the view it has children now
		GtkTreePath *ppath = uiprivTreeModelPathFor(m, parent);
		GtkTreeIter piter;
		if (ppath != NULL) {
			piter.stamp = m->stamp;
			piter.user_data = parent;
			gtk_tree_model_row_has_child_toggled(GTK_TREE_MODEL(m), ppath, &piter);
			gtk_tree_path_free(ppath);
		}
	}
	gtk_tree_path_free(path);
}

void uiTreeModelNodeDeleted(uiTreeModel *m, void *parent, int index)
{
	GtkTreePath *path;

	path = childPath(m, parent, index);
	if (path == NULL)
		return;
	gtk_tree_model_row_deleted(GTK_TREE_MODEL(m), path);
	gtk_tree_path_free(path);
}

void uiTreeModelNodeChanged(uiTreeModel *m, void *node)
{
	GtkTreePath *path;
	GtkTreeIter iter;

	path = uiprivTreeModelPathFor(m, node);
	if (path == NULL)
		return;
	iter.stamp = m->stamp;
	iter.user_data = node;
	gtk_tree_model_row_changed(GTK_TREE_MODEL(m), path, &iter);
	gtk_tree_path_free(path);
}
