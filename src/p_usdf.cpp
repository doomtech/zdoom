//
// p_usdf.cpp
//
// USDF dialogue parser
//
//---------------------------------------------------------------------------
// Copyright (c) 2010
//		Braden "Blzut3" Obrzut <admin@maniacsvault.net>
//		Christoph Oelckers
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//    * Neither the name of the <organization> nor the
//      names of its contributors may be used to endorse or promote products
//      derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "r_data.h"
#include "p_setup.h"
#include "p_lnspec.h"
#include "templates.h"
#include "i_system.h"
#include "p_conversation.h"
#include "p_udmf.h"

#define Zd 1
#define St 2

class USDFParser : public UDMFParserBase
{
	//===========================================================================
	//
	// Checks an actor type (different representation depending on manespace)
	//
	//===========================================================================

	const PClass *CheckActorType(const char *key)
	{
		if (namespace_bits == St)
		{
			return GetStrifeType(CheckInt(key));
		}
		else if (namespace_bits == Zd)
		{
			return PClass::FindClass(CheckString(key));
		}
		return NULL;
	}

	//===========================================================================
	//
	// Parse a cost block
	//
	//===========================================================================

	bool ParseCost(FStrifeDialogueReply *response, int index)
	{
		while (!sc.CheckToken('}'))
		{
			FName key = ParseKey();
			switch(key)
			{
			case NAME_Item:
				response->ItemCheck[index] = CheckActorType(key);
				break;

			case NAME_Amount:
				response->ItemCheckAmount[index] = CheckInt(key);
				break;
			}
		}
		return true;
	}

	//===========================================================================
	//
	// Parse a choice block
	//
	//===========================================================================

	bool ParseChoice(FStrifeDialogueReply **&replyptr)
	{
		FStrifeDialogueReply *reply = new FStrifeDialogueReply;
		memset(reply, 0, sizeof(*reply));

		reply->Next = *replyptr;
		*replyptr = reply;
		replyptr = &reply->Next;

		FString ReplyString;
		FString QuickYes;
		FString QuickNo;
		FString LogString;
		unsigned int costs = 0;
		bool closeDialog = false;


		reply->NeedsGold = true;
		while (!sc.CheckToken('}'))
		{
			bool block = false;
			int costs = 0;
			FName key = ParseKey(true, &block);
			if (!block)
			{
				switch(key)
				{
				case NAME_Text:
					ReplyString = CheckString(key);
					break;

				case NAME_Displaycost:
					reply->NeedsGold = CheckBool(key);
					break;

				case NAME_Yesmessage:
					QuickYes = CheckString(key);
					break;

				case NAME_Nomessage:
					QuickNo = CheckString(key);
					break;

				case NAME_Log:
					if (namespace_bits == St)
					{
						const char *s = CheckString(key);
						if(strlen(s) < 4 || strnicmp(s, "LOG", 3) != 0)
							sc.ScriptMessage("Log must be in the format of LOG# to compile, ignoring.");
						else
						{
							reply->LogNumber = atoi(s + 3);
						}
					}
					else
					{
						LogString = CheckString(key);
					}
					break;

				case NAME_Giveitem:
					reply->GiveType = CheckActorType(key);
					break;

				case NAME_Nextpage:
					reply->NextNode = CheckInt(key);
					break;

				case NAME_Closedialog:
					closeDialog = CheckBool(key);
					break;
				}
			}
			else
			{
				switch(key)
				{
				case NAME_Cost:
					if(costs > 2)
					{
						sc.ScriptMessage ("Too many cost blocks, ignoring.");
						sc.UnGet();
						Skip();
					}
					else
					{
						ParseCost(reply, costs);
						costs++;
					}
					break;

				default:
					sc.UnGet();
					Skip();
				}
			}
		}
		// Todo: Finalize
		if (reply->NeedsGold) ReplyString.AppendFormat(" for %u", reply->ItemCheckAmount[0]);

		reply->Reply = ncopystring(ReplyString);
		reply->QuickYes = ncopystring(QuickYes);
		if (reply->ItemCheck[0] != 0)
		{
			reply->QuickNo = ncopystring(QuickNo);
		}
		else
		{
			reply->QuickNo = NULL;
		}
		reply->LogString = ncopystring(LogString);
		if(!closeDialog) reply->NextNode *= -1;
		return true;
	}

	//===========================================================================
	//
	// Parse an ifitem block
	//
	//===========================================================================

