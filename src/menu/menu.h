#ifndef __M_MENU_H__
#define __M_MENU_H__




#include "dobject.h"
#include "textures/textures.h"

struct event_t;
class FTexture;
class FFont;
enum EColorRange;

enum EMenuKey
{
	MKEY_Up,
	MKEY_Down,
	MKEY_Left,
	MKEY_Right,
	MKEY_PageUp,
	MKEY_PageDown,
	//----------------- Keys past here do not repeat.
	MKEY_Enter,
	MKEY_Back,		// Back to previous menu
	MKEY_Clear,		// Clear keybinding/flip player sprite preview

	NUM_MKEYS
};


//=============================================================================
//
// menu descriptor. This is created from the menu definition lump
// Items must be inserted in the order they are cycled through with the cursor
//
//=============================================================================

enum EMenuDescriptorType
{
	MDESC_ListMenu,
	MDESC_OptionsMenu,
};

struct FMenuDescriptor
{
	FName mMenuName;
	int mType;
};

class FListMenuItem;

struct FListMenuDescriptor : public FMenuDescriptor
{
	TDeletingArray<FListMenuItem *> mItems;
	int mSelectedItem;
	int mSelectOfsX;
	int mSelectOfsY;
	FTextureID mSelector;
	int mDisplayTop;
	int mXpos, mYpos;
	int mLinespacing;	// needs to be stored for dynamically created menus
	FString mNetgameMessage;
	int mAutoselect;	// this can only be set by
	FFont *mFont;
	EColorRange mFontColor;
};

typedef TMap<FName, FMenuDescriptor *> MenuDescriptorList;

extern MenuDescriptorList MenuDescriptors;

void M_ParseMenuDefs();

//=============================================================================
//
//
//
//=============================================================================

struct FMenuRect
{
	int x, y;
	int width, height;

	void set(int _x, int _y, int _w, int _h)
	{
		x = _x;
		y = _y;
		width = _w;
		height = _h;
	}

	bool inside(int _x, int _y)
	{
		return _x >= x && _x < x+width && _y >= y && _y < y+height;
	}

};


class DMenu : public DObject
{
	DECLARE_CLASS (DMenu, DObject)
	HAS_OBJECT_POINTERS

protected:

public:
	static DMenu *CurrentMenu;
	TObjPtr<DMenu> mParentMenu;

	DMenu(DMenu *parent = NULL);
	virtual ~DMenu();
	virtual bool Responder (event_t *ev);
	virtual bool MenuEvent (int mkey);
	virtual void Ticker ();
	virtual void Drawer ();
};

//=============================================================================
//
// base class for menu items
//
//=============================================================================

class FListMenuItem
{
protected:
	int mXpos, mYpos;

public:
	FListMenuItem(int xpos = 0, int ypos = 0);
	virtual ~FListMenuItem();

	virtual bool CheckCoordinate(int x, int y);
	virtual void Ticker();
	virtual void Drawer();
	virtual bool Selectable();
	virtual bool Activate();
	void DrawSelector(int xofs, int yofs, FTextureID tex);
};	

class FListMenuItemStaticPatch : public FListMenuItem
{
protected:
	FTextureID mTexture;

public:
	FListMenuItemStaticPatch(int x, int y, FTextureID patch);
	void Drawer();
};

class FListMenuItemStaticAnimation : public FListMenuItemStaticPatch
{
	TArray<FTextureID> mFrames;
	int mFrameTime;
	int mFrameCount;
	unsigned int mFrame;

public:
	FListMenuItemStaticAnimation(int x, int y, int frametime);
	void AddTexture(FTextureID tex);
	void Ticker();
};

class FListMenuItemSelectable : public FListMenuItem
{
protected:
	FName mChild;
	FMenuRect mHotspot;
	int mHotkey;
	int mParam;

public:
	FListMenuItemSelectable(int x, int y, FName childmenu, int mParam = -1);
	void SetHotspot(int x, int y, int w, int h);
	bool CheckCoordinate(int x, int y);
	bool Selectable();
	bool CheckHotkey(int c) { return c == mHotkey; }
	bool Activate();
};

class FListMenuItemText : public FListMenuItemSelectable
{
	const char *mText;
	FFont *mFont;
	EColorRange mColor;
public:
	FListMenuItemText(int x, int y, int hotkey, const char *text, FFont *font, EColorRange color, FName child, int param = 0);
	~FListMenuItemText();
	void Drawer();
};

class FListMenuItemPatch : public FListMenuItemSelectable
{
	FTextureID mTexture;
public:
	FListMenuItemPatch(int x, int y, int hotkey, FTextureID patch, FName child, int param = 0);
	void Drawer();
};


//=============================================================================
//
// list menu class runs a menu described by a FListMrnuDescriptor
//
//=============================================================================

class DListMenu : public DMenu
{
	DECLARE_CLASS(DListMenu, DMenu)

	FListMenuDescriptor *mDesc;

public:
	DListMenu(DMenu *parent = NULL, FListMenuDescriptor *desc = NULL);
	~DListMenu();
	bool Responder (event_t *ev);
	bool MenuEvent (int mkey);
	void Ticker ();
	void Drawer ();
};



#endif