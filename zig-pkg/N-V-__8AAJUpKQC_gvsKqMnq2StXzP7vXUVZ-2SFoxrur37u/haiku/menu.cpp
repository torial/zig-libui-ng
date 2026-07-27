// libui-ng Haiku backend — menus (BMenuBar / BMenu / BMenuItem).
//
// Like the other backends, the menu model is GLOBAL: uiNewMenu / uiMenuAppend* build an in-memory
// description, which freezes ("finalizes") when the first window materializes it. Each window with a
// menubar gets its own native BMenuBar built from that description; a uiMenuItem therefore tracks
// every BMenuItem it spawned (across windows) so checked/enabled changes stay in sync. Menu clicks
// arrive at the owning window's looper as a 'uiMN' message carrying the uiMenuItem*.
#include "uipriv_haiku.h"

enum {
	typeRegular,
	typeCheckbox,
	typeQuit,
	typePreferences,
	typeAbout,
	typeSeparator,
};

struct uiMenu {
	char *name;
	BList *items;			// uiMenuItem*
};

struct uiMenuItem {
	char *name;
	int type;
	void (*onClicked)(uiMenuItem *, uiWindow *, void *);
	void *onClickedData;
	bool disabled;
	bool checked;
	BList *instances;		// BMenuItem* — one per window this item was materialized into
};

static BList *menus = NULL;		// uiMenu*
static bool menusFinalized = false;
static bool hasQuit = false;
static bool hasPreferences = false;
static bool hasAbout = false;

static void lockItemWindow(BMenuItem *mi, bool *locked, BWindow **win)
{
	*win = (mi->Menu() != NULL) ? mi->Menu()->Window() : NULL;
	*locked = (*win != NULL) && (*win)->Lock();
}

static void setChecked(uiMenuItem *item, bool checked)
{
	item->checked = checked;
	for (int32 i = 0; i < item->instances->CountItems(); i++) {
		BMenuItem *mi = (BMenuItem *) item->instances->ItemAt(i);
		bool locked; BWindow *win;
		lockItemWindow(mi, &locked, &win);
		mi->SetMarked(checked);
		if (locked) win->Unlock();
	}
}

static void defaultOnClicked(uiMenuItem *item, uiWindow *w, void *data)
{
	(void) item; (void) w; (void) data;
}

static void onQuitClicked(uiMenuItem *item, uiWindow *w, void *data)
{
	(void) item; (void) w; (void) data;
	if (uiprivShouldQuit())
		uiQuit();
}

// Called from the window looper (window.cpp) when a 'uiMN' message arrives.
void uiprivMenuItemClicked(uiMenuItem *item, uiWindow *w)
{
	if (item->type == typeCheckbox)
		setChecked(item, !item->checked);
	(*(item->onClicked))(item, w, item->onClickedData);
}

static void menuItemEnableDisable(uiMenuItem *item, bool enabled)
{
	item->disabled = !enabled;
	for (int32 i = 0; i < item->instances->CountItems(); i++) {
		BMenuItem *mi = (BMenuItem *) item->instances->ItemAt(i);
		bool locked; BWindow *win;
		lockItemWindow(mi, &locked, &win);
		mi->SetEnabled(enabled);
		if (locked) win->Unlock();
	}
}

void uiMenuItemEnable(uiMenuItem *item) { menuItemEnableDisable(item, true); }
void uiMenuItemDisable(uiMenuItem *item) { menuItemEnableDisable(item, false); }

void uiMenuItemOnClicked(uiMenuItem *item, void (*f)(uiMenuItem *, uiWindow *, void *), void *data)
{
	if (item->type == typeQuit)
		uiprivUserBug("You cannot call uiMenuItemOnClicked() on a Quit item; use uiOnShouldQuit() instead.");
	item->onClicked = f;
	item->onClickedData = data;
}

int uiMenuItemChecked(uiMenuItem *item) { return item->checked ? 1 : 0; }

void uiMenuItemSetChecked(uiMenuItem *item, int checked)
{
	setChecked(item, checked ? true : false);
}

