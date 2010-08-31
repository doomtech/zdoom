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
#include "menu/menu.h"
#include "w_wad.h"
#include "sc_man.h"
#include "v_font.h"
#include "g_level.h"
#include "d_player.h"
#include "v_video.h"
#include "gi.h"

MenuDescriptorList MenuDescriptors;
static FListMenuDescriptor DefaultListMenuSettings;	// contains common settings for all list menus


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
		else if (sc.Compare("StaticPatch"))
		{
			sc.MustGetNumber();
			int x = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			int y = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FTextureID tex = TexMan.CheckForTexture(sc.String, FTexture::TEX_MiscPatch);

			FListMenuItem *it = new FListMenuItemStaticPatch(x, y, tex);
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
				desc->mFontColor = V_FindFontColor((FName)sc.String);
			}
			else
			{
				desc->mFontColor = CR_UNTRANSLATED;
			}
		}
		else if (sc.Compare("NetgameMessage"))
		{
			sc.MustGetString();
			desc->mNetgameMessage = sc.String;
		}
		else if (sc.Compare("HexenPlayerDisplay"))
		{
			sc.MustGetNumber();
			int x = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			int y = sc.Number;
			sc.MustGetStringName(",");
			FListMenuItemHexenPlayer *hex = new FListMenuItemHexenPlayer(desc, x, y);
			desc->mItems.Push(hex);
			do
			{
				sc.MustGetString();
				hex->AddFrame(sc.String);
				sc.MustGetStringName(",");
				sc.MustGetString();
				hex->AddAnimation(sc.String);
			}
			while (sc.CheckString(","));
		}
		else if (sc.Compare("PlayerDisplay"))
		{
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
			FListMenuItemPlayerDisplay *it = new FListMenuItemPlayerDisplay(desc, x, y, c1, c2);
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

void M_ParseMenuDefs()
{
	int lump, lastlump = 0;

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
			if (gameinfo.gametype == GAME_Hexen && PlayerClasses.Size () == 3 &&
				PlayerClasses[0].Type->IsDescendantOf (PClass::FindClass (NAME_FighterPlayer)) &&
				PlayerClasses[1].Type->IsDescendantOf (PClass::FindClass (NAME_ClericPlayer)) &&
				PlayerClasses[2].Type->IsDescendantOf (PClass::FindClass (NAME_MagePlayer)))
			{
				// Use Hexen's standard PlayerClass menu
				FMenuDescriptor **desc2 = MenuDescriptors.CheckKey(NAME_HexenDefaultPlayerclassmenu);
				if (desc2 != NULL)
				{
					// Replace the generic player class menu with the special Hexen version.
					if ((*desc2)->mType == MDESC_ListMenu)
					{
						(*desc2)->mMenuName = (*desc)->mMenuName;
						delete *desc;
						*desc = *desc2;
						*desc2 = NULL;

					}
				}
			}
			else
			{
				FListMenuDescriptor *ld = static_cast<FListMenuDescriptor*>(*desc);
				// add player display
				ld->mSelectedItem = ld->mItems.Size();
				if (PlayerClasses.Size() == 1)
				{
					ld->mAutoselect = ld->mSelectedItem;
				}
				
				int n = 0;
				for (unsigned i = 0; i < PlayerClasses.Size (); i++, n++)
				{
					if (!(PlayerClasses[i].Flags & PCF_NOMENU))
					{
						const char *pname = PlayerClasses[i].Type->Meta.GetMetaString (APMETA_DisplayName);
						if (pname != NULL)
						{
							FListMenuItemText *it = new FListMenuItemText(ld->mXpos, ld->mYpos, *pname,
								pname, ld->mFont,ld->mFontColor, NAME_Episodemenu, i);
							ld->mItems.Push(it);
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
				/*
				if (ClassMenuDef.numitems > 4)
				{
					ClassMenuDef.y -= LINEHEIGHT;
				}
				*/
			}
			/* set default to an item with (NAME_Episodemenu, PlayerClassindex)
			int PlayerClassindex = players[consoleplayer].userinfo.PlayerClass;
			*/

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
