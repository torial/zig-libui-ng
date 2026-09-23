// 22 september 2026 -- uiTree on NSOutlineView (torial fork).
// WRITTEN BLIND: no macOS build has compiled or run this yet. The NSOutlineView data-source
// protocol maps onto the handler one-to-one; the one wrinkle is that Cocoa items must be
// Objective-C objects compared by pointer, so each app node gets ONE stable NSValue wrapper
// kept in a dictionary for the life of the model.
#import "uipriv_darwin.h"
#import "../common/tree.h"

@class uiprivTreeModel;
struct uiTreeModel {
	uiTreeModelHandler *mh;
	uiprivTreeModel *m;
	NSMutableArray *trees;
	NSMutableDictionary *items;   // NSNumber(pointer) -> NSValue wrapper
};
struct uiTree {
	uiDarwinControl c;
	NSScrollView *sv;
	NSOutlineView *ov;
	uiprivScrollViewData *d;
	uiTreeModel *m;
	BOOL inSet;
	void (*onSelectionChanged)(uiTree *, void *);
	void *onSelectionChangedData;
	void (*onNodeActivated)(uiTree *, void *, void *);
	void *onNodeActivatedData;
	void (*onNodeExpanded)(uiTree *, void *, int, void *);
	void *onNodeExpandedData;
};

static id itemFor(uiTreeModel *m, void *node)
{
	NSNumber *key;
	NSValue *v;

	if (node == NULL)
		return nil;
	key = [NSNumber numberWithUnsignedLongLong:(unsigned long long) node];
	v = [m->items objectForKey:key];
	if (v == nil) {
		v = [NSValue valueWithPointer:node];
		[m->items setObject:v forKey:key];
	}
	return v;
}

static void *nodeOf(id item)
{
	if (item == nil)
		return NULL;
	return [((NSValue *) item) pointerValue];
}

@interface uiprivTreeModel : NSObject<NSOutlineViewDataSource, NSOutlineViewDelegate> {
	uiTreeModel *m;
}
- (id)initWithModel:(uiTreeModel *)model;
@end

@interface uiprivOutlineView : NSOutlineView {
	uiTree *uiprivT;
}
- (id)initWithFrame:(NSRect)r uiprivT:(uiTree *)t;
- (void)onDoubleClicked:(id)sender;
@end

@implementation uiprivOutlineView
- (id)initWithFrame:(NSRect)r uiprivT:(uiTree *)t
{
	self = [super initWithFrame:r];
	if (self)
		self->uiprivT = t;
	return self;
}
- (void)onDoubleClicked:(id)sender
{
	uiTree *t = self->uiprivT;
	NSInteger row = [self clickedRow];
	id item;

	if (row < 0)
		return;
	item = [self itemAtRow:row];
	(*(t->onNodeActivated))(t, nodeOf(item), t->onNodeActivatedData);
}
@end

