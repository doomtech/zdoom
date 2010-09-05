/*
** menudef.cpp
** MENUDEF parser amd menu generation code
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
#include <float.h>

#include "menu/menu.h"
#include "c_dispatch.h"
#include "w_wad.h"
#include "sc_man.h"
#include "v_font.h"
#include "g_level.h"
#include "d_player.h"
#include "v_video.h"
#include "gi.h"
#include "i_system.h"
#include "c_bind.h"
#include "v_palette.h"
#include "d_event.h"
#include "d_gui.h"
#include "i_music.h"

#include "optionmenuitems.h"

static void InitCrosshairsList();

MenuDescriptorList MenuDescriptors;
static FListMenuDescriptor DefaultListMenuSettings;	// contains common settings for all list menus
static FOptionMenuDescriptor DefaultOptionMenuSettings;	// contains common settings for all Option menus
FOptionMenuSettings OptionSettings;
FOptionMap OptionValues;

static void DeinitMenus()
{
	{
		MenuDescriptorList::Iterator it(MenuDescriptors);

		MenuDescriptorList::Pair *pair;

		while (it.NextPair(pair))
		{
			delete pair->Value;
			pair->Value = NULL;
		}
	}

	{
		FOptionMap::Iterator it(OptionValues);

		FOptionMap::Pair *pair;

		while (it.NextPair(pair))
		{
			delete pair->Value;
			pair->Value = NULL;
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================

static void SkipSubBlock(FScanner &sc)
{
	sc.MustGetStringName("{");
	int depth = 1;
	while (depth > 0)
	{
		sc.MustGetString();
		if (sc.Compare("{")) depth++;
		if (sc.Compare("}")) depth--;
	}
}

//=============================================================================
//
//
//
//=============================================================================

static bool CheckSkipGameBlock(FScanner &sc)
{
	int filter = 0;
	sc.MustGetStringName("(");
	do
	{
		sc.MustGetString();
		if (sc.Compare("Doom")) filter |= GAME_Doom;
		if (sc.Compare("Heretic")) filter |= GAME_Heretic;
		if (sc.Compare("Hexen")) filter |= GAME_Hexen;
		if (sc.Compare("Strife")) filter |= GAME_Strife;
		if (sc.Compare("Chex")) filter |= GAME_Chex;
	}
	while (sc.CheckString(","));
	sc.MustGetStringName(")");
	if (!(gameinfo.gametype & filter))
	{
		SkipSubBlock(sc);
		return true;
	}
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

static bool CheckSkipOptionBlock(FScanner &sc)
{
	bool filter = false;
	sc.MustGetStringName("(");
	do
	{
		sc.MustGetString();
		if (sc.Compare("ReadThis")) filter |= gameinfo.drawreadthis;
		else if (sc.Compare("Windows"))
		{
			#ifdef _WIN32
				filter = true;
			#endif
		}
		else if (sc.Compare("unix"))
		{
			#ifdef unix
				filter = true;
			#endif
		}
		else if (sc.Compare("Mac"))
		{
			#ifdef __APPLE__
				filter = true;
			#endif
		}
	}
	while (sc.CheckString(","));
	sc.MustGetStringName(")");
	if (!filter)
	{
		SkipSubBlock(sc);
		return true;
	}
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

static void ParseListMenuBody(FScanner &sc, FListMenuDescriptor *desc)
{
	sc.MustGetStringName("{");
	while (!sc.CheckString("}"))
	{
		sc.MustGetString();
		if (sc.Compare("ifgame"))
		{
			if (!CheckSkipGameBlock(sc))
			{
				// recursively parse sub-block
				ParseListMenuBody(sc, desc);
			}
		}
		else if (sc.Compare("ifoption"))
		{
			if (!CheckSkipOptionBlock(sc))
			{
				// recursively parse sub-block
				ParseListMenuBody(sc, desc);
			}
		}
		else if (sc.Compare("Class"))
		{
			sc.MustGetString();
			const PClass *cls = PClass::FindClass(sc.String);
			if (cls == NULL || !cls->IsDescendantOf(RUNTIME_CLASS(DListMenu)))
			{
				sc.ScriptError("Unknown menu class '%s'", sc.String);
			}
			desc->mClass = cls;
		}
		else if (sc.Compare("Selector"))
		{
			sc.MustGetString();
			desc->mSelector = TexMan.CheckForTexture(sc.String, FTexture::TEX_MiscPatch);
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			desc->mSelectOfsX = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			desc->mSelectOfsY = sc.Number;
		}
		else if (sc.Compare("Linespacing"))
		{
			sc.MustGetNumber();
			desc->mLinespacing = sc.Number;
		}
		else if (sc.Compare("Position"))
		{
			sc.MustGetNumber();
			desc->mXpos = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			desc->mYpos = sc.Number;
		}
		else if (sc.Compare("StaticPatch") || sc.Compare("StaticPatchCentered"))
		{
			bool centered = sc.Compare("StaticPatchCentered");
			sc.MustGetNumber();
			int x = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			int y = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FTextureID tex = TexMan.CheckForTexture(sc.String, FTexture::TEX_MiscPatch);

			FListMenuItem *it = new FListMenuItemStaticPatch(x, y, tex, centered);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("StaticAnimation"))
		{
			sc.MustGetNumber();
			int x = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			int y = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FString templatestr = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			int delay = sc.Number;
			FListMenuItemStaticAnimation *it = new FListMenuItemStaticAnimation(x, y, delay);
			desc->mItems.Push(it);
			sc.MustGetStringName(",");
			do
			{
				sc.MustGetNumber();
				int a = sc.Number;
				sc.MustGetStringName(",");
				sc.MustGetNumber();
				int b = sc.Number;
				int step = b >= a? 1:-1;
				b+=step;
				for(int c = a; c != b; c+=step)
				{
					FString texname;
					texname.Format(templatestr.GetChars(), c);
					FTextureID texid = TexMan.CheckForTexture(texname, FTexture::TEX_MiscPatch);
					it->AddTexture(texid);
				}
			}
			while (sc.CheckString(","));
		}
		else if (sc.Compare("StaticText") || sc.Compare("StaticTextCentered"))
		{
			bool centered = sc.Compare("StaticTextCentered");
			sc.MustGetNumber();
			int x = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			int y = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FListMenuItem *it = new FListMenuItemStaticText(x, y, sc.String, desc->mFont, desc->mFontColor, centered);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("PatchItem"))
		{
			sc.MustGetString();
			FTextureID tex = TexMan.CheckForTexture(sc.String, FTexture::TEX_MiscPatch);
			sc.MustGetStringName(",");
			sc.MustGetString();
			int hotkey = sc.String[0];
			sc.MustGetStringName(",");
			sc.MustGetString();
			FName action = sc.String;
			int param = 0;
			if (sc.CheckString(","))
			{
				sc.MustGetNumber();
				param = sc.Number;
			}

			FListMenuItem *it = new FListMenuItemPatch(desc->mXpos, desc->mYpos, hotkey, tex, action, param);
			desc->mItems.Push(it);
			desc->mYpos += desc->mLinespacing;
			if (desc->mSelectedItem == -1) desc->mSelectedItem = desc->mItems.Size()-1;
		}
		else if (sc.Compare("TextItem"))
		{
			sc.MustGetString();
			FString text = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			int hotkey = sc.String[0];
			sc.MustGetStringName(",");
			sc.MustGetString();
			FName action = sc.String;
			int param = 0;
			if (sc.CheckString(","))
			{
				sc.MustGetNumber();
				param = sc.Number;
			}

			FListMenuItem *it = new FListMenuItemText(desc->mXpos, desc->mYpos, hotkey, text, desc->mFont, desc->mFontColor, action, param);
			desc->mItems.Push(it);
			desc->mYpos += desc->mLinespacing;
			if (desc->mSelectedItem == -1) desc->mSelectedItem = desc->mItems.Size()-1;

		}
		else if (sc.Compare("Font"))
		{
			sc.MustGetString();
			FFont *newfont = V_GetFont(sc.String);
			if (newfont != NULL) desc->mFont = newfont;
			if (sc.CheckString(","))
			{
				sc.MustGetString();
				desc->mFontColor2 = desc->mFontColor = V_FindFontColor((FName)sc.String);
				if (sc.CheckString(","))
				{
					sc.MustGetString();
					desc->mFontColor2 = V_FindFontColor((FName)sc.String);
				}
			}
			else
			{
				desc->mFontColor = CR_UNTRANSLATED;
				desc->mFontColor2 = CR_UNTRANSLATED;
			}
		}
		else if (sc.Compare("NetgameMessage"))
		{
			sc.MustGetString();
			desc->mNetgameMessage = sc.String;
		}
		else if (sc.Compare("PlayerDisplay"))
		{
			bool noportrait = false;
			FName action = NAME_None;
			sc.MustGetNumber();
			int x = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			int y = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetString();
			PalEntry c1 = V_GetColor(NULL, sc.String);
			sc.MustGetStringName(",");
			sc.MustGetString();
			PalEntry c2 = V_GetColor(NULL, sc.String);
			if (sc.CheckString(","))
			{
				sc.MustGetNumber();
				noportrait = !!sc.Number;
				if (sc.CheckString(","))
				{
					sc.MustGetString();
					action = sc.String;
				}
			}
			FListMenuItemPlayerDisplay *it = new FListMenuItemPlayerDisplay(desc, x, y, c1, c2, noportrait, action);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("PlayerNameBox"))
		{
			sc.MustGetString();
			FString text = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			int ofs = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FListMenuItem *it = new FPlayerNameBox(desc->mXpos, desc->mYpos, ofs, text, desc->mFont, desc->mFontColor, sc.String);
			desc->mItems.Push(it);
			desc->mYpos += desc->mLinespacing;
			if (desc->mSelectedItem == -1) desc->mSelectedItem = desc->mItems.Size()-1;
		}
		else if (sc.Compare("ValueText"))
		{
			sc.MustGetString();
			FString text = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FListMenuItem *it = new FValueTextItem(desc->mXpos, desc->mYpos, text, desc->mFont, desc->mFontColor, desc->mFontColor2, sc.String);
			desc->mItems.Push(it);
			desc->mYpos += desc->mLinespacing;
			if (desc->mSelectedItem == -1) desc->mSelectedItem = desc->mItems.Size()-1;
		}
		else if (sc.Compare("Slider"))
		{
			sc.MustGetString();
			FString text = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FString action = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			int min = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			int max = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			int step = sc.Number;
			FListMenuItem *it = new FSliderItem(desc->mXpos, desc->mYpos, text, desc->mFont, desc->mFontColor, action, min, max, step);
			desc->mItems.Push(it);
			desc->mYpos += desc->mLinespacing;
			if (desc->mSelectedItem == -1) desc->mSelectedItem = desc->mItems.Size()-1;
		}
		else
		{
			sc.ScriptError("Unknown keyword '%s'", sc.String);
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================

static void ParseListMenu(FScanner &sc)
{
	sc.MustGetString();

	FListMenuDescriptor *desc = new FListMenuDescriptor;
	desc->mType = MDESC_ListMenu;
	desc->mMenuName = sc.String;
	desc->mSelectedItem = -1;
	desc->mAutoselect = -1;
	desc->mSelectOfsX = DefaultListMenuSettings.mSelectOfsX;
	desc->mSelectOfsY = DefaultListMenuSettings.mSelectOfsY;
	desc->mSelector = DefaultListMenuSettings.mSelector;
	desc->mDisplayTop = DefaultListMenuSettings.mDisplayTop;
	desc->mXpos = DefaultListMenuSettings.mXpos;
	desc->mYpos = DefaultListMenuSettings.mYpos;
	desc->mLinespacing = DefaultListMenuSettings.mLinespacing;
	desc->mNetgameMessage = DefaultListMenuSettings.mNetgameMessage;
	desc->mFont = DefaultListMenuSettings.mFont;
	desc->mFontColor = DefaultListMenuSettings.mFontColor;
	desc->mFontColor2 = DefaultListMenuSettings.mFontColor2;
	desc->mClass = NULL;
	desc->mRedirect = NULL;

	FMenuDescriptor **pOld = MenuDescriptors.CheckKey(desc->mMenuName);
	if (pOld != NULL && *pOld != NULL) delete *pOld;
	MenuDescriptors[desc->mMenuName] = desc;

	ParseListMenuBody(sc, desc);
}

//=============================================================================
//
//
//
//=============================================================================

static void ParseOptionValue(FScanner &sc)
{
	FName optname;

	FOptionValues *val = new FOptionValues;
	sc.MustGetString();
	optname = sc.String;
	sc.MustGetStringName("{");
	while (!sc.CheckString("}"))
	{
		FOptionValues::Pair &pair = val->mValues[val->mValues.Reserve(1)];
		sc.MustGetFloat();
		pair.Value = sc.Float;
		sc.MustGetStringName(",");
		sc.MustGetString();
		pair.Text = strbin1(sc.String);
	}
	FOptionValues **pOld = OptionValues.CheckKey(optname);
	if (pOld != NULL && *pOld != NULL) delete *pOld;
	OptionValues[optname] = val;
}


//=============================================================================
//
//
//
//=============================================================================

static void ParseOptionString(FScanner &sc)
{
	FName optname;

	FOptionValues *val = new FOptionValues;
	sc.MustGetString();
	optname = sc.String;
	sc.MustGetStringName("{");
	while (!sc.CheckString("}"))
	{
		FOptionValues::Pair &pair = val->mValues[val->mValues.Reserve(1)];
		sc.MustGetString();
		pair.Value = DBL_MAX;
		pair.TextValue = sc.String;
		sc.MustGetStringName(",");
		sc.MustGetString();
		pair.Text = strbin1(sc.String);
	}
	FOptionValues **pOld = OptionValues.CheckKey(optname);
	if (pOld != NULL && *pOld != NULL) delete *pOld;
	OptionValues[optname] = val;
}


//=============================================================================
//
//
//
//=============================================================================

static void ParseOptionSettings(FScanner &sc)
{
	sc.MustGetStringName("{");
	while (!sc.CheckString("}"))
	{
		sc.MustGetString();
		if (sc.Compare("ifgame"))
		{
			if (!CheckSkipGameBlock(sc))
			{
				// recursively parse sub-block
				ParseOptionSettings(sc);
			}
		}
		else if (sc.Compare("ifoption"))
		{
			if (!CheckSkipOptionBlock(sc))
			{
				// recursively parse sub-block
				ParseOptionSettings(sc);
			}
		}
		else if (sc.Compare("FontColor"))
		{
			sc.MustGetString();
			OptionSettings.mFontColor = V_FindFontColor((FName)sc.String);
			sc.MustGetStringName(",");
			sc.MustGetString();
			OptionSettings.mFontColorValue = V_FindFontColor((FName)sc.String);
			sc.MustGetStringName(",");
			sc.MustGetString();
			OptionSettings.mFontColorMore = V_FindFontColor((FName)sc.String);
			sc.MustGetStringName(",");
			sc.MustGetString();
			OptionSettings.mFontColorHeader = V_FindFontColor((FName)sc.String);
			sc.MustGetStringName(",");
			sc.MustGetString();
			OptionSettings.mFontColorHighlight = V_FindFontColor((FName)sc.String);
		}
		else if (sc.Compare("Linespacing"))
		{
			sc.MustGetNumber();
			OptionSettings.mLinespacing = sc.Number;
		}
		else if (sc.Compare("LabelOffset"))
		{
			sc.MustGetNumber();
			OptionSettings.mLabelOffset = sc.Number;
		}
		else if (sc.Compare("TitleColor"))
		{
			sc.MustGetString();
			OptionSettings.mTitleColor = V_FindFontColor((FName)sc.String);
		}
		else
		{
			sc.ScriptError("Unknown keyword '%s'", sc.String);
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================

static void ParseOptionMenuBody(FScanner &sc, FOptionMenuDescriptor *desc)
{
	sc.MustGetStringName("{");
	while (!sc.CheckString("}"))
	{
		sc.MustGetString();
		if (sc.Compare("ifgame"))
		{
			if (!CheckSkipGameBlock(sc))
			{
				// recursively parse sub-block
				ParseOptionMenuBody(sc, desc);
			}
		}
		else if (sc.Compare("ifoption"))
		{
			if (!CheckSkipOptionBlock(sc))
			{
				// recursively parse sub-block
				ParseOptionMenuBody(sc, desc);
			}
		}
		else if (sc.Compare("Class"))
		{
			sc.MustGetString();
			const PClass *cls = PClass::FindClass(sc.String);
			if (cls == NULL || !cls->IsDescendantOf(RUNTIME_CLASS(DOptionMenu)))
			{
				sc.ScriptError("Unknown menu class '%s'", sc.String);
			}
			desc->mClass = cls;
		}
		else if (sc.Compare("Title"))
		{
			sc.MustGetString();
			desc->mTitle = sc.String;
		}
		else if (sc.Compare("Position"))
		{
			sc.MustGetNumber();
			desc->mPosition = sc.Number;
		}
		else if (sc.Compare("ScrollTop"))
		{
			sc.MustGetNumber();
			desc->mScrollTop = sc.Number;
		}
		else if (sc.Compare("Indent"))
		{
			sc.MustGetNumber();
			desc->mIndent = sc.Number;
		}
		else if (sc.Compare("Submenu"))
		{
			sc.MustGetString();
			FString label = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FOptionMenuItem *it = new FOptionMenuItemSubmenu(label, sc.String);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("Option"))
		{
			sc.MustGetString();
			FString label = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FString cvar = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FString values = sc.String;
			const char *check = NULL;
			int center = 0;
			if (sc.CheckString(","))
			{
				sc.MustGetString();
				if (*sc.String != 0) check = sc.String;
				if (sc.CheckString(","))
				{
					sc.MustGetNumber();
					center = sc.Number;
				}
			}
			FOptionMenuItem *it = new FOptionMenuItemOption(label, cvar, values, check, center);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("Command"))
		{
			sc.MustGetString();
			FString label = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FOptionMenuItem *it = new FOptionMenuItemCommand(label, sc.String);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("SafeCommand"))
		{
			sc.MustGetString();
			FString label = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FOptionMenuItem *it = new FOptionMenuItemSafeCommand(label, sc.String);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("Control") || sc.Compare("MapControl"))
		{
			bool map = sc.Compare("MapControl");
			sc.MustGetString();
			FString label = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FOptionMenuItem *it = new FOptionMenuItemControl(label, sc.String, map? &AutomapBindings : &Bindings);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("ColorPicker"))
		{
			sc.MustGetString();
			FString label = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FOptionMenuItem *it = new FOptionMenuItemColorPicker(label, sc.String);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("StaticText"))
		{
			sc.MustGetString();
			FString label = sc.String;
			bool cr = false;
			if (sc.CheckString(","))
			{
				sc.MustGetNumber();
				cr = !!sc.Number;
			}
			FOptionMenuItem *it = new FOptionMenuItemStaticText(label, cr);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("StaticTextSwitchable"))
		{
			sc.MustGetString();
			FString label = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FString label2 = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FName action = sc.String;
			bool cr = false;
			if (sc.CheckString(","))
			{
				sc.MustGetNumber();
				cr = !!sc.Number;
			}
			FOptionMenuItem *it = new FOptionMenuItemStaticTextSwitchable(label, label2, action, cr);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("Slider"))
		{
			sc.MustGetString();
			FString text = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FString action = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetFloat();
			double min = sc.Float;
			sc.MustGetStringName(",");
			sc.MustGetFloat();
			double max = sc.Float;
			sc.MustGetStringName(",");
			sc.MustGetFloat();
			double step = sc.Float;
			bool showvalue = true;
			if (sc.CheckString(","))
			{
				sc.MustGetNumber();
				showvalue = !!sc.Number;
			}
			FOptionMenuItem *it = new FOptionMenuSliderItem(text, action, min, max, step, showvalue? 1:-1);
			desc->mItems.Push(it);
		}
		else
		{
			sc.ScriptError("Unknown keyword '%s'", sc.String);
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================

static void ParseOptionMenu(FScanner &sc)
{
	sc.MustGetString();

	FOptionMenuDescriptor *desc = new FOptionMenuDescriptor;
	desc->mType = MDESC_OptionsMenu;
	desc->mMenuName = sc.String;
	desc->mSelectedItem = -1;
	desc->mScrollPos = 0;
	desc->mClass = NULL;
	desc->mPosition = DefaultOptionMenuSettings.mPosition;
	desc->mScrollTop = DefaultOptionMenuSettings.mScrollTop;
	desc->mIndent =  DefaultOptionMenuSettings.mIndent;
	desc->mDontDim =  DefaultOptionMenuSettings.mDontDim;

	FMenuDescriptor **pOld = MenuDescriptors.CheckKey(desc->mMenuName);
	if (pOld != NULL && *pOld != NULL) delete *pOld;
	MenuDescriptors[desc->mMenuName] = desc;

	ParseOptionMenuBody(sc, desc);

	desc->CalcIndent();
}

//=============================================================================
//
//
//
//=============================================================================

void M_ParseMenuDefs()
{
	int lump, lastlump = 0;

	atterm(	DeinitMenus);
	while ((lump = Wads.FindLump ("MENUDEF", &lastlump)) != -1)
	{
		FScanner sc(lump);

		sc.SetCMode(true);
		while (sc.GetString())
		{
			if (sc.Compare("LISTMENU"))
			{
				ParseListMenu(sc);
			}
			else if (sc.Compare("DEFAULTLISTMENU"))
			{
				ParseListMenuBody(sc, &DefaultListMenuSettings);
			}
			else if (sc.Compare("OPTIONVALUE"))
			{
				ParseOptionValue(sc);
			}
			else if (sc.Compare("OPTIONSTRING"))
			{
				ParseOptionString(sc);
			}
			else if (sc.Compare("OPTIONMENUSETTINGS"))
			{
				ParseOptionSettings(sc);
			}
			else if (sc.Compare("OPTIONMENU"))
			{
				ParseOptionMenu(sc);
			}
			else if (sc.Compare("DEFAULTOPTIONMENU"))
			{
				ParseOptionMenuBody(sc, &DefaultOptionMenuSettings);
			}
			else
			{
				sc.ScriptError("Unknown keyword '%s'", sc.String);
			}
		}
	}

	// Build episode menu
	FMenuDescriptor **desc = MenuDescriptors.CheckKey(NAME_Episodemenu);
	if (desc != NULL)
	{
		if ((*desc)->mType == MDESC_ListMenu)
		{
			FListMenuDescriptor *ld = static_cast<FListMenuDescriptor*>(*desc);
			int posy = ld->mYpos;
			ld->mSelectedItem = ld->mItems.Size();
			if (AllEpisodes.Size() >= 5)
			{
				// desc->mYpos -= desc->mLineHeight;
			}
			for(unsigned i = 0; i < AllEpisodes.Size(); i++)
			{
				FListMenuItem *it;
				if (AllEpisodes[i].mPicName.IsNotEmpty())
				{
					FTextureID tex = TexMan.CheckForTexture(AllEpisodes[i].mPicName, FTexture::TEX_MiscPatch);
					it = new FListMenuItemPatch(ld->mXpos, posy, AllEpisodes[i].mShortcut, 
						tex, NAME_Skillmenu, i);
				}
				else
				{
					it = new FListMenuItemText(ld->mXpos, posy, AllEpisodes[i].mShortcut, 
						AllEpisodes[i].mEpisodeName, ld->mFont, ld->mFontColor, NAME_Skillmenu, i);
				}
				ld->mItems.Push(it);
				posy += ld->mLinespacing;
			}
			if (AllEpisodes.Size() == 1)
			{
				ld->mAutoselect = ld->mSelectedItem;
			}
		}
	}

	// Build player class menu
	desc = MenuDescriptors.CheckKey(NAME_Playerclassmenu);
	if (desc != NULL)
	{
		if ((*desc)->mType == MDESC_ListMenu)
		{
			FListMenuDescriptor *ld = static_cast<FListMenuDescriptor*>(*desc);
			// add player display
			ld->mSelectedItem = ld->mItems.Size();
			
			int n = 0;
			for (unsigned i = 0; i < PlayerClasses.Size (); i++)
			{
				if (!(PlayerClasses[i].Flags & PCF_NOMENU))
				{
					const char *pname = PlayerClasses[i].Type->Meta.GetMetaString (APMETA_DisplayName);
					if (pname != NULL)
					{
						FListMenuItemText *it = new FListMenuItemText(ld->mXpos, ld->mYpos, *pname,
							pname, ld->mFont,ld->mFontColor, NAME_Episodemenu, i);
						ld->mItems.Push(it);
						ld->mYpos += ld->mLinespacing;
						n++;
					}
				}
			}
			if (n > 1)
			{
				FListMenuItemText *it = new FListMenuItemText(ld->mXpos, ld->mYpos, 'r',
					"$MNU_RANDOM", ld->mFont,ld->mFontColor, NAME_Episodemenu, -1);
				ld->mItems.Push(it);
			}
			if (n == 0)
			{
				const char *pname = PlayerClasses[0].Type->Meta.GetMetaString (APMETA_DisplayName);
				if (pname != NULL)
				{
					FListMenuItemText *it = new FListMenuItemText(ld->mXpos, ld->mYpos, *pname,
						pname, ld->mFont,ld->mFontColor, NAME_Episodemenu, 0);
					ld->mItems.Push(it);
				}
			}
			if (n < 2)
			{
				ld->mAutoselect = ld->mSelectedItem;
			}
			/*
			if (ClassMenuDef.numitems > 4)
			{
				ClassMenuDef.y -= LINEHEIGHT;
			}
			*/
			/* set default to an item with (NAME_Episodemenu, PlayerClassindex)
			int PlayerClassindex = players[consoleplayer].userinfo.PlayerClass;
			*/

		}
	}
	InitCrosshairsList();

	FOptionValues **opt = OptionValues.CheckKey(NAME_Mididevices);
	if (opt != NULL) 
	{
		I_BuildMIDIMenuList(*opt);
		return;	// no crosshair value list present. No need to go on.
	}


}


