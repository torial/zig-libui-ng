// 22 september 2026 -- uiTree on GTK (torial fork): a GtkTreeView, one text column, no
// header, over the hierarchical uiTreeModel in treemodel.c.
#include "uipriv_unix.h"
#include "tree.h"

struct uiTree {
	uiUnixControl c;
	GtkWidget *widget;
	GtkContainer *scontainer;
	GtkScrolledWindow *sw;
	GtkWidget *treeWidget;
	GtkTreeView *tv;
	uiTreeModel *model;
	gboolean inSet;
	gboolean userAct;	// inside a mouse/key press on the view: a selection is the user's
	void (*onSelectionChanged)(uiTree *, void *);
	void *onSelectionChangedData;
	void (*onNodeActivated)(uiTree *, void *, void *);
	void *onNodeActivatedData;
	void (*onNodeExpanded)(uiTree *, void *, int, void *);
	void *onNodeExpandedData;
};

uiUnixControlAllDefaultsExceptDestroy(uiTree)

static void uiTreeDestroy(uiControl *c)
{
	uiTree *t = uiTree(c);

	g_ptr_array_remove(t->model->trees, t);
	g_object_unref(t->widget);
	uiFreeControl(uiControl(t));
}

static void defaultOnSelectionChanged(uiTree *t, void *data) {}
static void defaultOnNodeActivated(uiTree *t, void *node, void *data) {}
static void defaultOnNodeExpanded(uiTree *t, void *node, int expanded, void *data) {}

void uiTreeOnSelectionChanged(uiTree *t, void (*f)(uiTree *, void *), void *data) { t->onSelectionChanged = f; t->onSelectionChangedData = data; }
void uiTreeOnNodeActivated(uiTree *t, void (*f)(uiTree *, void *, void *), void *data) { t->onNodeActivated = f; t->onNodeActivatedData = data; }
void uiTreeOnNodeExpanded(uiTree *t, void (*f)(uiTree *, void *, int, void *), void *data) { t->onNodeExpanded = f; t->onNodeExpandedData = data; }

// GTK selects the first row by itself when the view first takes keyboard focus
// (gtk_tree_view_focus_to_cursor), which reached the app as an onSelectionChanged
// nobody clicked for. A selection is allowed only while a press is being handled
// or while uiTreeSetSelection runs; the focus-time one is refused, so the view
// shows nothing selected and says nothing -- the same as the Windows control.
static gboolean selectFunc(GtkTreeSelection *s, GtkTreeModel *m, GtkTreePath *path, gboolean cur, gpointer data)
{
	uiTree *t = uiTree(data);

	if (cur)		// unselecting is always fine
		return TRUE;
	return t->userAct || t->inSet;
}

static gboolean onPress(GtkWidget *w, GdkEvent *e, gpointer data)
{
	uiTree(data)->userAct = TRUE;
	return FALSE;
}

static gboolean onRelease(GtkWidget *w, GdkEvent *e, gpointer data)
{
	uiTree(data)->userAct = FALSE;
	return FALSE;
}

static void onSelectionChanged(GtkTreeSelection *s, gpointer data)
{
	uiTree *t = uiTree(data);

	if (t->inSet)
		return;
	(*(t->onSelectionChanged))(t, t->onSelectionChangedData);
}

static void onRowActivated(GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *col, gpointer data)
{
	uiTree *t = uiTree(data);
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(t->model), &iter, path))
		return;
	(*(t->onNodeActivated))(t, iter.user_data, t->onNodeActivatedData);
}

static void onRowExpanded(GtkTreeView *tv, GtkTreeIter *iter, GtkTreePath *path, gpointer data)
{
	uiTree *t = uiTree(data);

	if (t->inSet)
		return;
	(*(t->onNodeExpanded))(t, iter->user_data, 1, t->onNodeExpandedData);
}

static void onRowCollapsed(GtkTreeView *tv, GtkTreeIter *iter, GtkTreePath *path, gpointer data)
{
	uiTree *t = uiTree(data);

	if (t->inSet)
		return;
	(*(t->onNodeExpanded))(t, iter->user_data, 0, t->onNodeExpandedData);
}

