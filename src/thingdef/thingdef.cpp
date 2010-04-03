/*
** thingdef.cpp
**
** Actor definitions
**
**---------------------------------------------------------------------------
** Copyright 2002-2008 Christoph Oelckers
** Copyright 2004-2008 Randy Heit
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
** 4. When not used as part of ZDoom or a ZDoom derivative, this code will be
**    covered by the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or (at
**    your option) any later version.
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

#include "gi.h"
#include "actor.h"
#include "info.h"
#include "sc_man.h"
#include "tarray.h"
#include "w_wad.h"
#include "templates.h"
#include "r_defs.h"
#include "r_draw.h"
#include "a_pickups.h"
#include "s_sound.h"
#include "cmdlib.h"
#include "p_lnspec.h"
#include "a_action.h"
#include "decallib.h"
#include "m_random.h"
#include "i_system.h"
#include "p_local.h"
#include "doomerrors.h"
#include "a_hexenglobal.h"
#include "a_weaponpiece.h"
#include "p_conversation.h"
#include "v_text.h"
#include "thingdef.h"
#include "thingdef_exp.h"
#include "a_sharedglobal.h"
#include "vmbuilder.h"
#include "stats.h"

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------
void InitThingdef();
void ParseDecorate (FScanner &sc);

// STATIC FUNCTION PROTOTYPES --------------------------------------------
PClassActor *QuestItemClasses[31];
PSymbolTable GlobalSymbols;

//==========================================================================
//
// Starts a new actor definition
//
//==========================================================================
PClassActor *CreateNewActor(const FScriptPosition &sc, FName typeName, FName parentName, bool native)
{
	PClassActor *replacee = NULL;
	PClassActor *ti = NULL;

	PClassActor *parent = RUNTIME_CLASS(AActor);

	if (parentName != NAME_None)
	{
		parent = PClass::FindActor(parentName);
		
		PClassActor *p = parent;
		while (p != NULL)
		{
			if (p->TypeName == typeName)
			{
				sc.Message(MSG_ERROR, "'%s' inherits from a class with the same name", typeName.GetChars());
				break;
			}
			p = dyn_cast<PClassActor>(p->ParentClass);
		}

		if (parent == NULL)
		{
			sc.Message(MSG_ERROR, "Parent type '%s' not found in %s", parentName.GetChars(), typeName.GetChars());
			parent = RUNTIME_CLASS(AActor);
		}
		else if (!parent->IsDescendantOf(RUNTIME_CLASS(AActor)))
		{
			sc.Message(MSG_ERROR, "Parent type '%s' is not an actor in %s", parentName.GetChars(), typeName.GetChars());
			parent = RUNTIME_CLASS(AActor);
		}
	}

	if (native)
	{
		ti = PClass::FindActor(typeName);
		if (ti == NULL)
		{
			sc.Message(MSG_ERROR, "Unknown native actor '%s'", typeName.GetChars());
			goto create;
		}
		else if (ti != RUNTIME_CLASS(AActor) && ti->ParentClass->NativeClass() != parent->NativeClass())
		{
			sc.Message(MSG_ERROR, "Native class '%s' does not inherit from '%s'", typeName.GetChars(), parentName.GetChars());
			parent = RUNTIME_CLASS(AActor);
			goto create;
		}
		else if (ti->Defaults != NULL)
		{
			sc.Message(MSG_ERROR, "Redefinition of internal class '%s'", typeName.GetChars());
			goto create;
		}
		ti->InitializeNativeDefaults();
	}
	else
	{
	create:
		ti = static_cast<PClassActor *>(parent->CreateDerivedClass (typeName, parent->Size));
	}

	if (parent->DamageFactors != NULL)
	{
		// copy damage factors from parent
		ti->DamageFactors = new DmgFactors;
		*ti->DamageFactors = *parent->DamageFactors;
	}
	if (parent->PainChances != NULL)
	{
		// copy pain chances from parent
		ti->PainChances = new PainChanceList;
		*ti->PainChances = *parent->PainChances;
	}
	ti->Replacee = ti->Replacement = NULL;
	ti->DoomEdNum = -1;
	return ti;
}

//==========================================================================
//
// 
//
//==========================================================================

void SetReplacement(PClassActor *info, FName replaceName)
{
	// Check for "replaces"
	if (replaceName != NAME_None)
	{
		// Get actor name
		PClassActor *replacee = PClass::FindActor(replaceName);

		if (replacee == NULL)
		{
			I_Error ("Replaced type '%s' not found in %s", replaceName.GetChars(), info->TypeName.GetChars());
		}
		if (replacee != NULL)
		{
			replacee->Replacement = info;
			info->Replacee = replacee;
		}
	}

}

//==========================================================================
//
// Finalizes an actor definition
//
//==========================================================================

void FinishActor(const FScriptPosition &sc, PClassActor *info, Baggage &bag)
{
	AActor *defaults = (AActor*)info->Defaults;

	try
	{
		bag.statedef.FinishStates (info, defaults);
	}
	catch (CRecoverableError &err)
	{
		sc.Message(MSG_ERROR, "%s", err.GetMessage());
		bag.statedef.MakeStateDefines(NULL);
		return;
	}
	bag.statedef.InstallStates (info, defaults);
	bag.statedef.MakeStateDefines(NULL);
	if (bag.DropItemSet)
	{
		info->DropItems = bag.DropItemList;
	}
	if (info->IsDescendantOf (RUNTIME_CLASS(AInventory)))
	{
		defaults->flags |= MF_SPECIAL;
	}

	// Weapons must be checked for all relevant states. They may crash the game otherwise.
	if (info->IsDescendantOf(RUNTIME_CLASS(AWeapon)))
	{
		FState *ready = info->FindState(NAME_Ready);
		FState *select = info->FindState(NAME_Select);
		FState *deselect = info->FindState(NAME_Deselect);
		FState *fire = info->FindState(NAME_Fire);

		// Consider any weapon without any valid state abstract and don't output a warning
		// This is for creating base classes for weapon groups that only set up some properties.
		if (ready || select || deselect || fire)
		{
			if (!ready)
			{
				sc.Message(MSG_ERROR, "Weapon %s doesn't define a ready state.\n", info->TypeName.GetChars());
			}
			if (!select) 
			{
				sc.Message(MSG_ERROR, "Weapon %s doesn't define a select state.\n", info->TypeName.GetChars());
			}
			if (!deselect) 
			{
				sc.Message(MSG_ERROR, "Weapon %s doesn't define a deselect state.\n", info->TypeName.GetChars());
			}
			if (!fire) 
			{
				sc.Message(MSG_ERROR, "Weapon %s doesn't define a fire state.\n", info->TypeName.GetChars());
			}
		}
	}
}

//==========================================================================
//
// Do some postprocessing after everything has been defined
//
//==========================================================================

static void FinishThingdef()
{
	int errorcount = StateParams.ResolveAll();
	unsigned i, j;

	FILE *dump = fopen("disasm.txt", "w");
	for (i = 0; i < StateTempCalls.Size(); ++i)
	{
		FStateTempCall *tcall = StateTempCalls[i];
		VMFunction *func;

		assert(tcall->Function != NULL);
		if (tcall->Parameters.Size() == 0)
		{
			func = tcall->Function;
		}
		else
		{
			FCompileContext ctx(tcall->ActorClass, true);
			for (j = 0; j < tcall->Parameters.Size(); ++j)
			{
				tcall->Parameters[j]->Resolve(ctx);
			}
			VMFunctionBuilder buildit;
			// Allocate registers used to pass parameters in.
			// self, stateowner, state, statecalldata (all are pointers)
			buildit.Registers[REGT_POINTER].Get(4);
			// Emit code to pass the standard action function parameters.
			buildit.Emit(OP_PARAM, 0, REGT_POINTER, 0);
			buildit.Emit(OP_PARAM, 0, REGT_POINTER, 1);
			buildit.Emit(OP_PARAM, 0, REGT_POINTER, 2);
			buildit.Emit(OP_PARAM, 0, REGT_POINTER, 3);
			// Emit code for action parameters.
			for (j = 0; j < tcall->Parameters.Size(); ++j)
			{
				FxExpression *p = /*new FxParameter*/(tcall->Parameters[j]);
				p->Emit(&buildit);
				delete p;
			}
			buildit.Emit(OP_CALL_K, buildit.GetConstantAddress(tcall->Function, ATAG_OBJECT), NAP + j, 0);
			buildit.Emit(OP_RET, 0, REGT_NIL, 0);
			VMScriptFunction *sfunc = buildit.MakeFunction();
			sfunc->NumArgs = NAP;
			func = sfunc;