	bool ParseIfItem(FStrifeDialogueNode *node, int index)
	{
		while (!sc.CheckToken('}'))
		{
			FName key = ParseKey();
			switch(key)
			{
			case NAME_Item:
				node->ItemCheck[index] = CheckActorType(key);
				break;

			case NAME_Page:
				// Yes, I know, thats badly implemented - but the spec requires this here.
				if (index == 0)	node->ItemCheckNode = CheckInt(key);
				break;

			case NAME_Count:
				// Not yet implemented in the engine. Todo later
				CheckInt(key);
				break;
			}
		}
		return true;
	}

	//===========================================================================
	//
	// Parse a page block
	//
	//===========================================================================

	bool ParsePage()
	{
		int ifitemcount = 0;
		FStrifeDialogueNode *node = new FStrifeDialogueNode;
		FStrifeDialogueReply **replyptr = &node->Children;
		memset(node, 0, sizeof(*node));
		node->ThisNodeNum = StrifeDialogues.Push(node);

		FString SpeakerName;
		FString Dialogue;

		while (!sc.CheckToken('}'))
		{
			bool block = false;
			FName key = ParseKey(true, &block);
			if (!block)
			{
				switch(key)
				{
				case NAME_Name:
					SpeakerName = CheckString(key);
					break;

				case NAME_Panel:
					node->Backdrop = TexMan.CheckForTexture (CheckString(key), FTexture::TEX_MiscPatch);
					break;

				case NAME_Voice:
					{
						FString soundname = (namespace_bits == St? "svox/" : "");
						const char * name = CheckString(key);
						if (name[0] != 0)
						{
							soundname += name;
							node->SpeakerVoice = FSoundID(S_FindSound(name));
						}
					}
					break;

				case NAME_Dialog:
					Dialogue = CheckString(key);
					break;

				case NAME_Drop:
					node->DropType = CheckActorType(key);
					break;
				}
			}
			else
			{
				switch(key)
				{
				case NAME_Ifitem:
					if (ifitemcount > 2)
					{
						sc.ScriptMessage("Too many ifitem blocks, ignoring.");
						sc.UnGet();
						Skip();
					}
					else if (!ParseIfItem(node, ifitemcount++)) return false;
					break;

				case NAME_Choice:
					if (!ParseChoice(replyptr)) return false;
					break;

				default:
					sc.UnGet();
					Skip();
				}
			}
		}
		node->SpeakerName = ncopystring(SpeakerName);
		node->Dialogue = ncopystring(Dialogue);
		return true;
	}


	//===========================================================================
	//
	// Parse a conversation block
	//
	//===========================================================================

	bool ParseConversation()
	{
		const PClass *type = NULL;
		unsigned int startpos = StrifeDialogues.Size();

		while (!sc.CheckToken('}'))
		{
			bool block = false;
			FName key = ParseKey(true, &block);
			if (!block)
			{
				switch(key)
				{
				case NAME_Actor:
					type = CheckActorType(key);
					break;
				}
			}
			else
			{
				switch(key)
				{
				case NAME_Page:
					if (!ParsePage()) return false;
					break;

				default:
					sc.UnGet();
					Skip();
				}
			}
		}
		if (type == NULL)
		{
			sc.ScriptMessage("No valid actor type defined in conversation.");
			return false;
		}
		for(;startpos < StrifeDialogues.Size(); startpos++)
		{
			StrifeDialogues[startpos]->SpeakerType = type;
		}
		return true;
	}

	//===========================================================================
	//
	// Parse an USDF lump
	//
	//===========================================================================

public:
	bool Parse(int lumpnum)
	{
		sc.OpenLumpNum(lumpnum);
		sc.SetCMode(true);
		// Namespace must be the first field because everything else depends on it.
		if (sc.CheckString("namespace"))
		{
			sc.MustGetStringName("=");
			sc.MustGetString();
			namespc = sc.String;
			switch(namespc)
			{
			case NAME_ZDoom:
				namespace_bits = Zd;
				break;
			case NAME_Strife:
				namespace_bits = St;
				break;
			default:
				sc.ScriptMessage("Unknown namespace %s. Ignoring dialogue lump.\n", sc.String);
				return false;
			}
			sc.MustGetStringName(";");
		}
		else
		{
			sc.ScriptMessage("Map does not define a namespace.\n");
			return false;
		}

		while (sc.GetString())
		{
			if (sc.Compare("conversation"))
			{
				if (!ParseConversation()) return false;
			}
			else if (sc.Compare("include"))
			{
				sc.MustGetToken(TK_StringConst);
				// This must also be able to handle a binary dialogue lump!
			}
			else
			{
				Skip();
			}
		}
		return true;
	}
};



bool P_ParseUSDF(int lumpnum)
{
	USDFParser parse;

	if (!parse.Parse(lumpnum))
	{
		// clean up the incomplete dialogue structures here
		return false;
	}
	return true;
}