void uiTreeSetExpanded(uiTree *t, void *node, int expanded)
{
	GtkTreePath *path;

	path = uiprivTreeModelPathFor(t->model, node);
	if (path == NULL)
		return;
	t->inSet = TRUE;
	if (expanded)
		gtk_tree_view_expand_to_path(t->tv, path);   // opens the ancestors too
	else
		gtk_tree_view_collapse_row(t->tv, path);
	t->inSet = FALSE;
	gtk_tree_path_free(path);
}

int uiTreeExpanded(uiTree *t, void *node)
{
	GtkTreePath *path;
	gboolean r;

	path = uiprivTreeModelPathFor(t->model, node);
	if (path == NULL)
		return 0;
	r = gtk_tree_view_row_expanded(t->tv, path);
	gtk_tree_path_free(path);
	return r ? 1 : 0;
}

void *uiTreeSelection(uiTree *t)
{
	GtkTreeSelection *s;
	GtkTreeIter iter;

	s = gtk_tree_view_get_selection(t->tv);
	if (!gtk_tree_selection_get_selected(s, NULL, &iter))
		return NULL;
	return iter.user_data;
}

void uiTreeSetSelection(uiTree *t, void *node)
{
	GtkTreeSelection *s;
	GtkTreePath *path;

	s = gtk_tree_view_get_selection(t->tv);
	t->inSet = TRUE;
	if (node == NULL)
		gtk_tree_selection_unselect_all(s);
	else {
		path = uiprivTreeModelPathFor(t->model, node);
		if (path != NULL) {
			gtk_tree_view_expand_to_path(t->tv, path);
			gtk_tree_selection_select_path(s, path);
			gtk_tree_path_free(path);
		}
	}
	t->inSet = FALSE;
}

uiTree *uiNewTree(uiTreeModel *m)
{
	uiTree *t;
	GtkTreeViewColumn *col;
	GtkCellRenderer *r;
	GtkTreeSelection *selection;

	uiUnixNewControl(uiTree, t);

	t->model = m;
	t->inSet = FALSE;
	t->userAct = FALSE;

	t->widget = gtk_scrolled_window_new(NULL, NULL);
	t->scontainer = GTK_CONTAINER(t->widget);
	t->sw = GTK_SCROLLED_WINDOW(t->widget);
	gtk_scrolled_window_set_shadow_type(t->sw, GTK_SHADOW_IN);

	t->treeWidget = gtk_tree_view_new_with_model(GTK_TREE_MODEL(m));
	t->tv = GTK_TREE_VIEW(t->treeWidget);
	gtk_tree_view_set_headers_visible(t->tv, FALSE);
	col = gtk_tree_view_column_new();
	r = gtk_cell_renderer_pixbuf_new();		// the icon, 2026-09-23; an empty pixbuf renders nothing
	gtk_tree_view_column_pack_start(col, r, FALSE);
	gtk_tree_view_column_add_attribute(col, r, "pixbuf", 1);
	r = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, r, TRUE);
	gtk_tree_view_column_add_attribute(col, r, "text", 0);
	gtk_tree_view_append_column(t->tv, col);

	uiTreeOnSelectionChanged(t, defaultOnSelectionChanged, NULL);
	uiTreeOnNodeActivated(t, defaultOnNodeActivated, NULL);
	uiTreeOnNodeExpanded(t, defaultOnNodeExpanded, NULL);

	selection = gtk_tree_view_get_selection(t->tv);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	gtk_tree_selection_set_select_function(selection, selectFunc, t, NULL);
	g_signal_connect(selection, "changed", G_CALLBACK(onSelectionChanged), t);
	g_signal_connect(t->tv, "button-press-event", G_CALLBACK(onPress), t);
	g_signal_connect(t->tv, "key-press-event", G_CALLBACK(onPress), t);
	g_signal_connect(t->tv, "button-release-event", G_CALLBACK(onRelease), t);
	g_signal_connect(t->tv, "key-release-event", G_CALLBACK(onRelease), t);
	g_signal_connect(t->tv, "row-activated", G_CALLBACK(onRowActivated), t);
	g_signal_connect(t->tv, "row-expanded", G_CALLBACK(onRowExpanded), t);
	g_signal_connect(t->tv, "row-collapsed", G_CALLBACK(onRowCollapsed), t);

	gtk_container_add(t->scontainer, t->treeWidget);
	gtk_widget_show(t->treeWidget);

	g_ptr_array_add(m->trees, t);
	return t;
}