@implementation uiprivTreeModel
- (id)initWithModel:(uiTreeModel *)model
{
	self = [super init];
	if (self)
		self->m = model;
	return self;
}
- (NSInteger)outlineView:(NSOutlineView *)ov numberOfChildrenOfItem:(id)item
{
	return uiprivTreeModelNumChildren(self->m, nodeOf(item));
}
- (id)outlineView:(NSOutlineView *)ov child:(NSInteger)index ofItem:(id)item
{
	return itemFor(self->m, uiprivTreeModelChild(self->m, nodeOf(item), (int) index));
}
- (BOOL)outlineView:(NSOutlineView *)ov isItemExpandable:(id)item
{
	return uiprivTreeModelHasChildren(self->m, nodeOf(item)) != 0;
}
- (id)outlineView:(NSOutlineView *)ov objectValueForTableColumn:(NSTableColumn *)col byItem:(id)item
{
	return [NSString stringWithUTF8String:uiprivTreeModelText(self->m, nodeOf(item))];
}
// View-based cells (2026-09-23, still blind): an NSTableCellView with an image view and a
// text field, so a node can carry the handler's icon. Implementing this delegate method is
// what switches NSOutlineView from cell-based to view-based; objectValueForTableColumn
// above still supplies the object value.
- (NSView *)outlineView:(NSOutlineView *)ov viewForTableColumn:(NSTableColumn *)col item:(id)item
{
	NSTableCellView *cv;
	NSTextField *tf;
	NSImageView *iv;
	uiImage *img;
	void *node = nodeOf(item);

	cv = [ov makeViewWithIdentifier:@"uiTreeCell" owner:self];
	if (cv == nil) {
		cv = [[[NSTableCellView alloc] initWithFrame:NSZeroRect] autorelease];
		[cv setIdentifier:@"uiTreeCell"];
		iv = [[NSImageView alloc] initWithFrame:NSMakeRect(0, 0, 16, 16)];
		[iv setImageScaling:NSImageScaleProportionallyDown];
		[cv addSubview:iv];
		[cv setImageView:iv];
		[iv release];
		tf = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 0, 200, 17)];
		[tf setBordered:NO];
		[tf setDrawsBackground:NO];
		[tf setEditable:NO];
		[[tf cell] setLineBreakMode:NSLineBreakByTruncatingTail];
		[cv addSubview:tf];
		[cv setTextField:tf];
		[tf release];
		[iv setAutoresizingMask:NSViewMinYMargin | NSViewMaxYMargin];
		[tf setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin | NSViewMaxYMargin];
	}
	[[cv textField] setStringValue:[NSString stringWithUTF8String:uiprivTreeModelText(self->m, node)]];
	img = uiprivTreeModelIcon(self->m, node);
	if (img != NULL) {
		[[cv imageView] setImage:uiprivImageNSImage(img)];
		[[cv imageView] setHidden:NO];
		[[cv textField] setFrame:NSMakeRect(20, 0, [cv bounds].size.width - 20, [cv bounds].size.height)];
	} else {
		[[cv imageView] setImage:nil];
		[[cv imageView] setHidden:YES];
		[[cv textField] setFrame:NSMakeRect(0, 0, [cv bounds].size.width, [cv bounds].size.height)];
	}
	return cv;
}
- (void)outlineViewSelectionDidChange:(NSNotification *)note
{
	uiprivOutlineView *ov = (uiprivOutlineView *) [note object];
	uiTree *t = ov->uiprivT;

	if (t->inSet)
		return;
	(*(t->onSelectionChanged))(t, t->onSelectionChangedData);
}
- (void)outlineViewItemDidExpand:(NSNotification *)note
{
	uiprivOutlineView *ov = (uiprivOutlineView *) [note object];
	uiTree *t = ov->uiprivT;
	id item = [[note userInfo] objectForKey:@"NSObject"];

	if (t->inSet)
		return;
	(*(t->onNodeExpanded))(t, nodeOf(item), 1, t->onNodeExpandedData);
}
- (void)outlineViewItemDidCollapse:(NSNotification *)note
{
	uiprivOutlineView *ov = (uiprivOutlineView *) [note object];
	uiTree *t = ov->uiprivT;
	id item = [[note userInfo] objectForKey:@"NSObject"];

	if (t->inSet)
		return;
	(*(t->onNodeExpanded))(t, nodeOf(item), 0, t->onNodeExpandedData);
}
@end

uiTreeModel *uiNewTreeModel(uiTreeModelHandler *mh)
{
	uiTreeModel *m;

	m = uiprivNew(uiTreeModel);
	m->mh = mh;
	m->m = [[uiprivTreeModel alloc] initWithModel:m];
	m->trees = [NSMutableArray new];
	m->items = [NSMutableDictionary new];
	return m;
}

void uiFreeTreeModel(uiTreeModel *m)
{
	if ([m->trees count] != 0)
		uiprivUserBug("You cannot free a uiTreeModel while uiTrees are using it.");
	[m->items release];
	[m->trees release];
	[m->m release];
	uiprivFree(m);
}

uiTreeModelHandler *uiprivTreeModelHandler(uiTreeModel *m)
{
	return m->mh;
}

void uiTreeModelNodeInserted(uiTreeModel *m, void *parent, int index)
{
	NSOutlineView *ov;

	for (ov in m->trees)
		[ov insertItemsAtIndexes:[NSIndexSet indexSetWithIndex:index] inParent:itemFor(m, parent) withAnimation:NSTableViewAnimationEffectNone];
}

void uiTreeModelNodeDeleted(uiTreeModel *m, void *parent, int index)
{
	NSOutlineView *ov;

	for (ov in m->trees)
		[ov removeItemsAtIndexes:[NSIndexSet indexSetWithIndex:index] inParent:itemFor(m, parent) withAnimation:NSTableViewAnimationEffectNone];
}

