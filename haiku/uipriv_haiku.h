// libui-ng Haiku backend — private umbrella header included by every haiku/*.cpp.
// Mirrors uipriv_unix.h: pulls the public ui.h, then the OS ui_haiku.h, then common uipriv.h
// (order matters — uipriv.h must see ui.h + ui_haiku.h first, per its own comment).
#ifndef __LIBUI_UIPRIV_HAIKU_H__
#define __LIBUI_UIPRIV_HAIKU_H__

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Control.h>
#include <Button.h>
#include <CheckBox.h>
#include <RadioButton.h>
#include <TextControl.h>
#include <TextView.h>
#include <StringView.h>
#include <Slider.h>
#include <StatusBar.h>
#include <SeparatorView.h>
#include <Box.h>
#include <ColorControl.h>
#include <ListView.h>
#include <ListItem.h>
#include <Font.h>
#include <TabView.h>
#include <OptionPopUp.h>
#include <ScrollView.h>
#include <GridView.h>
#include <GridLayout.h>
#include <LayoutItem.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <SeparatorItem.h>
#include <Shape.h>
#include <AffineTransform.h>
#include <Region.h>
#include <Gradient.h>
#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GroupView.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <Messenger.h>
#include <String.h>
#include <Rect.h>

#include "../ui.h"
#include "ui_haiku.h"
#include "../common/uipriv.h"

// backend-internal (defined in alloc.cpp / main.cpp); C++ linkage, only the backend uses them.
void uiprivInitAlloc(void);
void uiprivUninitAlloc(void);

// Generic control-event plumbing. A control wires its BControl invocation/modification message to a
// 'uiEV' BMessage carrying (fn, control); the owning window's looper calls fn(control), which reads
// the control's stored user callback and fires it. This keeps window.cpp independent of every
// control type — adding a control needs no change there.
enum {
	uiprivMsgControlEvent = 'uiEV',
	uiprivMsgMenuItem = 'uiMN',
};
typedef void (*uiprivEventFn)(void *control);
BMessage *uiprivNewEventMessage(uiprivEventFn fn, void *control);

// ---- table (tablemodel.cpp / table.cpp) ----
// The model wraps the user's handler and tracks the tables built from it, so row-change
// notifications can be pushed into each table's native BColumnListView.
struct uiTableModel {
	uiTableModelHandler *mh;
	BList *tables;		// uiTable*
};
void uiprivTableRowInserted(uiTable *t, int index);
void uiprivTableRowChanged(uiTable *t, int index);
void uiprivTableRowDeleted(uiTable *t, int index);

// image.cpp — returns a BBitmap representation of a uiImage (NULL if none); class fwd-declared so
// headers needn't pull <Bitmap.h>.
class BBitmap;
BBitmap *uiprivImageBestBitmap(uiImage *i);

// ---- drawing (area.cpp / draw.cpp / drawpath.cpp / drawmatrix.cpp) ----
// A draw context is just the BView being drawn into; all uiDraw* calls happen inside its Draw().
struct uiDrawContext {
	BView *view;
};
// A path is a BShape plus the libui fill mode and a little figure-tracking state.
struct uiDrawPath {
	BShape *shape;
	unsigned int fillMode;
	bool ended;
	bool inFigure;
	double startX, startY;	// current figure's start point (for arcs/close)
	double curX, curY;	// current point
};

// menu.cpp — the menu model is global (built before the first window, then materialized per window).
// uiprivMakeMenubar builds a window's native BMenuBar; the window forwards 'uiMN' clicks (carrying an
// "item" pointer) to uiprivMenuItemClicked; uiprivUninitMenus tears the global model down at exit.
BMenuBar *uiprivMakeMenubar(uiWindow *w);
void uiprivMenuItemClicked(uiMenuItem *item, uiWindow *w);
void uiprivUninitMenus(void);

#endif
