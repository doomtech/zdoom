/*
** optionmenuitems.h
** Control items for option menus
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


void M_DrawConText (int color, int x, int y, const char *str);
void M_DrawSlider (int x, int y, double min, double max, double cur,int fracdigits);



//=============================================================================
//
// opens a submenu, action is a submenu name
//
//=============================================================================

class FOptionMenuItemSubmenu : public FOptionMenuItem
{
public:
	FOptionMenuItemSubmenu(const char *label, const char *menu)
		: FOptionMenuItem(label, menu)
	{
	}

	void Draw(FOptionMenuDescriptor *desc, int y, int indent)
	{
		drawLabel(indent, y, OptionSettings.mFontColorMore);
	}

	bool Activate()
	{
		S_Sound (CHAN_VOICE | CHAN_UI, "menu/choose", snd_menuvolume, ATTN_NONE);
		M_SetMenu(mAction, 0);
		return true;
	}
};


//=============================================================================
//
// Executes a CCMD, action is a CCMD name
//
//=============================================================================

class FOptionMenuItemCommand : public FOptionMenuItemSubmenu
{
public:
	FOptionMenuItemCommand(const char *label, const char *menu)
		: FOptionMenuItemSubmenu(label, menu)
	{
	}

	bool Activate()
	{
		C_DoCommand(mAction);
		return true;
	}

};

//=============================================================================
//
// Executes a CCMD after confirmation, action is a CCMD name
//
//=============================================================================

class FOptionMenuItemSafeCommand : public FOptionMenuItemCommand
{
	// action is a CCMD
public:
	FOptionMenuItemSafeCommand(const char *label, const char *menu)
		: FOptionMenuItemCommand(label, menu)
	{
	}

	bool MenuEvent (int mkey, bool fromcontroller)
	{
		if (mkey == MKEY_MBYes)
		{
			C_DoCommand(mAction);
			return true;
		}
		return FOptionMenuItemCommand::MenuEvent(mkey, fromcontroller);
	}

	bool Activate()
	{
		M_StartMessage("Do you really want to do this?", 0);
		return true;
	}
};

//=============================================================================
//
// Change a CVAR, action is the CVAR name
//
//=============================================================================

class FOptionMenuItemOption : public FOptionMenuItem
{
	// action is a CVAR
	FOptionValues *mValues;
	FBaseCVar *mCVar;
	FBoolCVar *mGrayCheck;
	int mSelection;
public:

	FOptionMenuItemOption(const char *label, const char *menu, const char *values, const char *graycheck)
		: FOptionMenuItem(label, menu)
	{
		FOptionValues **opt = OptionValues.CheckKey(values);
		if (opt != NULL) 
		{
			mValues = *opt;
			mSelection = -2;
		}
		else
		{
			mValues = NULL;
			mSelection = -1;
		}
		mGrayCheck = (FBoolCVar*)FindCVar(graycheck, NULL);
		if (mGrayCheck != NULL && mGrayCheck->GetRealType() != CVAR_Bool) mGrayCheck = NULL;
	}

	void SetSelection()
	{
		mSelection = -1;
		if (mValues != NULL) 
		{
			mCVar = FindCVar(mAction, NULL);
			if (mCVar != NULL)
			{
				UCVarValue cv = mCVar->GetGenericRep(CVAR_Float);
				for(unsigned i=0;i<mValues->mValues.Size(); i++)
				{
					if (fabs(cv.Float - mValues->mValues[i].Value) < FLT_EPSILON)
					{
						mSelection = i;
						break;
					}
				}
			}
		}
	}

	//=============================================================================
	void Draw(FOptionMenuDescriptor *desc, int y, int indent)
	{
		bool grayed = mGrayCheck != NULL && !(**mGrayCheck);
		drawLabel(indent, y, OptionSettings.mFontColor, grayed);

		if (mSelection == -2)
		{
			SetSelection();
		}
		if (mValues != NULL && mCVar != NULL)
		{
			int overlay = grayed? MAKEARGB(96,48,0,0) : 0;
			const char *text;
			if (mSelection < 0)
			{
				text = "Unknown";
			}
			else
			{
				text = mValues->mValues[mSelection].Text;
			}
			screen->DrawText (SmallFont, OptionSettings.mFontColorValue, indent + CURSORSPACE, y, 
				text, DTA_CleanNoMove_1, true, DTA_ColorOverlay, overlay, TAG_DONE);
		}
	}

	//=============================================================================
	bool MenuEvent (int mkey, bool fromcontroller)
	{
		UCVarValue value;
		if (mValues != NULL && mCVar != NULL)
		{
			if (mkey == MKEY_Left)
			{
				if (mSelection == -1) mSelection = 0;
				else if (--mSelection < 0) mSelection = mValues->mValues.Size()-1;
			}
			else if (mkey == MKEY_Right || mkey == MKEY_Enter)
			{
				if (++mSelection >= (int)mValues->mValues.Size()) mSelection = 0;
			}
			else
			{
				return FOptionMenuItem::MenuEvent(mkey, fromcontroller);
			}
			value.Float = (float)mValues->mValues[mSelection].Value;
			mCVar->SetGenericRep (value, CVAR_Float);
			S_Sound (CHAN_VOICE | CHAN_UI, "menu/change", snd_menuvolume, ATTN_NONE);
			return true;
		}
		return false;
	}

	bool Selectable()
	{
		return !(mGrayCheck != NULL && !(**mGrayCheck));
	}
};

//=============================================================================
//
// This class is used to capture the key to be used as the new key binding
// for a control item
//
//=============================================================================

class DEnterKey : public DMenu
{
	DECLARE_CLASS(DEnterKey, DMenu)

	int *pKey;

public:
	DEnterKey(DMenu *parent, int *keyptr)
	: DMenu(parent)
	{
		pKey = keyptr;
		SetMenuMessage(1);
		menuactive = MENU_WaitKey;	// There should be a better way to disable GUI capture...
	}

	bool TranslateKeyboardEvents()
	{
		return false; 
	}

	void SetMenuMessage(int which)
	{
		if (mParentMenu->IsKindOf(RUNTIME_CLASS(DListMenu)))
		{
			DListMenu *m = barrier_cast<DListMenu*>(mParentMenu);
			FListMenuItem *it = m->GetItem(NAME_Controlmessage);
			if (it != NULL)
			{
				it->SetValue(0, which);
			}
		}
	}

	bool Responder(event_t *ev)
	{
		if (ev->type == EV_KeyDown)
		{
			*pKey = ev->data1;
			menuactive = MENU_On;
			SetMenuMessage(0);
			Close();
			mParentMenu->MenuEvent((ev->data1 == KEY_ESCAPE)? MKEY_Abort : MKEY_Input, 0);
			return true;
		}
		return false;
	}

	void Drawer()
	{
		mParentMenu->Drawer();
	}
};

#ifndef NO_IMP
IMPLEMENT_ABSTRACT_CLASS(DEnterKey)
#endif

//=============================================================================
//
// // Edit a key binding, Action is the CCMD to bind
//
//=============================================================================

class FOptionMenuItemControl : public FOptionMenuItem
{
	FKeyBindings *mBindings;
	int mKey1, mKey2;
	int mInput;
	bool mWaiting;
public:

	FOptionMenuItemControl(const char *label, const char *menu, FKeyBindings *bindings)
		: FOptionMenuItem(label, menu)
	{
		mBindings = bindings;
		mBindings->GetKeysForCommand(mAction, &mKey1, &mKey2);
		mWaiting = false;
	}

	//=============================================================================
	void Draw(FOptionMenuDescriptor *desc, int y, int indent)
	{
		drawLabel(indent, y, mWaiting? OptionSettings.mFontColorHighlight: OptionSettings.mFontColor);

		char description[64];

		C_NameKeys (description, mKey1, mKey2);
		if (description[0])
		{
			M_DrawConText(CR_WHITE, indent + CURSORSPACE, y-1+OptionSettings.mLabelOffset, description);
		}
		else
		{
			screen->DrawText(SmallFont, CR_BLACK, indent + CURSORSPACE, y + OptionSettings.mLabelOffset, "---",
				DTA_CleanNoMove_1, true, TAG_DONE);
		}
	}

	//=============================================================================
	bool MenuEvent(int mkey, bool fromcontroller)
	{
		if (mkey == MKEY_Input)
		{
			mWaiting = false;
			mBindings->SetBind(mInput, mAction);
			mBindings->GetKeysForCommand(mAction, &mKey1, &mKey2);
			return true;
		}
		else if (mkey == MKEY_Clear)
		{
			mBindings->UnbindACommand(mAction);
			mBindings->GetKeysForCommand(mAction, &mKey1, &mKey2);
			return true;
		}
		else if (mkey == MKEY_Abort)
		{
			mWaiting = false;
			return true;
		}
		return false;
	}

	bool Activate()
	{
		mWaiting = true;
		DMenu *input = new DEnterKey(DMenu::CurrentMenu, &mInput);
		M_ActivateMenu(input);
		return true;
	}
};

//=============================================================================
//
//
//
//=============================================================================

class FOptionMenuItemStaticText : public FOptionMenuItem
{
	EColorRange mColor;
public:
	FOptionMenuItemStaticText(const char *label, bool header)
		: FOptionMenuItem(label, NAME_None, true)
	{
		mColor = header? OptionSettings.mFontColorHeader : OptionSettings.mFontColor;
	}

	void Draw(FOptionMenuDescriptor *desc, int y, int indent)
	{
		drawLabel(indent, y, mColor);
	}

	bool Selectable()
	{
		return false;
	}

};

//=============================================================================
//
//
//
//=============================================================================

class FOptionMenuItemStaticTextSwitchable : public FOptionMenuItem
{
	EColorRange mColor;
	FString mAltText;
	int mCurrent;

public:
	FOptionMenuItemStaticTextSwitchable(const char *label, const char *label2, FName action, bool header)
		: FOptionMenuItem(label, action, true)
	{
		mColor = header? OptionSettings.mFontColorHeader : OptionSettings.mFontColor;
		mAltText = label2;
		mCurrent = 0;
	}

	void Draw(FOptionMenuDescriptor *desc, int y, int indent)
	{
		int w = SmallFont->StringWidth(mLabel) * CleanXfac_1;
		int x = (screen->GetWidth() - w) / 2;
		screen->DrawText (SmallFont, mColor, x, y, mCurrent? mAltText : mLabel, DTA_CleanNoMove_1, true, TAG_DONE);
	}

	bool SetValue(int i, int val)
	{
		if (i == 0) 
		{
			mCurrent = val;
			return true;
		}
		return false;
	}

	bool Selectable()
	{
		return false;
	}
};

//=============================================================================
//
//
//
//=============================================================================

class FOptionMenuSliderItem : public FOptionMenuItem
{
	// action is a CVAR
	float mMin, mMax, mStep;
	float mValue;
	int mShowValue;
	FBaseCVar *mCVar;
	float *mPVal;
public:
	FOptionMenuSliderItem(const char *label, const char *menu, double min, double max, double step, int showval)
		: FOptionMenuItem(label, NAME_None)
	{
		mMin = (float)min;
		mMax = (float)max;
		mStep = (float)step;
		mShowValue = showval;
		mCVar = FindCVar(menu, NULL);
		mPVal = NULL;
	}

	FOptionMenuSliderItem(const char *label, float *pVal, double min, double max, double step, int showval)
		: FOptionMenuItem(label, NAME_None)
	{
		mMin = (float)min;
		mMax = (float)max;
		mStep = (float)step;
		mShowValue = showval;
		mPVal = pVal;
		mCVar = NULL;
	}

	//=============================================================================
	void Draw(FOptionMenuDescriptor *desc, int y, int indent)
	{
		drawLabel(indent, y, OptionSettings.mFontColor);

		UCVarValue value;

		if (mCVar != NULL)
		{
			value = mCVar->GetGenericRep(CVAR_Float);
		}
		else if (mPVal != NULL)
		{
			value.Float = *mPVal;		
		}
		else return;
		M_DrawSlider (indent + CURSORSPACE, y + OptionSettings.mLabelOffset, mMin, mMax, value.Float, mShowValue);
	}

	//=============================================================================
	bool MenuEvent (int mkey, bool fromcontroller)
	{
		UCVarValue value;

		if (mCVar != NULL || mPVal != NULL)
		{
			if (mCVar != NULL)
			{
				value = mCVar->GetGenericRep(CVAR_Float);
			}
			else if (mPVal != NULL)
			{
				value.Float = *mPVal;		
			}

			if (mkey == MKEY_Left)
			{
				value.Float -= mStep;
			}
			else if (mkey == MKEY_Right)
			{
				value.Float += mStep;
			}
			else
			{
				return FOptionMenuItem::MenuEvent(mkey, fromcontroller);
			}
			value.Float = clamp(value.Float, mMin, mMax);
			if (mCVar != NULL) mCVar->SetGenericRep (value, CVAR_Float);
			else if (mPVal != NULL) *mPVal = value.Float;
			S_Sound (CHAN_VOICE | CHAN_UI, "menu/change", snd_menuvolume, ATTN_NONE);
			return true;
		}
		return false;
	}

};

//=============================================================================
//
// // Edit a key binding, Action is the CCMD to bind
//
//=============================================================================

class FOptionMenuItemColorPicker : public FOptionMenuItem
{
	FColorCVar *mCVar;
public:

	enum
	{
		CPF_RESET = 0x20001,
	};

	FOptionMenuItemColorPicker(const char *label, const char *menu)
		: FOptionMenuItem(label, menu)
	{
		FBaseCVar *cv = FindCVar(menu, NULL);
		if (cv->GetRealType() == CVAR_Color)
		{
			mCVar = (FColorCVar*)cv;
		}
		else mCVar = NULL;
	}

	//=============================================================================
	void Draw(FOptionMenuDescriptor *desc, int y, int indent)
	{
		drawLabel(indent, y, OptionSettings.mFontColor);

		if (mCVar != NULL)
		{
			int box_x = indent + CURSORSPACE;
			int box_y = y + OptionSettings.mLabelOffset * CleanYfac_1 / 2;
			screen->Clear (box_x, box_y, box_x + 32*CleanXfac_1, box_y + (SmallFont->GetHeight() - 1) * CleanYfac_1,
				-1, (uint32)*mCVar | 0xff000000);
		}
	}

	bool SetValue(int i, int v)
	{
		if (i == CPF_RESET && mCVar != NULL)
		{
			mCVar->ResetToDefault();
			return true;
		}
		return false;
	}

	bool Activate()
	{
		if (mCVar != NULL)
		{
			S_Sound (CHAN_VOICE | CHAN_UI, "menu/choose", snd_menuvolume, ATTN_NONE);
			DMenu *picker = StartPickerMenu(DMenu::CurrentMenu, mLabel, mCVar);
			if (picker != NULL)
			{
				M_ActivateMenu(picker);
				return true;
			}
		}
		return false;
	}
};

#ifndef NO_IMP
CCMD(am_restorecolors)
{
	if (DMenu::CurrentMenu != NULL && DMenu::CurrentMenu->IsKindOf(RUNTIME_CLASS(DOptionMenu)))
	{
		DOptionMenu *m = (DOptionMenu*)DMenu::CurrentMenu;
		const FOptionMenuDescriptor *desc = m->GetDescriptor();
		// Find the color cvars by scanning the MapColors menu.
		for (unsigned i = 0; i < desc->mItems.Size(); ++i)
		{
			desc->mItems[i]->SetValue(FOptionMenuItemColorPicker::CPF_RESET, 0);
		}
	}
}
#endif



/*
		if (item->type != screenres &&
			item->type != joymore && (item->type != discrete || item->c.discretecenter != 1))
*/

		