// Reads any XHAIRS lumps for the names of crosshairs and
// adds them to the display options menu.
static void InitCrosshairsList()
{
	int lastlump, lump;

	lastlump = 0;

	FOptionValues **opt = OptionValues.CheckKey(NAME_Crosshairs);
	if (opt == NULL) 
	{
		return;	// no crosshair value list present. No need to go on.
	}

	FOptionValues::Pair *pair = &(*opt)->mValues[(*opt)->mValues.Reserve(1)];
	pair->Value = 0;
	pair->Text = "None";

	while ((lump = Wads.FindLump("XHAIRS", &lastlump)) != -1)
	{
		FScanner sc(lump);
		while (sc.GetNumber())
		{
			FOptionValues::Pair value;
			value.Value = sc.Number;
			sc.MustGetString();
			value.Text = sc.String;
			if (value.Value != 0)
			{ // Check if it already exists. If not, add it.
				unsigned int i;

				for (i = 1; i < (*opt)->mValues.Size(); ++i)
				{
					if ((*opt)->mValues[i].Value == value.Value)
					{
						break;
					}
				}
				if (i < (*opt)->mValues.Size())
				{
					(*opt)->mValues[i].Text = value.Text;
				}
				else
				{
					(*opt)->mValues.Push(value);
				}
			}
		}
	}
}

