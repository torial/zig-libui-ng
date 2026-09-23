// 23 september 2026 -- uiToolbar on NSToolbar (torial fork).
// WRITTEN BLIND: no macOS build has compiled or run this yet (macOS is the second tier of
// this fork). NSToolbar lives in the title bar rather than under a menubar, which is where
// the platform puts toolbars; the item set is served through the delegate from the record
// kept here, and a change re-attaches the toolbar so Cocoa re-reads the identifiers.
#import "uipriv_darwin.h"

@class uiprivToolbarDelegate;

struct tbItem {
	char *label;		// NULL for a separator
	uiImage *icon;
	char *tooltip;
	BOOL enabled;
};

struct uiToolbar {
	NSToolbar *toolbar;
	uiprivToolbarDelegate *delegate;
	NSMutableArray *items;		// of NSValue(pointer to struct tbItem)
	NSWindow *window;			// nil until attached
	void (*onClicked)(uiToolbar *, int, void *);
	void *onClickedData;
};

@interface uiprivToolbarDelegate : NSObject<NSToolbarDelegate> {
	uiToolbar *t;
}
- (id)initWithToolbar:(uiToolbar *)tb;
- (void)itemClicked:(id)sender;
@end

static NSString *identifierFor(int index)
{
	return [NSString stringWithFormat:@"libui.toolbar.%d", index];
}

static int indexFor(NSString *identifier)
{
	if (![identifier hasPrefix:@"libui.toolbar."])
		return -1;
	return [[identifier substringFromIndex:[@"libui.toolbar." length]] intValue];
}

static struct tbItem *itemAt(uiToolbar *t, int index)
{
	if (index < 0 || (NSUInteger) index >= [t->items count])
		return NULL;
	return (struct tbItem *) [[t->items objectAtIndex:index] pointerValue];
}

@implementation uiprivToolbarDelegate

- (id)initWithToolbar:(uiToolbar *)tb
{
	self = [super init];
	if (self)
		self->t = tb;
	return self;
}

- (NSArray<NSToolbarItemIdentifier> *)identifiers
{
	NSMutableArray *a;
	NSUInteger i, n;
	struct tbItem *it;

	a = [NSMutableArray new];
	n = [self->t->items count];
	for (i = 0; i < n; i++) {
		it = itemAt(self->t, (int) i);
		if (it != NULL && it->label == NULL)
			[a addObject:NSToolbarSpaceItemIdentifier];
		else
			[a addObject:identifierFor((int) i)];
	}
	return [a autorelease];
}

- (NSArray<NSToolbarItemIdentifier> *)toolbarDefaultItemIdentifiers:(NSToolbar *)toolbar
{
	return [self identifiers];
}

- (NSArray<NSToolbarItemIdentifier> *)toolbarAllowedItemIdentifiers:(NSToolbar *)toolbar
{
	return [self identifiers];
}

- (NSToolbarItem *)toolbar:(NSToolbar *)toolbar itemForItemIdentifier:(NSToolbarItemIdentifier)identifier willBeInsertedIntoToolbar:(BOOL)flag
{
	NSToolbarItem *item;
	struct tbItem *it;
	int index;

	index = indexFor(identifier);
	it = itemAt(self->t, index);
	if (it == NULL || it->label == NULL)
		return nil;
	item = [[[NSToolbarItem alloc] initWithItemIdentifier:identifier] autorelease];
	[item setLabel:uiprivToNSString(it->label)];
	[item setPaletteLabel:uiprivToNSString(it->label)];
	if (it->tooltip != NULL)
		[item setToolTip:uiprivToNSString(it->tooltip)];
	if (it->icon != NULL)
		[item setImage:uiprivImageNSImage(it->icon)];
	[item setTarget:self];
	[item setAction:@selector(itemClicked:)];
	[item setTag:index];
	[item setEnabled:it->enabled];
	return item;
}

- (void)itemClicked:(id)sender
{
	int index;

	index = (int) [sender tag];
	if (itemAt(self->t, index) == NULL)
		return;
	(*(self->t->onClicked))(self->t, index, self->t->onClickedData);
}