#if 1
			const char *marks = "=======================================================";
			char label[64];
			int labellen = mysnprintf(label, countof(label), "Function %s.States[%d] (*%d)",
				tcall->ActorClass->TypeName.GetChars(),
				tcall->FirstState, tcall->NumStates);
			fprintf(dump, "\n%.*s %s %.*s", MAX(3, 38 - labellen / 2), marks, label, MAX(3, 38 - labellen / 2), marks);
			fprintf(dump, "\nInteger regs: %-3d  Float regs: %-3d  Address regs: %-3d  String regs: %-3d\nStack size: %d\n",
				sfunc->NumRegD, sfunc->NumRegF, sfunc->NumRegA, sfunc->NumRegS, sfunc->MaxParam);
			VMDumpConstants(dump, sfunc);
			fprintf(dump, "\nDisassembly @ %p:\n", sfunc->Code);
			VMDisasm(dump, sfunc->Code, sfunc->CodeSize, sfunc);
#endif
		}
		for (int k = 0; k < tcall->NumStates; ++k)
		{
			tcall->ActorClass->OwnedStates[tcall->FirstState + k].SetAction(func);
		}
	}
	fclose(dump);

	for (i = 0; i < PClass::m_Types.Size(); i++)
	{
		PClass * ti = PClass::m_Types[i];

		// Skip non-actors
		if (!ti->IsDescendantOf(RUNTIME_CLASS(AActor))) continue;

		if (ti->Size == (unsigned)-1)
		{
			Printf("Class %s referenced but not defined\n", ti->TypeName.GetChars());
			errorcount++;
			continue;
		}

		AActor *def = GetDefaultByType(ti);

		if (!def)
		{
			Printf("No ActorInfo defined for class '%s'\n", ti->TypeName.GetChars());
			errorcount++;
			continue;
		}
	}
	if (errorcount > 0)
	{
		I_Error("%d errors during actor postprocessing", errorcount);
	}

	// Since these are defined in DECORATE now the table has to be initialized here.
	for(int i = 0; i < 31; i++)
	{
		char fmt[20];
		mysnprintf(fmt, countof(fmt), "QuestItem%d", i+1);
		QuestItemClasses[i] = PClass::FindActor(fmt);
	}
}



//==========================================================================
//
// LoadActors
//
// Called from FActor::StaticInit()
//
//==========================================================================

void LoadActors ()
{
	int lastlump, lump;
	cycle_t timer;

	timer.Reset(); timer.Clock();
	FScriptPosition::ResetErrorCounter();
	InitThingdef();
	lastlump = 0;
	while ((lump = Wads.FindLump ("DECORATE", &lastlump)) != -1)
	{
		FScanner sc(lump);
		ParseDecorate (sc);
	}
	if (FScriptPosition::ErrorCounter > 0)
	{
		I_Error("%d errors while parsing DECORATE scripts", FScriptPosition::ErrorCounter);
	}
	FinishThingdef();
	timer.Unclock();
	Printf("DECORATE parsing took %.2f ms\n", timer.TimeMS());
	// Base time: ~52 ms
}