//=============================================================================
//
// THe skill menu must be refeshed each time it starts up
//
//=============================================================================

void M_StartupSkillMenu(FGameStartup *gs)
{
	static bool done = false;
	FMenuDescriptor **desc = MenuDescriptors.CheckKey(NAME_Skillmenu);
	if (desc != NULL)
	{
		if ((*desc)->mType == MDESC_ListMenu)
		{
			FListMenuDescriptor *ld = static_cast<FListMenuDescriptor*>(*desc);
			int x = ld->mXpos;
			int y = ld->mYpos;
			if (gameinfo.gametype == GAME_Hexen)
			{
				// THere really needs to be a better way to do this... :(
				if (gs->PlayerClass != NULL)
				{
					if (!stricmp(gs->PlayerClass, "fighter")) x = 120;
					else if (!stricmp(gs->PlayerClass, "cleric")) x = 116;
					else if (!stricmp(gs->PlayerClass, "mage")) x = 112;
				}
			}
			// Delete previous contents
			for(unsigned i=0; i<ld->mItems.Size(); i++)
			{
				FName n = ld->mItems[i]->GetAction(NULL);
				if (n == NAME_Startgame || n == NAME_StartgameConfirm) 
				{
					for(unsigned j=i; j<ld->mItems.Size(); j++)
					{
						delete ld->mItems[j];
					}
					ld->mItems.Resize(i);
					break;
				}
			}
			if (!done)
			{
				done = true;
				int defskill = DefaultSkill;
				if ((unsigned int)defskill >= AllSkills.Size())
				{
					defskill = (AllSkills.Size() - 1) / 2;
				}

				ld->mSelectedItem = ld->mItems.Size() + defskill;
			}

			unsigned firstitem = ld->mItems.Size();
			for(unsigned int i = 0; i < AllSkills.Size(); i++)
			{
				FSkillInfo &skill = AllSkills[i];
				FListMenuItem *li;
				// Using a different name for skills that must be confirmed makes handling this easier.
				FName action = skill.MustConfirm? NAME_StartgameConfirm : NAME_Startgame;

				FString *pItemText = NULL;
				if (gs->PlayerClass != NULL)
				{
					pItemText = skill.MenuNamesForPlayerClass.CheckKey(gs->PlayerClass);
				}

				if (skill.PicName.Len() != 0 && pItemText == NULL)
				{
					FTextureID tex = TexMan.CheckForTexture(skill.PicName, FTexture::TEX_MiscPatch);
					li = new FListMenuItemPatch(ld->mXpos, y, skill.Shortcut, tex, action, i);
				}
				else
				{
					EColorRange color = (EColorRange)skill.GetTextColor();
					if (color == CR_UNTRANSLATED) color = ld->mFontColor;
					li = new FListMenuItemText(x, y, skill.Shortcut, 
									pItemText? *pItemText : skill.MenuName, ld->mFont, color, action, i);
				}
				ld->mItems.Push(li);
				y += ld->mLinespacing;
			}
			if (AllEpisodes[gs->Episode].mNoSkill || AllSkills.Size() == 1)
			{
				ld->mAutoselect = MIN(2u, AllEpisodes.Size()-1);
			}

		}
	}
}