static uiMenuItem *newItem(uiMenu *m, int type, const char *name)
{
	if (menusFinalized)
		uiprivUserBug("You cannot create a new menu item after menus have been finalized.");

	uiMenuItem *item = uiprivNew(uiMenuItem);
	m->items->AddItem(item);
	item->type = type;

	switch (type) {
	case typeQuit:        item->name = uiHaikuStrdupText("Quit"); break;
	case typePreferences: item->name = uiHaikuStrdupText("Preferences..."); break;
	case typeAbout:       item->name = uiHaikuStrdupText("About"); break;
	case typeSeparator:   item->name = NULL; break;
	default:              item->name = uiHaikuStrdupText(name); break;
	}

	if (type == typeQuit) {
		item->onClicked = onQuitClicked;	// can't go through uiMenuItemOnClicked() (it rejects Quit)
		item->onClickedData = NULL;
	} else {
		item->onClicked = defaultOnClicked;
		item->onClickedData = NULL;
	}

	item->disabled = false;
	item->checked = false;
	item->instances = new BList();
	return item;
}

uiMenuItem *uiMenuAppendItem(uiMenu *m, const char *name) { return newItem(m, typeRegular, name); }
uiMenuItem *uiMenuAppendCheckItem(uiMenu *m, const char *name) { return newItem(m, typeCheckbox, name); }

uiMenuItem *uiMenuAppendQuitItem(uiMenu *m)
{
	if (hasQuit)
		uiprivUserBug("You cannot have multiple Quit menu items in the same program.");
	hasQuit = true;
	newItem(m, typeSeparator, NULL);
	return newItem(m, typeQuit, NULL);
}

uiMenuItem *uiMenuAppendPreferencesItem(uiMenu *m)
{
	if (hasPreferences)
		uiprivUserBug("You cannot have multiple Preferences menu items in the same program.");
	hasPreferences = true;
	newItem(m, typeSeparator, NULL);
	return newItem(m, typePreferences, NULL);
}

uiMenuItem *uiMenuAppendAboutItem(uiMenu *m)
{
	if (hasAbout)
		uiprivUserBug("You cannot have multiple About menu items in the same program.");
	hasAbout = true;
	newItem(m, typeSeparator, NULL);
	return newItem(m, typeAbout, NULL);
}

void uiMenuAppendSeparator(uiMenu *m) { newItem(m, typeSeparator, NULL); }

uiMenu *uiNewMenu(const char *name)
{
	if (menusFinalized)
		uiprivUserBug("You cannot create a new menu after menus have been finalized.");
	if (menus == NULL)
		menus = new BList();

	uiMenu *m = uiprivNew(uiMenu);
	menus->AddItem(m);
	m->name = uiHaikuStrdupText(name);
	m->items = new BList();
	return m;
}

BMenuBar *uiprivMakeMenubar(uiWindow *w)
{
	menusFinalized = true;

	BMenuBar *menubar = new BMenuBar("uiMenubar");
	BWindow *bw = (BWindow *) uiControlHandle(uiControl(w));
	BMessenger target(NULL, bw);

	if (menus != NULL)
		for (int32 i = 0; i < menus->CountItems(); i++) {
			uiMenu *m = (uiMenu *) menus->ItemAt(i);
			BMenu *menu = new BMenu(m->name);
			for (int32 j = 0; j < m->items->CountItems(); j++) {
				uiMenuItem *item = (uiMenuItem *) m->items->ItemAt(j);
				if (item->type == typeSeparator) {
					menu->AddSeparatorItem();
					continue;
				}
				BMessage *msg = new BMessage(uiprivMsgMenuItem);
				msg->AddPointer("item", item);
				BMenuItem *mi = new BMenuItem(item->name, msg);
				mi->SetEnabled(!item->disabled);
				if (item->type == typeCheckbox)
					mi->SetMarked(item->checked);
				menu->AddItem(mi);
				item->instances->AddItem(mi);
			}
			menu->SetTargetForItems(target);
			menubar->AddItem(menu);
		}

	return menubar;
}

void uiprivUninitMenus(void)
{
	// Frees only the global description (names, lists, structs). The BMenuItems live in their
	// windows' view trees and are freed when those windows are destroyed.
	if (menus == NULL)
		return;
	for (int32 i = 0; i < menus->CountItems(); i++) {
		uiMenu *m = (uiMenu *) menus->ItemAt(i);
		for (int32 j = 0; j < m->items->CountItems(); j++) {
			uiMenuItem *item = (uiMenuItem *) m->items->ItemAt(j);
			if (item->name != NULL) uiprivFree(item->name);
			delete item->instances;
			uiprivFree(item);
		}
		delete m->items;
		uiprivFree(m->name);
		uiprivFree(m);
	}
	delete menus;
	menus = NULL;
	menusFinalized = false;
	hasQuit = hasPreferences = hasAbout = false;
}
