/*
** listmenu.cpp
** A simple menu consisting of a list of items
**
**---------------------------------------------------------------------------
** Copyright 2010 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "v_video.h"
#include "v_font.h"
#include "cmdlib.h"
#include "gstrings.h"
#include "g_level.h"
#include "gi.h"
#include "menu/menu.h"

IMPLEMENT_CLASS(DListMenu)
void M_SetMenu(FName menu);



//=============================================================================
//
//
//
//=============================================================================

DListMenu::DListMenu(DMenu *parent, FListMenuDescriptor *desc)
: DMenu(parent)
{
	mDesc = desc;
}

//=============================================================================
//
//
//
//=============================================================================

DListMenu::~DListMenu()
{
}

//=============================================================================
//
//
//
//=============================================================================

bool DListMenu::Responder (event_t *ev)
{
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

bool DListMenu::MenuEvent (int mkey)
{
	int startedAt = mDesc->mSelectedItem;

	switch (mkey)
	{
	case MKEY_Up:
		do
		{
			if (--mDesc->mSelectedItem < 0) mDesc->mSelectedItem = mDesc->mItems.Size()-1;
		}
		while (!mDesc->mItems[mDesc->mSelectedItem]->Selectable() && mDesc->mSelectedItem != startedAt);
		return true;

	case MKEY_Down:
		do
		{
			if (++mDesc->mSelectedItem >= (int)mDesc->mItems.Size()) mDesc->mSelectedItem = 0;
		}
		while (!mDesc->mItems[mDesc->mSelectedItem]->Selectable() && mDesc->mSelectedItem != startedAt);
		return true;

	case MKEY_Enter:
		return mDesc->mItems[mDesc->mSelectedItem]->Activate();

	default:
		return Super::MenuEvent(mkey);
	}
}

//=============================================================================
//
//
//
//=============================================================================

void DListMenu::Ticker ()
{
	for(unsigned i=0;i<mDesc->mItems.Size(); i++)
	{
		mDesc->mItems[i]->Ticker();
	}
}

//=============================================================================
//
//
//
//=============================================================================

void DListMenu::Drawer ()
{
	for(unsigned i=0;i<mDesc->mItems.Size(); i++)
	{
		mDesc->mItems[i]->Drawer();
	}
	if (mDesc->mSelectedItem >= 0 && mDesc->mSelectedItem < (int)mDesc->mItems.Size())
		mDesc->mItems[mDesc->mSelectedItem]->DrawSelector(mDesc->mSelectOfsX, mDesc->mSelectOfsY, mDesc->mSelector);
}

//=============================================================================
//
// base class for menu items
//
//=============================================================================

FListMenuItem::FListMenuItem(int x, int y) 
{
	mXpos = x;
	mYpos = y;
}

FListMenuItem::~FListMenuItem()
{
}

bool FListMenuItem::CheckCoordinate(int x, int y)
{
	return false;
}

void FListMenuItem::Ticker()
{
}

void FListMenuItem::Drawer()
{
}

bool FListMenuItem::Selectable()
{
	return false;
}

void FListMenuItem::DrawSelector(int xofs, int yofs, FTextureID tex)
{
	screen->DrawTexture (TexMan(tex), mXpos + xofs, mYpos + yofs, DTA_Clean, true, TAG_DONE);
}

bool FListMenuItem::Activate()
{
	return false;	// cannot be activated
}

FName FListMenuItem::GetAction(int *pparam)
{
	return NAME_None;
}


//=============================================================================
//
// static patch
//
//=============================================================================

FListMenuItemStaticPatch::FListMenuItemStaticPatch(int x, int y, FTextureID patch)
: FListMenuItem(x, y)
{
	mTexture = patch;
}
	
void FListMenuItemStaticPatch::Drawer()
{
	screen->DrawTexture (TexMan(mTexture), mXpos, mYpos, DTA_Clean, true, TAG_DONE);
}

//=============================================================================
//
// static animation
//
//=============================================================================

FListMenuItemStaticAnimation::FListMenuItemStaticAnimation(int x, int y, int frametime)
: FListMenuItemStaticPatch(x, y, FNullTextureID())
{
	mFrameTime = frametime;
	mFrameCount = 0;
}

void FListMenuItemStaticAnimation::AddTexture(FTextureID tex)
{
	if (!mTexture.isValid()) mTexture = tex;
	if (tex.isValid()) mFrames.Push(tex);
}
	
void FListMenuItemStaticAnimation::Ticker()
{
	if (++mFrameCount > mFrameTime)
	{
		mFrameCount = 0;
		if (++mFrame >= mFrames.Size())
		{
			mFrame = 0;
		}
		mTexture = mFrames[mFrame];
	}
}

//=============================================================================
//
// static text
//
//=============================================================================

FListMenuItemStaticText::FListMenuItemStaticText(int x, int y, const char *text, FFont *font, EColorRange color, bool centered)
: FListMenuItem(x, y)
{
	mText = ncopystring(text);
	mFont = font;
	mColor = color;
	mCentered = centered;
}
	
void FListMenuItemStaticText::Drawer()
{
	const char *text = mText;
	if (text != NULL)
	{
		if (*text == '$') text = GStrings(text+1);
		int x = mXpos;
		if (mCentered) x -= mFont->StringWidth(text)/2;
		screen->DrawText(mFont, mColor, x, mYpos, text, DTA_Clean, true, TAG_DONE);
	}
}

//=============================================================================
//
// Hexen's player class display
//
//=============================================================================

FListMenuItemHexenPlayer::FListMenuItemHexenPlayer(FListMenuDescriptor *menu, int x, int y)
: FListMenuItem(x, y)
{
	mOwner = menu;	// we need this to get the currently selected player
}

void FListMenuItemHexenPlayer::AddFrame(const char *tex)
{
	FTextureID t = TexMan.CheckForTexture(tex, FTexture::TEX_MiscPatch);
	mTextures.Push(t);
}

void FListMenuItemHexenPlayer::AddAnimation(const char *tex)
{
	FString tname;
	for(int i=1;i<=4;i++)
	{
		tname.Format(tex, i);
		AddFrame(tname);
	}
}

void FListMenuItemHexenPlayer::Drawer()
{
	int sel = mOwner->mSelectedItem;
	int classnum;
	FName seltype = mOwner->mItems[sel]->GetAction(&classnum);

	if (seltype != NAME_Episodemenu) return;
	if (classnum < 0 || classnum >= (int)mTextures.Size()/4)
	{
		classnum = (DMenu::MenuTime>>2) % 3;
	}
	screen->DrawTexture (TexMan[mTextures[classnum*5]], mXpos, mYpos, DTA_Clean, true, TAG_DONE);
	screen->DrawTexture (TexMan[mTextures[classnum*5 + ((DMenu::MenuTime >> 3) & 3) + 1]], mXpos + 24, mYpos + 12, DTA_Clean, true, TAG_DONE);
}

//=============================================================================
//
// base class for selectable items
//
//=============================================================================

FListMenuItemSelectable::FListMenuItemSelectable(int x, int y, FName childmenu, int param)
: FListMenuItem(x, y), mAction(childmenu)
{
	mParam = param;
}

void FListMenuItemSelectable::SetHotspot(int x, int y, int w, int h)
{
	mHotspot.set(x, y, w, h);
}

bool FListMenuItemSelectable::CheckCoordinate(int x, int y)
{
	return mHotspot.inside(x, y);
}

bool FListMenuItemSelectable::Selectable()
{
	return true;
}

bool FListMenuItemSelectable::Activate()
{
	if (mParam >= 0)
	{
		// Todo: handle parameters for start game submenus
		//M_SetParam(mAction, mParam);
	}
	M_SetMenu(mAction);
	return true;
}

FName FListMenuItemSelectable::GetAction(int *pparam)
{
	if (pparam != NULL) *pparam = mParam;
	return mAction;
}



//=============================================================================
//
// text item
//
//=============================================================================

FListMenuItemText::FListMenuItemText(int x, int y, int hotkey, const char *text, FFont *font, EColorRange color, FName child, int param)
: FListMenuItemSelectable(x, y, child, param)
{
	mText = ncopystring(text);
	mFont = font;
	mColor = color;
	mHotkey = hotkey;
}

FListMenuItemText::~FListMenuItemText()
{
	if (mText != NULL)
	{
		delete [] mText;
	}
}

void FListMenuItemText::Drawer()
{
	const char *text = mText;
	if (text != NULL)
	{
		if (*text == '$') text = GStrings(text+1);
		screen->DrawText(mFont, mColor, mXpos, mYpos, text, DTA_Clean, true, TAG_DONE);
	}
}

//=============================================================================
//
// patch item
//
//=============================================================================

FListMenuItemPatch::FListMenuItemPatch(int x, int y, int hotkey, FTextureID patch, FName child, int param)
: FListMenuItemSelectable(x, y, child, param)
{
	mTexture = patch;
}

void FListMenuItemPatch::Drawer()
{
	screen->DrawTexture (TexMan(mTexture), mXpos, mYpos, DTA_Clean, true, TAG_DONE);
}
