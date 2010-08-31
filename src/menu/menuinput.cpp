/*
** menuinput.cpp
** The string input code
**
**---------------------------------------------------------------------------
** Copyright 2001-2010 Randy Heit
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
#include "v_video.h"
#include "d_event.h"
#include "d_gui.h"
#include "v_font.h"
#include "v_palette.h"

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

};

IMPLEMENT_ABSTRACT_CLASS(DTextEnterMenu)

#define INPUTGRID_WIDTH		13
#define INPUTGRID_HEIGHT	5

// Heretic and Hexen do not, by default, come with glyphs for all of these
// characters. Oh well. Doom and Strife do.
static const char InputGridChars[INPUTGRID_WIDTH * INPUTGRID_HEIGHT] =
	"ABCDEFGHIJKLM"
	"NOPQRSTUVWXYZ"
	"0123456789+-="
	".,!?@'\":;[]()"
	"<>^#$%&*/_ \b";


//=============================================================================
//
//
//
//=============================================================================

DTextEnterMenu::DTextEnterMenu(DMenu *parent, char *textbuffer, int maxlen, int sizemode, bool showgrid)
{
	mEnterString = textbuffer;
	mEnterSize = maxlen;
	mEnterPos = (unsigned)strlen(textbuffer);
	mSizeMode = sizemode;
	mInputGridOkay = showgrid;
	InputGridX = INPUTGRID_WIDTH - 1;
	InputGridY = INPUTGRID_HEIGHT - 1;
}


//=============================================================================
//
//
//
//=============================================================================

bool DTextEnterMenu::Responder(event_t *ev)
{
	if (ev->type == EV_GUI_Event)
	{
		// Save game and player name string input
		if (ev->subtype == EV_GUI_Char)
		{
			mInputGridOkay = false;
			if (mEnterPos < mEnterSize &&
				(mSizeMode == 2/*entering player name*/ || (size_t)SmallFont->StringWidth(mEnterString) < (mEnterSize-1)*8))
			{
				mEnterString[mEnterPos] = (char)ev->data1;
				mEnterString[++mEnterPos] = 0;
			}
			return true;
		}
		char ch = (char)ev->data1;
		if ((ev->subtype == EV_GUI_KeyDown || ev->subtype == EV_GUI_KeyRepeat) && ch == '\b')
		{
			if (mEnterPos > 0)
			{
				mEnterPos--;
				mEnterString[mEnterPos] = 0;
			}
		}
		else if (ev->subtype == EV_GUI_KeyDown)
		{
			if (ch == GK_ESCAPE)
			{
				mParentMenu->MenuEvent(MKEY_Abort, false);
				Destroy();
				return true;
			}
			else if (ch == '\r')
			{
				if (mEnterString[0])
				{
					mParentMenu->MenuEvent(MKEY_Input, false);
					Destroy();
					return true;
				}
			}
		}
		if (ev->subtype == EV_GUI_KeyDown || ev->subtype == EV_GUI_KeyRepeat)
		{
			return true;
		}
	}
	return false;
}
		
//=============================================================================
//
//
//
//=============================================================================

