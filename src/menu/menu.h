#ifndef __M_MENU_MENU_H__
#define __M_MENU_MENU_H__




#include "dobject.h"
#include "lists.h"
#include "d_player.h"
#include "r_translate.h"
#include "c_cvars.h"
#include "version.h"
#include "textures/textures.h"

EXTERN_CVAR(Float, snd_menuvolume)

struct event_t;
class FTexture;
class FFont;
enum EColorRange;
class FPlayerClass;
class FKeyBindings;

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
	NUM_MKEYS,

	// These are not buttons but events sent from other menus 

	MKEY_Input,		// Sent when input is confirmed
	MKEY_Abort,		// Input aborted
	MKEY_MBYes,
	MKEY_MBNo,
};


struct FGameStartup
{
	const char *PlayerClass;
	int Episode;
	int Skill;
};

extern FGameStartup GameStartupInfo;

struct FSaveGameNode : public Node
{
	char Title[SAVESTRINGSIZE];
	FString Filename;
	bool bOldVersion;
	bool bMissingWads;
	bool bNoDelete;

	FSaveGameNode() { bNoDelete = false; }
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
class FOptionMenuItem;

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
	int mAutoselect;	// this can only be set by internal menu creation functions
	FFont *mFont;
	EColorRange mFontColor;
	EColorRange mFontColor2;
	const PClass *mClass;
	FMenuDescriptor *mRedirect;	// used to redirect overlong skill and episode menus to option menu based alternatives
};

struct FOptionMenuSettings
{
	EColorRange mTitleColor;

	EColorRange mFontColor;
	EColorRange mFontColorValue;
	EColorRange mFontColorMore;
	EColorRange mFontColorHeader;
	int mLinespacing;
	int mLabelOffset;
};

struct FOptionMenuDescriptor : public FMenuDescriptor
{
	TDeletingArray<FOptionMenuItem *> mItems;
	FString mTitle;
	int mSelectedItem;
	int mScrollTop;
	int mScrollPos;
	int mIndent;
	int mPosition;
	bool mDontDim;
	const PClass *mClass;
};
						

typedef TMap<FName, FMenuDescriptor *> MenuDescriptorList;

extern FOptionMenuSettings OptionSettings;
extern MenuDescriptorList MenuDescriptors;


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
	static int MenuTime;

	TObjPtr<DMenu> mParentMenu;

	DMenu(DMenu *parent = NULL);
	virtual bool Responder (event_t *ev);
	virtual bool MenuEvent (int mkey, bool fromcontroller);
	virtual void Ticker ();
	virtual void Drawer ();
	virtual bool DimAllowed ();
	virtual bool TranslateKeyboardEvents();
	virtual void Close();
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
	FName mAction;

public:
	bool mEnabled;

	FListMenuItem(int xpos = 0, int ypos = 0, FName action = NAME_None);
	virtual ~FListMenuItem();

	virtual bool CheckCoordinate(int x, int y);
	virtual void Ticker();
	virtual void Drawer();
	virtual bool Selectable();
	virtual bool Activate();
	virtual FName GetAction(int *pparam);
	virtual bool SetString(int i, const char *s);
	virtual bool GetString(int i, char *s, int len);
	virtual bool SetValue(int i, int value);
	virtual bool GetValue(int i, int *pvalue);
	virtual void Enable(bool on);
	virtual bool MenuEvent (int mkey, bool fromcontroller);
	virtual bool CheckHotkey(int c);
	void DrawSelector(int xofs, int yofs, FTextureID tex);
};	

class FListMenuItemStaticPatch : public FListMenuItem
{
protected:
	FTextureID mTexture;
	bool mCentered;

public:
	FListMenuItemStaticPatch(int x, int y, FTextureID patch, bool centered);
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

class FListMenuItemStaticText : public FListMenuItem
{
protected:
	const char *mText;
	FFont *mFont;
	EColorRange mColor;
	bool mCentered;

public:
	FListMenuItemStaticText(int x, int y, const char *text, FFont *font, EColorRange color, bool centered);
	void Drawer();
};

//=============================================================================
//
// the player sprite window
//
//=============================================================================

class FListMenuItemPlayerDisplay : public FListMenuItem
{
	FListMenuDescriptor *mOwner;
	FTexture *mBackdrop;
	FRemapTable mRemap;
	FPlayerClass *mPlayerClass;
	FState *mPlayerState;
	int mPlayerTics;
	bool mNoportrait;
	BYTE mRotation;
	BYTE mMode;	// 0: automatic (used by class selection), 1: manual (used by player setup)
	BYTE mTranslate;
	int mSkin;

	void SetPlayerClass(int classnum);
	bool UpdatePlayerClass();

public:

	enum
	{
		PDF_ROTATION = 0x10001,
		PDF_SKIN = 0x10002,
		PDF_CLASS = 0x10003,
		PDF_MODE = 0x10004,
		PDF_TRANSLATE = 0x10005,
	};

	FListMenuItemPlayerDisplay(FListMenuDescriptor *menu, int x, int y, PalEntry c1, PalEntry c2, bool np, FName action);
	~FListMenuItemPlayerDisplay();
	virtual void Ticker();
	virtual void Drawer();
	bool SetValue(int i, int value);
};


//=============================================================================
//
// selectable items
//
//=============================================================================

class FListMenuItemSelectable : public FListMenuItem
{
protected:
	FMenuRect mHotspot;
	int mHotkey;
	int mParam;

public:
	FListMenuItemSelectable(int x, int y, FName childmenu, int mParam = -1);
	void SetHotspot(int x, int y, int w, int h);
	bool CheckCoordinate(int x, int y);
	bool Selectable();
	bool CheckHotkey(int c);
	bool Activate();
	FName GetAction(int *pparam);
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
// items for the player menu
//
//=============================================================================

class FPlayerNameBox : public FListMenuItemSelectable
{
	const char *mText;
	FFont *mFont;
	EColorRange mFontColor;
	int mFrameSize;
	char mPlayerName[MAXPLAYERNAME+1];
	char mEditName[MAXPLAYERNAME+2];
	bool mEntering;

	void DrawBorder (int x, int y, int len);

public:

	FPlayerNameBox(int x, int y, int frameofs, const char *text, FFont *font, EColorRange color, FName action);
	~FPlayerNameBox();
	bool SetString(int i, const char *s);
	bool GetString(int i, char *s, int len);
	void Drawer();
	bool MenuEvent (int mkey, bool fromcontroller);
};

//=============================================================================
//
// items for the player menu
//
//=============================================================================

class FValueTextItem : public FListMenuItemSelectable
{
	TArray<FString> mSelections;
	const char *mText;
	int mSelection;
	FFont *mFont;
	EColorRange mFontColor;
	EColorRange mFontColor2;

public:

	FValueTextItem(int x, int y, const char *text, FFont *font, EColorRange color, EColorRange valuecolor, FName action);
	~FValueTextItem();
	bool SetString(int i, const char *s);
	bool SetValue(int i, int value);
	bool GetValue(int i, int *pvalue);
	bool MenuEvent (int mkey, bool fromcontroller);
	void Drawer();
};

//=============================================================================
//
// items for the player menu
//
//=============================================================================

class FSliderItem : public FListMenuItemSelectable
{
	const char *mText;
	FFont *mFont;
	EColorRange mFontColor;
	int mMinrange, mMaxrange;
	int mStep;
	int mSelection;

	void DrawSlider (int x, int y);

public:

	FSliderItem(int x, int y, const char *text, FFont *font, EColorRange color, FName action, int min, int max, int step);
	~FSliderItem();
	bool SetValue(int i, int value);
	bool GetValue(int i, int *pvalue);
	bool MenuEvent (int mkey, bool fromcontroller);
	void Drawer();
};

//=============================================================================
//
// list menu class runs a menu described by a FListMenuDescriptor
//
//=============================================================================

class DListMenu : public DMenu
{
	DECLARE_CLASS(DListMenu, DMenu)

protected:
	FListMenuDescriptor *mDesc;

public:
	DListMenu(DMenu *parent = NULL, FListMenuDescriptor *desc = NULL);
	virtual void Init(DMenu *parent = NULL, FListMenuDescriptor *desc = NULL);
	FListMenuItem *GetItem(FName name);
	bool Responder (event_t *ev);
	bool MenuEvent (int mkey, bool fromcontroller);
	void Ticker ();
	void Drawer ();
};


//=============================================================================
//
// base class for menu items
//
//=============================================================================

class FOptionMenuItem : public FListMenuItem
{
protected:
	char *mLabel;
	bool mCentered;

	void drawLabel(int indent, int y, EColorRange color, bool grayed = false);
public:

	FOptionMenuItem(const char *text, FName action = NAME_None, bool center = false);
	~FOptionMenuItem();
	virtual bool CheckCoordinate(FOptionMenuDescriptor *desc, int x, int y);
	virtual void Draw(FOptionMenuDescriptor *desc, int y, int indent);
	void DrawSelector(int xofs, int yofs, FTextureID tex);
	virtual bool Selectable();
	virtual int GetIndent();
};	

//=============================================================================
//
//
//
//=============================================================================
struct FOptionValues
{
	struct Pair
	{
		double Value;
		FString Text;
	};

