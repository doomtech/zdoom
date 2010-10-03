#ifndef __INTERMISSION_H
#define __INTERMISSION_H

#include "doomdef.h"
#include "dobject.h"
#include "m_fixed.h"
#include "textures/textures.h"
#include "s_sound.h"
#include "v_font.h"

struct event_t;

#define DECLARE_SUPER_CLASS(cls,parent) \
private: \
	typedef parent Super; \
	typedef cls ThisClass;

struct FIntermissionPatch
{
	FString mName;
	double x, y;
};

struct FIIntermissionPatch
{
	FTextureID mPic;
	double x, y;
};

struct FCastSound
{
	BYTE mSequence;
	BYTE mIndex;
	FString mSound;
};

struct FICastSound
{
	BYTE mSequence;
	BYTE mIndex;
	FSoundID mSound;
};

enum EFadeType
{
	FADE_In,
	FADE_Out,
};

enum EScrollDir
{
	SCROLL_Left,
	SCROLL_Right,
	SCROLL_Up,
	SCROLL_Down,
};

// actions that don't create objects
#define WIPER_ID ((const PClass*)intptr_t(-1))
#define TITLE_ID ((const PClass*)intptr_t(-2))

//==========================================================================

struct FIntermissionAction
{
	int mSize;
	const PClass *mClass;
	FString mMusic;
	int mMusicOrder;
	int mCdTrack;
	int mCdId;
	int mDuration;
	FString mBackground;
	FString mPalette;
	FString mSound;
	bool mFlatfill;
	bool mMusicLooping;
	TArray<FIntermissionPatch> mOverlays;

	FIntermissionAction();
	virtual bool ParseKey(FScanner &sc);
};

struct FIntermissionActionFader : public FIntermissionAction
{
	typedef FIntermissionAction Super;

	EFadeType mFadeType;

	FIntermissionActionFader();
	virtual bool ParseKey(FScanner &sc);
};

struct FIntermissionActionWiper : public FIntermissionAction
{
	typedef FIntermissionAction Super;

	gamestate_t mWipeType;

	FIntermissionActionWiper();
	virtual bool ParseKey(FScanner &sc);
};

struct FIntermissionActionTextscreen : public FIntermissionAction
{
	typedef FIntermissionAction Super;

	FString mText;
	int mTextDelay;
	int mTextSpeed;
	int mTextX, mTextY;
	EColorRange mTextColor;

	FIntermissionActionTextscreen();
	virtual bool ParseKey(FScanner &sc);
};

struct FIntermissionActionCast : public FIntermissionAction
{
	typedef FIntermissionAction Super;

	FString mName;
	FName mCastClass;
	TArray<FCastSound> mCastSounds;

	FIntermissionActionCast();
	virtual bool ParseKey(FScanner &sc);
};

struct FIntermissionActionScroller : public FIntermissionAction
{
	typedef FIntermissionAction Super;

	FString mSecondPic;
	int mScrollDelay;
	int mScrollTime;
	int mScrollDir;

	FIntermissionActionScroller();
	virtual bool ParseKey(FScanner &sc);
};

struct FIntermissionDescriptor
{
	FName mLink;
	TArray<FIntermissionAction *> mActions;
};

typedef TMap<FName, FIntermissionDescriptor*> FIntermissionDescriptorList;

extern FIntermissionDescriptorList IntermissionDescriptors;

//==========================================================================

class DIntermissionScreen : public DObject
{
	DECLARE_CLASS (DIntermissionScreen, DObject)

protected:
	int mDuration;
	int mTicker;
	FTextureID mBackground;
	bool mFlatfill;
	bool mPaletteChanged;
	TArray<FIIntermissionPatch> mOverlays;

public:

	DIntermissionScreen() {}
	virtual void Init(FIntermissionAction *desc, bool first);
	virtual int Responder (event_t *ev);
	virtual int Ticker ();
	virtual void Drawer ();
	void Destroy();
	FTextureID GetBackground(bool *fill)
	{
		*fill = mFlatfill;
		return mBackground;
	}
	void SetBackground(FTextureID tex, bool fill)
	{
		mBackground = tex;
		mFlatfill = fill;
	}
};

class DIntermissionScreenFader : public DIntermissionScreen
{
	DECLARE_CLASS (DIntermissionScreenFader, DIntermissionScreen)

	int mTotalTime;
	int mCounter;
	EFadeType mType;

public:

	DIntermissionScreenFader() {}
	virtual void Init(FIntermissionAction *desc, bool first);
	virtual int Responder (event_t *ev);
	virtual int Ticker ();
	virtual void Drawer ();
};

class DIntermissionScreenText : public DIntermissionScreen
{
	DECLARE_CLASS (DIntermissionScreenText, DIntermissionScreen)

	const char *mText;
	int mTextSpeed;
	int mTextX, mTextY;
	int mTextCounter;
	int mTextDelay;
	int mTextLen;
	EColorRange mTextColor;

public:

	DIntermissionScreenText() {}
	virtual void Init(FIntermissionAction *desc, bool first);
	virtual int Responder (event_t *ev);
	virtual int Ticker ();
	virtual void Drawer ();
};

class DIntermissionScreenCast : public DIntermissionScreen
{
	DECLARE_CLASS (DIntermissionScreenCast, DIntermissionScreen)

	FString mName;
	const PClass *mClass;
	TArray<FICastSound> mCastSounds;

public:

	DIntermissionScreenCast() {}
	virtual void Init(FIntermissionAction *desc, bool first);
	virtual int Responder (event_t *ev);
	virtual int Ticker ();
	virtual void Drawer ();
};

class DIntermissionScreenScroller : public DIntermissionScreen
{
	DECLARE_CLASS (DIntermissionScreenScroller, DIntermissionScreen)

	FTextureID mSecondPic;
	int mScrollDelay;
	int mScrollTime;
	int mScrollDir;

public:

	DIntermissionScreenScroller() {}
	virtual void Init(FIntermissionAction *desc, bool first);
	virtual int Responder (event_t *ev);
	virtual int Ticker ();
	virtual void Drawer ();
};


class DIntermissionController : public DObject
{
	DECLARE_CLASS (DIntermissionController, DObject)
	HAS_OBJECT_POINTERS

	FIntermissionDescriptor *mDesc;
	TObjPtr<DIntermissionScreen> mScreen;
	bool mDeleteDesc;
	bool mFirst;
	bool mAdvance;
	bool mEndingGame;
	int mIndex;

	bool NextPage();

public:
	static DIntermissionController *CurrentIntermission;

	DIntermissionController(FIntermissionDescriptor *mDesc = NULL, bool mDeleteDesc = false, bool endinggame = false);
	bool Responder (event_t *ev);
	void Ticker ();
	void Drawer ();
	void Destroy();
};


// Interface for main loop
bool F_Responder (event_t* ev);
void F_Ticker ();
void F_Drawer ();
void F_StartIntermission(FIntermissionDescriptor *desc, bool deleteme, bool endinggame);

// Create an intermission from old cluster data
void F_StartFinale (const char *music, int musicorder, int cdtrack, unsigned int cdid, const char *flat, 
					const char *text, INTBOOL textInLump, INTBOOL finalePic, INTBOOL lookupText, 
					bool ending, FName endsequence = NAME_None);



#endif