// every item is enabled/disabled by the record, not by the responder chain
- (BOOL)validateToolbarItem:(NSToolbarItem *)item
{
	struct tbItem *it;

	it = itemAt(self->t, (int) [item tag]);
	if (it == NULL)
		return NO;
	return it->enabled;
}

@end

static void defaultOnClicked(uiToolbar *t, int index, void *data)
{
	// do nothing
}

static char *dupString(const char *s)
{
	char *out;
	size_t n;

	n = strlen(s) + 1;
	out = (char *) uiprivAlloc(n, "char[]");
	memcpy(out, s, n);
	return out;
}

uiToolbar *uiNewToolbar(void)
{
	uiToolbar *t;

	t = uiprivNew(uiToolbar);
	t->items = [NSMutableArray new];
	t->delegate = [[uiprivToolbarDelegate alloc] initWithToolbar:t];
	t->toolbar = [[NSToolbar alloc] initWithIdentifier:@"libui.toolbar"];
	[t->toolbar setDelegate:t->delegate];
	[t->toolbar setAllowsUserCustomization:NO];
	[t->toolbar setDisplayMode:NSToolbarDisplayModeIconAndLabel];
	t->window = nil;
	t->onClicked = defaultOnClicked;
	t->onClickedData = NULL;
	return t;
}

// NSToolbar reads the identifier list when it is set on a window; after a change we
// re-set it so the list is read again
static void sync(uiToolbar *t)
{
	if (t->window == nil)
		return;
	[t->window setToolbar:nil];
	[t->window setToolbar:t->toolbar];
}

int uiToolbarAppendItem(uiToolbar *t, const char *label, uiImage *icon, const char *tooltip)
{
	struct tbItem *it;

	it = uiprivNew(struct tbItem);
	it->label = dupString(label);
	it->icon = icon;
	it->tooltip = tooltip != NULL ? dupString(tooltip) : NULL;
	it->enabled = YES;
	[t->items addObject:[NSValue valueWithPointer:it]];
	sync(t);
	return (int) [t->items count] - 1;
}

int uiToolbarAppendSeparator(uiToolbar *t)
{
	struct tbItem *it;

	it = uiprivNew(struct tbItem);
	it->label = NULL;
	it->icon = NULL;
	it->tooltip = NULL;
	it->enabled = YES;
	[t->items addObject:[NSValue valueWithPointer:it]];
	sync(t);
	return (int) [t->items count] - 1;
}

int uiToolbarNumItems(uiToolbar *t)
{
	return (int) [t->items count];
}

static void freeItems(uiToolbar *t)
{
	NSUInteger i, n;
	struct tbItem *it;

	n = [t->items count];
	for (i = 0; i < n; i++) {
		it = itemAt(t, (int) i);
		if (it == NULL)
			continue;
		if (it->label != NULL)
			uiprivFree(it->label);
		if (it->tooltip != NULL)
			uiprivFree(it->tooltip);
		uiprivFree(it);
	}
	[t->items removeAllObjects];
}

void uiToolbarClear(uiToolbar *t)
{
	freeItems(t);
	sync(t);
}

void uiToolbarSetItemEnabled(uiToolbar *t, int index, int enabled)
{
	struct tbItem *it;

	it = itemAt(t, index);
	if (it == NULL)
		return;
	it->enabled = enabled ? YES : NO;
	if (t->window != nil)
		[t->toolbar validateVisibleItems];
}

void uiToolbarOnClicked(uiToolbar *t, void (*f)(uiToolbar *, int, void *), void *data)
{
	t->onClicked = f;
	t->onClickedData = data;
}

// window.m's half

void uiprivToolbarAttach(uiToolbar *t, NSWindow *w)
{
	t->window = w;
	[w setToolbar:t->toolbar];
}

void uiprivFreeToolbar(uiToolbar *t)
{
	if (t->window != nil)
		[t->window setToolbar:nil];
	freeItems(t);
	[t->items release];
	[t->toolbar release];
	[t->delegate release];
	uiprivFree(t);
}