	TArray<Pair> mValues;
};

typedef TMap< FName, FOptionValues* > FOptionMap;

extern FOptionMap OptionValues;


class FOptionMenuItemSubmenu : public FOptionMenuItem
{
	// action is a submenu name
public:
	FOptionMenuItemSubmenu(const char *label, const char *menu);
	virtual void Draw(FOptionMenuDescriptor *desc, int y, int indent);
	virtual bool Activate();
};

class FOptionMenuItemCommand : public FOptionMenuItemSubmenu
{
	// action is a CCMD
public:
	FOptionMenuItemCommand(const char *label, const char *menu);
	virtual bool Activate();
};

class FOptionMenuItemSafeCommand : public FOptionMenuItemCommand
{
	// action is a CCMD
public:
	FOptionMenuItemSafeCommand(const char *label, const char *menu);
	bool MenuEvent (int mkey, bool fromcontroller);
	virtual bool Activate();
};

class FOptionMenuItemOption : public FOptionMenuItem
{
	// action is a CVAR
	FOptionValues *mValues;
	FBaseCVar *mCVar;
	FBoolCVar *mGrayCheck;
	int mSelection;
public:
	FOptionMenuItemOption(const char *label, const char *menu, const char *values, const char *graycheck);
	virtual void Draw(FOptionMenuDescriptor *desc, int y, int indent);
	bool MenuEvent (int mkey, bool fromcontroller);
	virtual bool Activate();
	virtual bool Selectable();
};

class FOptionMenuItemControl : public FOptionMenuItem
{
	// action is a CCMD
	FKeyBindings *mBindings;
public:
	FOptionMenuItemControl(const char *label, const char *menu, FKeyBindings *bindings);
	virtual void Draw(FOptionMenuDescriptor *desc, int y, int indent);
	virtual bool Activate();
};

class FOptionMenuItemStaticText : public FOptionMenuItem
{
	EColorRange mColor;
public:
	FOptionMenuItemStaticText(const char *label, bool header);
	virtual void Draw(FOptionMenuDescriptor *desc, int y, int indent);
	virtual bool Activate();
	virtual bool Selectable();
};

class FOptionMenuSliderItem : public FOptionMenuItem
{
	// action is a CVAR
	float mMin, mMax, mStep;
	float mValue;
	bool mShowValue;
	FBaseCVar *mCVar;
	float *mPVal;
public:
	FOptionMenuSliderItem(const char *label, const char *menu, double min, double max, double step, bool showval);
	FOptionMenuSliderItem(const char *label, float *pVal, double min, double max, double step, bool showval);
	virtual void Draw(FOptionMenuDescriptor *desc, int y, int indent);
	bool MenuEvent (int mkey, bool fromcontroller);
	virtual bool Activate();
};

//=============================================================================
//
// Option menu class runs a menu described by a FOptionMenuDescriptor
//
//=============================================================================

class DOptionMenu : public DMenu
{
	DECLARE_CLASS(DOptionMenu, DMenu)

	bool CanScrollUp;
	bool CanScrollDown;
	int VisBottom;

protected:
	FOptionMenuDescriptor *mDesc;

public:
	DOptionMenu(DMenu *parent = NULL, FOptionMenuDescriptor *desc = NULL);
	virtual void Init(DMenu *parent = NULL, FOptionMenuDescriptor *desc = NULL);
	//FListMenuItem *GetItem(FName name);
	bool Responder (event_t *ev);
	bool MenuEvent (int mkey, bool fromcontroller);
	void Ticker ();
	void Drawer ();
};


//=============================================================================
//
// Input some text
//
//=============================================================================

class DTextEnterMenu : public DMenu
{
	DECLARE_ABSTRACT_CLASS(DTextEnterMenu, DMenu)

	char *mEnterString;
	unsigned int mEnterSize;
	unsigned int mEnterPos;
	int mSizeMode; // 1: size is length in chars. 2: also check string width
	bool mInputGridOkay;

	int InputGridX;
	int InputGridY;

public:

	DTextEnterMenu(DMenu *parent, char *textbuffer, int maxlen, int sizemode, bool showgrid);

	void Drawer ();
	bool MenuEvent (int mkey, bool fromcontroller);
	bool Responder(event_t *ev);
	bool TranslateKeyboardEvents();

};




struct event_t;
bool M_Responder (event_t *ev);
void M_Ticker (void);
void M_Drawer (void);
void M_Init (void);
void M_ActivateMenu(DMenu *menu);
void M_ClearMenus ();
void M_ParseMenuDefs();
void M_StartupSkillMenu(FGameStartup *gs);
void M_StartControlPanel (bool makeSound);
void M_SetMenu(FName menu, int param = -1);
void M_NotifyNewSave (const char *file, const char *title, bool okForQuicksave);
void M_StartMessage(const char *message, int messagemode, FName action = NAME_None);


#endif