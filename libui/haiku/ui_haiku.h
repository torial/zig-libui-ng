// libui-ng Haiku backend — control base + vtable macros (mirrors ui_unix.h).
// Assumes <ui.h> and the Haiku Interface Kit headers are included beforehand. Included only by
// the C++ backend translation units (uses BView*), never by the common C code.
#ifndef __LIBUI_UI_HAIKU_H__
#define __LIBUI_UI_HAIKU_H__

class BView;

#ifdef __cplusplus
extern "C" {
#endif

// Every BView-backed control embeds this. Toplevel windows (BWindow, not a BView) provide their
// own vtable instead of using the macros below.
typedef struct uiHaikuControl uiHaikuControl;
struct uiHaikuControl {
	uiControl c;
	uiControl *parent;
	int addedBefore;
	// Add (remove=0) or remove (remove=1) this control's BView from a parent BView. The parent
	// is a BGroupView/BWindow content area; AddChild routes into the layout.
	void (*SetContainer)(uiHaikuControl *, BView *parent, int remove);
};
#define uiHaikuControl(this) ((uiHaikuControl *) (this))

_UI_EXTERN uiHaikuControl *uiHaikuAllocControl(size_t n, uint32_t typesig, const char *typenamestr);
_UI_EXTERN void uiHaikuControlSetContainer(uiHaikuControl *, BView *parent, int remove);
_UI_EXTERN char *uiHaikuStrdupText(const char *);

// ---- default vtable functions for BView-backed controls (struct field: BView *view) ----
#define uiHaikuControlDefaultDestroy(type) \
	static void type ## Destroy(uiControl *c) { \
		if (type(c)->view->Parent() != NULL) type(c)->view->RemoveSelf(); \
		delete type(c)->view; \
		uiFreeControl(c); \
	}
#define uiHaikuControlDefaultHandle(type) \
	static uintptr_t type ## Handle(uiControl *c) { return (uintptr_t) (type(c)->view); }
#define uiHaikuControlDefaultParent(type) \
	static uiControl *type ## Parent(uiControl *c) { return uiHaikuControl(c)->parent; }
#define uiHaikuControlDefaultSetParent(type) \
	static void type ## SetParent(uiControl *c, uiControl *parent) { \
		uiControlVerifySetParent(c, parent); \
		uiHaikuControl(c)->parent = parent; \
	}
#define uiHaikuControlDefaultToplevel(type) \
	static int type ## Toplevel(uiControl *c) { return 0; }
#define uiHaikuControlDefaultVisible(type) \
	static int type ## Visible(uiControl *c) { return type(c)->view->IsHidden() ? 0 : 1; }
#define uiHaikuControlDefaultShow(type) \
	static void type ## Show(uiControl *c) { if (type(c)->view->IsHidden()) type(c)->view->Show(); }
#define uiHaikuControlDefaultHide(type) \
	static void type ## Hide(uiControl *c) { type(c)->view->Hide(); }
#define uiHaikuControlDefaultEnabled(type) \
	static int type ## Enabled(uiControl *c) { \
		BControl *bc = dynamic_cast<BControl *>(type(c)->view); \
		return bc != NULL ? (bc->IsEnabled() ? 1 : 0) : 1; \
	}
#define uiHaikuControlDefaultEnable(type) \
	static void type ## Enable(uiControl *c) { \
		BControl *bc = dynamic_cast<BControl *>(type(c)->view); \
		if (bc != NULL) bc->SetEnabled(true); \
	}
#define uiHaikuControlDefaultDisable(type) \
	static void type ## Disable(uiControl *c) { \
		BControl *bc = dynamic_cast<BControl *>(type(c)->view); \
		if (bc != NULL) bc->SetEnabled(false); \
	}
#define uiHaikuControlDefaultSetContainer(type) \
	static void type ## SetContainer(uiHaikuControl *c, BView *parent, int remove) { \
		if (remove) parent->RemoveChild(type(c)->view); \
		else parent->AddChild(type(c)->view); \
	}

#define uiHaikuControlAllDefaultsExceptDestroy(type) \
	uiHaikuControlDefaultHandle(type) \
	uiHaikuControlDefaultParent(type) \
	uiHaikuControlDefaultSetParent(type) \
	uiHaikuControlDefaultToplevel(type) \
	uiHaikuControlDefaultVisible(type) \
	uiHaikuControlDefaultShow(type) \
	uiHaikuControlDefaultHide(type) \
	uiHaikuControlDefaultEnabled(type) \
	uiHaikuControlDefaultEnable(type) \
	uiHaikuControlDefaultDisable(type) \
	uiHaikuControlDefaultSetContainer(type)

#define uiHaikuControlAllDefaults(type) \
	uiHaikuControlDefaultDestroy(type) \
	uiHaikuControlAllDefaultsExceptDestroy(type)

#define uiHaikuNewControl(type, var) \
	var = type(uiHaikuAllocControl(sizeof (type), type ## Signature, #type)); \
	uiControl(var)->Destroy = type ## Destroy; \
	uiControl(var)->Handle = type ## Handle; \
	uiControl(var)->Parent = type ## Parent; \
	uiControl(var)->SetParent = type ## SetParent; \
	uiControl(var)->Toplevel = type ## Toplevel; \
	uiControl(var)->Visible = type ## Visible; \
	uiControl(var)->Show = type ## Show; \
	uiControl(var)->Hide = type ## Hide; \
	uiControl(var)->Enabled = type ## Enabled; \
	uiControl(var)->Enable = type ## Enable; \
	uiControl(var)->Disable = type ## Disable; \
	uiHaikuControl(var)->SetContainer = type ## SetContainer;

#ifdef __cplusplus
}
#endif

#endif