bool DTextEnterMenu::MenuEvent (int key, bool fromcontroller)
{
	if (key == MKEY_Back)
	{
		mParentMenu->MenuEvent(MKEY_Abort, false);
		Destroy();
		return true;
	}
	if (fromcontroller)
	{
		int ch;

		mInputGridOkay = true;
		switch (key)
		{
		case MKEY_Down:
			InputGridY = (InputGridY + 1) % INPUTGRID_HEIGHT;
			break;

		case MKEY_Up:
			InputGridY = (InputGridY + INPUTGRID_HEIGHT - 1) % INPUTGRID_HEIGHT;
			break;

		case MKEY_Right:
			InputGridX = (InputGridX + 1) % INPUTGRID_WIDTH;
			break;

		case MKEY_Left:
			InputGridX = (InputGridX + INPUTGRID_WIDTH - 1) % INPUTGRID_WIDTH;
			break;

		case MKEY_Clear:
			if (mEnterPos > 0)
			{
				mEnterString[--mEnterPos] = 0;
			}
			break;

		case MKEY_Enter:
			assert(unsigned(InputGridX) < INPUTGRID_WIDTH && unsigned(InputGridY) < INPUTGRID_HEIGHT);
			if (mInputGridOkay)
			{
				ch = InputGridChars[InputGridX + InputGridY * INPUTGRID_WIDTH];
				if (ch == 0)			// end
				{
					if (mEnterString[0] != '\0')
					{
						mParentMenu->MenuEvent(MKEY_Input, true);
						Destroy();
						return true;
					}
				}
				else if (ch == '\b')	// bs
				{
					if (mEnterPos > 0)
					{
						mEnterString[--mEnterPos] = 0;
					}
				}
				else if (mEnterPos < mEnterSize &&
					(mSizeMode == 2/*entering player name*/ || (size_t)SmallFont->StringWidth(mEnterString) < (mEnterSize-1)*8))
				{
					mEnterString[mEnterPos] = ch;
					mEnterString[++mEnterPos] = 0;
				}
			}
			break;

		default:
			break;	// Keep GCC quiet
		}
	}
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

void DTextEnterMenu::Drawer ()
{
	mParentMenu->Drawer();
	if (mInputGridOkay)
	{
		const int cell_width = 18 * CleanXfac;
		const int cell_height = 12 * CleanYfac;
		const int top_padding = cell_height / 2 - SmallFont->GetHeight() * CleanYfac / 2;

		// Darken the background behind the character grid.
		// Unless we frame it with a border, I think it looks better to extend the
		// background across the full width of the screen.
		screen->Dim(0, 0.8f,
			0 /*screen->GetWidth()/2 - 13 * cell_width / 2*/,
			screen->GetHeight() - 5 * cell_height,
			screen->GetWidth() /*13 * cell_width*/,
			5 * cell_height);

		// Highlight the background behind the selected character.
		screen->Dim(MAKERGB(255,248,220), 0.6f,
			InputGridX * cell_width - INPUTGRID_WIDTH * cell_width / 2 + screen->GetWidth() / 2,
			InputGridY * cell_height - INPUTGRID_HEIGHT * cell_height + screen->GetHeight(),
			cell_width, cell_height);

		for (int y = 0; y < INPUTGRID_HEIGHT; ++y)
		{
			const int yy = y * cell_height - INPUTGRID_HEIGHT * cell_height + screen->GetHeight();
			for (int x = 0; x < INPUTGRID_WIDTH; ++x)
			{
				int width;
				const int xx = x * cell_width - INPUTGRID_WIDTH * cell_width / 2 + screen->GetWidth() / 2;
				const int ch = InputGridChars[y * INPUTGRID_WIDTH + x];
				FTexture *pic = SmallFont->GetChar(ch, &width);
				EColorRange color;
				FRemapTable *remap;

				// The highlighted character is yellow; the rest are dark gray.
				color = (x == InputGridX && y == InputGridY) ? CR_YELLOW : CR_DARKGRAY;
				remap = SmallFont->GetColorTranslation(color);

				if (pic != NULL)
				{
					// Draw a normal character.
					screen->DrawTexture(pic, xx + cell_width/2 - width*CleanXfac/2, yy + top_padding,
						DTA_Translation, remap,
						DTA_CleanNoMove, true,
						TAG_DONE);
				}
				else if (ch == ' ')
				{
					// Draw the space as a box outline. We also draw it 50% wider than it really is.
					const int x1 = xx + cell_width/2 - width * CleanXfac * 3 / 4;
					const int x2 = x1 + width * 3 * CleanXfac / 2;
					const int y1 = yy + top_padding;
					const int y2 = y1 + SmallFont->GetHeight() * CleanYfac;
					const int palentry = remap->Remap[remap->NumEntries*2/3];
					const uint32 palcolor = remap->Palette[remap->NumEntries*2/3];
					screen->Clear(x1, y1, x2, y1+CleanYfac, palentry, palcolor);	// top
					screen->Clear(x1, y2, x2, y2+CleanYfac, palentry, palcolor);	// bottom
					screen->Clear(x1, y1+CleanYfac, x1+CleanXfac, y2, palentry, palcolor);	// left
					screen->Clear(x2-CleanXfac, y1+CleanYfac, x2, y2, palentry, palcolor);	// right
				}
				else if (ch == '\b' || ch == 0)
				{
					// Draw the backspace and end "characters".
					const char *const str = ch == '\b' ? "BS" : "ED";
					screen->DrawText(SmallFont, color,
						xx + cell_width/2 - SmallFont->StringWidth(str)*CleanXfac/2,
						yy + top_padding, str, DTA_CleanNoMove, true, TAG_DONE);
				}
			}
		}
	}
}