void uiTreeModelNodeChanged(uiTreeModel *m, void *node)
{
	NSOutlineView *ov;

	for (ov in m->trees)
		[ov reloadItem:itemFor(m, node)];
}

uiDarwinControlAllDefaultsExceptDestroy(uiTree, sv)

static void uiTreeDestroy(uiControl *c)
{
	uiTree *t = uiTree(c);

	[t->m->trees removeObject:t->ov];
	uiprivScrollViewFreeData(t->sv, t->d);
	[t->ov release];
	[t->sv release];
	uiFreeControl(uiControl(t));
}

static void defaultOnSelectionChanged(uiTree *t, void *data) {}
static void defaultOnNodeActivated(uiTree *t, void *node, void *data) {}
static void defaultOnNodeExpanded(uiTree *t, void *node, int expanded, void *data) {}

void uiTreeOnSelectionChanged(uiTree *t, void (*f)(uiTree *, void *), void *data) { t->onSelectionChanged = f; t->onSelectionChangedData = data; }
void uiTreeOnNodeActivated(uiTree *t, void (*f)(uiTree *, void *, void *), void *data) { t->onNodeActivated = f; t->onNodeActivatedData = data; }
void uiTreeOnNodeExpanded(uiTree *t, void (*f)(uiTree *, void *, int, void *), void *data) { t->onNodeExpanded = f; t->onNodeExpandedData = data; }

void uiTreeSetExpanded(uiTree *t, void *node, int expanded)
{
	id item = itemFor(t->m, node);

	t->inSet = YES;
	if (expanded)
		[t->ov expandItem:item];
	else
		[t->ov collapseItem:item];
	t->inSet = NO;
}

int uiTreeExpanded(uiTree *t, void *node)
{
	return [t->ov isItemExpanded:itemFor(t->m, node)] ? 1 : 0;
}

void *uiTreeSelection(uiTree *t)
{
	NSInteger row = [t->ov selectedRow];

	if (row < 0)
		return NULL;
	return nodeOf([t->ov itemAtRow:row]);
}

void uiTreeSetSelection(uiTree *t, void *node)
{
	NSInteger row;

	t->inSet = YES;
	if (node == NULL)
		[t->ov deselectAll:nil];
	else {
		row = [t->ov rowForItem:itemFor(t->m, node)];
		if (row >= 0)
			[t->ov selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
	}
	t->inSet = NO;
}

uiTree *uiNewTree(uiTreeModel *m)
{
	uiTree *t;
	uiprivScrollViewCreateParams sp;
	NSTableColumn *col;

	uiDarwinNewControl(uiTree, t);
	t->m = m;
	t->inSet = NO;

	t->ov = [[uiprivOutlineView alloc] initWithFrame:NSZeroRect uiprivT:t];
	col = [[NSTableColumn alloc] initWithIdentifier:@"tree"];
	[col setEditable:NO];
	[t->ov addTableColumn:col];
	[t->ov setOutlineTableColumn:col];
	[col release];
	[t->ov setHeaderView:nil];
	[t->ov setDataSource:m->m];
	[t->ov setDelegate:m->m];
	[t->ov setAllowsMultipleSelection:NO];
	[t->ov setAllowsEmptySelection:YES];
	[t->ov reloadData];
	[m->trees addObject:t->ov];

	uiTreeOnSelectionChanged(t, defaultOnSelectionChanged, NULL);
	uiTreeOnNodeActivated(t, defaultOnNodeActivated, NULL);
	uiTreeOnNodeExpanded(t, defaultOnNodeExpanded, NULL);
	[t->ov setDoubleAction:@selector(onDoubleClicked:)];

	memset(&sp, 0, sizeof (uiprivScrollViewCreateParams));
	sp.DocumentView = t->ov;
	sp.BackgroundColor = [NSColor colorWithCalibratedWhite:1.0 alpha:1.0];
	sp.DrawsBackground = YES;
	sp.Bordered = YES;
	sp.HScroll = YES;
	sp.VScroll = YES;
	t->sv = uiprivMkScrollView(&sp, &(t->d));
	[t->sv setWantsLayer:YES];

	return t;
}
