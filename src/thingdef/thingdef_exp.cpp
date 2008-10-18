/*
** thingdef_exp.cpp
**
** Expression parsing / runtime evaluating support
**
**---------------------------------------------------------------------------
** Copyright 2005 Jan Cholasta
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

#include "actor.h"
#include "sc_man.h"
#include "tarray.h"
#include "templates.h"
#include "cmdlib.h"
#include "i_system.h"
#include "m_random.h"
#include "a_pickups.h"
#include "thingdef.h"
#include "p_lnspec.h"
#include "doomstat.h"
#include "thingdef_exp.h"

FRandom pr_exrandom ("EX_Random");

extern PSymbolTable		 GlobalSymbols;






typedef ExpVal (*ExpVarGet) (AActor *, int);

ExpVal GetAlpha (AActor *actor, int id)
{
	ExpVal val;
	val.Type = VAL_Float;
	val.Float = FIXED2FLOAT (actor->alpha);
	return val;
}

ExpVal GetAngle (AActor *actor, int id)
{
	ExpVal val;
	val.Type = VAL_Float;
	val.Float = (double)actor->angle / ANGLE_1;
	return val;
}

ExpVal GetArgs (AActor *actor, int id)
{
	ExpVal val;
	val.Type = VAL_Int;
	val.Int = actor->args[id];
	return val;
}

ExpVal GetCeilingZ (AActor *actor, int id)
{
	ExpVal val;
	val.Type = VAL_Float;
	val.Float = FIXED2FLOAT (actor->ceilingz);
	return val;
}

ExpVal GetFloorZ (AActor *actor, int id)
{
	ExpVal val;
	val.Type = VAL_Float;
	val.Float = FIXED2FLOAT (actor->floorz);
	return val;
}

ExpVal GetHealth (AActor *actor, int id)
{
	ExpVal val;
	val.Type = VAL_Int;
	val.Int = actor->health;
	return val;
}

ExpVal GetPitch (AActor *actor, int id)
{
	ExpVal val;
	val.Type = VAL_Float;
	val.Float = (double)actor->pitch / ANGLE_1;
	return val;
}

ExpVal GetSpecial (AActor *actor, int id)
{
	ExpVal val;
	val.Type = VAL_Int;
	val.Int = actor->special;
	return val;
}

ExpVal GetTID (AActor *actor, int id)
{
	ExpVal val;
	val.Type = VAL_Int;
	val.Int = actor->tid;
	return val;
}

ExpVal GetTIDToHate (AActor *actor, int id)
{
	ExpVal val;
	val.Type = VAL_Int;
	val.Int = actor->TIDtoHate;
	return val;
}

ExpVal GetWaterLevel (AActor *actor, int id)
{
	ExpVal val;
	val.Type = VAL_Int;
	val.Int = actor->waterlevel;
	return val;
}

ExpVal GetX (AActor *actor, int id)
{
	ExpVal val;
	val.Type = VAL_Float;
	val.Float = FIXED2FLOAT (actor->x);
	return val;
}

ExpVal GetY (AActor *actor, int id)
{
	ExpVal val;
	val.Type = VAL_Float;
	val.Float = FIXED2FLOAT (actor->y);
	return val;
}

ExpVal GetZ (AActor *actor, int id)
{
	ExpVal val;
	val.Type = VAL_Float;
	val.Float = FIXED2FLOAT (actor->z);
	return val;
}

ExpVal GetMomX (AActor *actor, int id)
{
	ExpVal val;
	val.Type = VAL_Float;
	val.Float = FIXED2FLOAT (actor->momx);
	return val;
}

ExpVal GetMomY (AActor *actor, int id)
{
	ExpVal val;
	val.Type = VAL_Float;
	val.Float = FIXED2FLOAT (actor->momy);
	return val;
}

ExpVal GetMomZ (AActor *actor, int id)
{
	ExpVal val;
	val.Type = VAL_Float;
	val.Float = FIXED2FLOAT (actor->momz);
	return val;
}

static struct FExpVar
{
	ENamedName name;	// identifier
	int array;			// array size (0 if not an array)
	ExpVarGet get;
	int ValueType;
} ExpVars[] = {
	{ NAME_Alpha,		0, GetAlpha, VAL_Float },
	{ NAME_Angle,		0, GetAngle, VAL_Float },
	{ NAME_Args,		5, GetArgs, VAL_Int },
	{ NAME_CeilingZ,	0, GetCeilingZ, VAL_Float },
	{ NAME_FloorZ,		0, GetFloorZ, VAL_Float },
	{ NAME_Health,		0, GetHealth, VAL_Int },
	{ NAME_Pitch,		0, GetPitch, VAL_Float },
	{ NAME_Special,		0, GetSpecial, VAL_Int },
	{ NAME_TID,			0, GetTID, VAL_Int },
	{ NAME_TIDtoHate,	0, GetTIDToHate, VAL_Int },
	{ NAME_WaterLevel,	0, GetWaterLevel, VAL_Int },
	{ NAME_X,			0, GetX, VAL_Float },
	{ NAME_Y,			0, GetY, VAL_Float },
	{ NAME_Z,			0, GetZ, VAL_Float },
	{ NAME_MomX,		0, GetMomX, VAL_Float },
	{ NAME_MomY,		0, GetMomY, VAL_Float },
	{ NAME_MomZ,		0, GetMomZ, VAL_Float },
};


TDeletingArray<FxExpression *> StateExpressions;

//
// ParseExpression
// [GRB] Parses an expression and stores it into Expression array
//

static FxExpression *ParseExpressionM (FScanner &sc, const PClass *cls);
static FxExpression *ParseExpressionL (FScanner &sc, const PClass *cls);
static FxExpression *ParseExpressionK (FScanner &sc, const PClass *cls);
static FxExpression *ParseExpressionJ (FScanner &sc, const PClass *cls);
static FxExpression *ParseExpressionI (FScanner &sc, const PClass *cls);
static FxExpression *ParseExpressionH (FScanner &sc, const PClass *cls);
static FxExpression *ParseExpressionG (FScanner &sc, const PClass *cls);
static FxExpression *ParseExpressionF (FScanner &sc, const PClass *cls);
static FxExpression *ParseExpressionE (FScanner &sc, const PClass *cls);
static FxExpression *ParseExpressionD (FScanner &sc, const PClass *cls);
static FxExpression *ParseExpressionC (FScanner &sc, const PClass *cls);
static FxExpression *ParseExpressionB (FScanner &sc, const PClass *cls);
static FxExpression *ParseExpressionA (FScanner &sc, const PClass *cls);
static FxExpression *ParseExpression0 (FScanner &sc, const PClass *cls);

FxExpression *ParseExpression (FScanner &sc, PClass *cls)
{
	FxExpression *data = ParseExpressionM (sc, cls);

	FCompileContext ctx;
	ctx.cls = cls;
	data = data->Resolve(ctx);

	return data;
}


int ParseExpression (FScanner &sc, bool _not, PClass *cls)
{
	if (StateExpressions.Size()==0)
	{
		// StateExpressions[0] always is const 0;
		FxExpression *data = new FxConstant(0, FScriptPosition());
		StateExpressions.Push (data);
	}

	FxExpression *data = ParseExpression (sc, cls);
	return StateExpressions.Push (data);
}

static FxExpression *ParseExpressionM (FScanner &sc, const PClass *cls)
{
	FxExpression *condition = ParseExpressionL (sc, cls);

	if (sc.CheckToken('?'))
	{
		FxExpression *truex = ParseExpressionM (sc, cls);
		sc.MustGetToken(':');
		FxExpression *falsex = ParseExpressionM (sc, cls);
		return new FxConditional(condition, truex, falsex);
	}
	else
	{
		return condition;
	}
}

static FxExpression *ParseExpressionL (FScanner &sc, const PClass *cls)
{
	FxExpression *tmp = ParseExpressionK (sc, cls);

	while (sc.CheckToken(TK_OrOr))
	{
		FxExpression *right = ParseExpressionK (sc, cls);
		tmp = new FxBinaryLogical(TK_OrOr, tmp, right);
	}
	return tmp;
}

static FxExpression *ParseExpressionK (FScanner &sc, const PClass *cls)
{
	FxExpression *tmp = ParseExpressionJ (sc, cls);

	while (sc.CheckToken(TK_AndAnd))
	{
		FxExpression *right = ParseExpressionJ (sc, cls);
		tmp = new FxBinaryLogical(TK_AndAnd, tmp, right);
	}
	return tmp;
}

static FxExpression *ParseExpressionJ (FScanner &sc, const PClass *cls)
{
	FxExpression *tmp = ParseExpressionI (sc, cls);

	while (sc.CheckToken('|'))
	{
		FxExpression *right = ParseExpressionI (sc, cls);
		tmp = new FxBinaryInt('|', tmp, right);
	}
	return tmp;
}

static FxExpression *ParseExpressionI (FScanner &sc, const PClass *cls)
{
	FxExpression *tmp = ParseExpressionH (sc, cls);

	while (sc.CheckToken('^'))
	{
		FxExpression *right = ParseExpressionH (sc, cls);
		tmp = new FxBinaryInt('^', tmp, right);
	}
	return tmp;
}

static FxExpression *ParseExpressionH (FScanner &sc, const PClass *cls)
{
	FxExpression *tmp = ParseExpressionG (sc, cls);

	while (sc.CheckToken('&'))
	{
		FxExpression *right = ParseExpressionG (sc, cls);
		tmp = new FxBinaryInt('&', tmp, right);
	}
	return tmp;
}

static FxExpression *ParseExpressionG (FScanner &sc, const PClass *cls)
{
	FxExpression *tmp = ParseExpressionF (sc, cls);

	while (sc.GetToken() && (sc.TokenType == TK_Eq || sc.TokenType == TK_Neq))
	{
		int token = sc.TokenType;
		FxExpression *right = ParseExpressionF (sc, cls);
		tmp = new FxCompareEq(token, tmp, right);
	}
	if (!sc.End) sc.UnGet();
	return tmp;
}

static FxExpression *ParseExpressionF (FScanner &sc, const PClass *cls)
{
	FxExpression *tmp = ParseExpressionE (sc, cls);

	while (sc.GetToken() && (sc.TokenType == '<' || sc.TokenType == '>' || sc.TokenType == TK_Leq || sc.TokenType == TK_Geq))
	{
		int token = sc.TokenType;
		FxExpression *right = ParseExpressionE (sc, cls);
		tmp = new FxCompareRel(token, tmp, right);
	}
	if (!sc.End) sc.UnGet();
	return tmp;
}

static FxExpression *ParseExpressionE (FScanner &sc, const PClass *cls)
{
	FxExpression *tmp = ParseExpressionD (sc, cls);

	while (sc.GetToken() && (sc.TokenType == TK_LShift || sc.TokenType == TK_RShift || sc.TokenType == TK_URShift))
	{
		int token = sc.TokenType;
		FxExpression *right = ParseExpressionD (sc, cls);
		tmp = new FxBinaryInt(token, tmp, right);
	}
	if (!sc.End) sc.UnGet();
	return tmp;
}

static FxExpression *ParseExpressionD (FScanner &sc, const PClass *cls)
{
	FxExpression *tmp = ParseExpressionC (sc, cls);

	while (sc.GetToken() && (sc.TokenType == '+' || sc.TokenType == '-'))
	{
		int token = sc.TokenType;
		FxExpression *right = ParseExpressionC (sc, cls);
		tmp = new FxAddSub(token, tmp, right);
	}
	if (!sc.End) sc.UnGet();
	return tmp;
}

static FxExpression *ParseExpressionC (FScanner &sc, const PClass *cls)
{
	FxExpression *tmp = ParseExpressionB (sc, cls);

	while (sc.GetToken() && (sc.TokenType == '*' || sc.TokenType == '/' || sc.TokenType == '%'))
	{
		int token = sc.TokenType;
		FxExpression *right = ParseExpressionB (sc, cls);
		tmp = new FxMulDiv(token, tmp, right);
	}
	if (!sc.End) sc.UnGet();
	return tmp;
}

static FxExpression *ParseExpressionB (FScanner &sc, const PClass *cls)
{
	sc.GetToken();
	switch(sc.TokenType)
	{
	case '~':
		return new FxUnaryNotBitwise(ParseExpressionA (sc, cls));

	case '!':
		return new FxUnaryNotBoolean(ParseExpressionA (sc, cls));

	case '-':
		return new FxMinusSign(ParseExpressionA (sc, cls));

	case '+':
		return new FxPlusSign(ParseExpressionA (sc, cls));

	default:
		sc.UnGet();
		return ParseExpressionA (sc, cls);
	}
}

//==========================================================================
//
//	ParseExpressionB
//
//==========================================================================

static FxExpression *ParseExpressionA (FScanner &sc, const PClass *cls)
{
	FxExpression *base_expr = ParseExpression0 (sc, cls);

	while(1)
	{
		FScriptPosition pos(sc);

		if (sc.CheckToken('.'))
		{
			if (sc.CheckToken(TK_Default))
			{
				sc.MustGetToken('.');
				base_expr = new FxClassDefaults(base_expr, pos);
			}
			sc.MustGetToken(TK_Identifier);

			FName FieldName = sc.String;
			pos = sc;
			/* later!
			if (SC_CheckToken('('))
			{
				if (base_expr->IsDefaultObject())
				{
					SC_ScriptError("Cannot call methods for default.");
				}
				base_expr = ParseFunctionCall(base_expr, FieldName, false, false, pos);
			}
			else
			*/
			{
				base_expr = new FxDotIdentifier(base_expr, FieldName, pos);
			}
		}
		else if (sc.CheckToken('['))
		{
			FxExpression *index = ParseExpressionM(sc, cls);
			sc.MustGetToken(']');
			base_expr = new FxArrayElement(base_expr, index, pos);
		}
		else break;
	} 

	return base_expr;
}



static FxExpression *ParseExpression0 (FScanner &sc, const PClass *cls)
{
	FScriptPosition scpos(sc);
	if (sc.CheckToken('('))
	{
		FxExpression *data = ParseExpressionM (sc, cls);
		sc.MustGetToken(')');
		return data;
	}
	else if (sc.CheckToken(TK_True))
	{
		return new FxConstant(1, scpos);
	}
	else if (sc.CheckToken(TK_False))
	{
		return new FxConstant(0, scpos);
	}
	else if (sc.CheckToken(TK_IntConst))
	{
		return new FxConstant(sc.Number, scpos);
	}
	else if (sc.CheckToken(TK_FloatConst))
	{
		return new FxConstant(sc.Float, scpos);
	}
	else if (sc.CheckToken(TK_Identifier))
	{
		FName identifier = FName(sc.String);
		switch (identifier)
		{
		case NAME_Random:
		{
			FRandom *rng;

			if (sc.CheckToken('['))
			{
				sc.MustGetToken(TK_Identifier);
				rng = FRandom::StaticFindRNG(sc.String);
				sc.MustGetToken(']');
			}
			else
			{
				rng = &pr_exrandom;
			}
			sc.MustGetToken('(');

			FxExpression *min = ParseExpressionM (sc, cls);
			sc.MustGetToken(',');
			FxExpression *max = ParseExpressionM (sc, cls);
			sc.MustGetToken(')');

			return new FxRandom(rng, min, max, sc);
		}
		break;

		case NAME_Random2:
		{
			FRandom *rng;

			if (sc.CheckToken('['))
			{
				sc.MustGetToken(TK_Identifier);
				rng = FRandom::StaticFindRNG(sc.String);
				sc.MustGetToken(']');
			}
			else
			{
				rng = &pr_exrandom;
			}

			sc.MustGetToken('(');

			FxExpression *mask = NULL;

			if (!sc.CheckToken(')'))
			{
				mask = ParseExpressionM(sc, cls);
				sc.MustGetToken(')');
			}
			return new FxRandom2(rng, mask, sc);
		}
		break;

		case NAME_Abs:
		{
			sc.MustGetToken('(');
			FxExpression *x = ParseExpressionM (sc, cls);
			sc.MustGetToken(')');
			return new FxAbs(x); 
		}

		default:
			if (sc.CheckToken('('))
			{
				if (identifier == NAME_Sin)
				{
					sc.MustGetToken('(');

					FxExpression *data = new FxExpression;
					data->Type = EX_Sin;
					data->ValueType = VAL_Float;

					data->Children[0] = ParseExpressionM (sc, cls);

					sc.MustGetToken(')');
					return data;
				}
				else if (identifier == NAME_Cos)
				{
					sc.MustGetToken('(');

					FxExpression *data = new FxExpression;
					data->Type = EX_Cos;
					data->ValueType = VAL_Float;

					data->Children[0] = ParseExpressionM (sc, cls);

					sc.MustGetToken(')');
					return data;
				}
				else
				{
					int specnum, min_args, max_args;

					// Check if this is an action special
					specnum = P_FindLineSpecial (sc.String, &min_args, &max_args);
					if (specnum != 0 && min_args >= 0)
					{
						int i;

						sc.MustGetToken('(');

						FxExpression *data = new FxExpression, **left;
						data->Type = EX_ActionSpecial;
						data->Value.Int = specnum;
						data->ValueType = VAL_Int;

						data->Children[0] = ParseExpressionM (sc, cls);
						left = &data->Children[1];

						for (i = 1; i < 5 && sc.CheckToken(','); ++i)
						{
							FxExpression *right = new FxExpression;
							right->Type = EX_Right;
							right->Children[0] = ParseExpressionM (sc, cls);
							*left = right;
							left = &right->Children[1];
						}
						*left = NULL;
						sc.MustGetToken(')');
						if (i < min_args)
							sc.ScriptError ("Not enough arguments to action special");
						if (i > max_args)
							sc.ScriptError ("Too many arguments to action special");

						return data;
					}
					else
					{
						sc.ScriptError("Unknown function '%s'", identifier.GetChars());
					}
				}

			}	
			else
			{
				return new FxIdentifier(identifier, sc);
			}

			/*
			// Check if this is a constant
			if (cls != NULL)
			{
				PSymbol *sym = cls->Symbols.FindSymbol (identifier, true);
				if (sym == NULL) sym = GlobalSymbols.FindSymbol (identifier, true);
				if (sym != NULL && sym->SymbolType == SYM_Const)
				{
					return new FxConstant(static_cast<PSymbolConst *>(sym)->Value, sc);
				}
			}

			// Check if it's a variable we understand
			int varid = -1;
			FName vname = sc.String;
			for (size_t i = 0; i < countof(ExpVars); i++)
			{
				if (vname == ExpVars[i].name)
				{
					varid = (int)i;
					break;
				}
			}

			if (varid == -1)
				sc.ScriptError ("Unknown value '%s'", sc.String);

			FxExpression *data = new FxExpression;
			data->Type = EX_Var;
			data->Value.Type = VAL_Int;
			data->Value.Int = varid;
			data->ValueType = ExpVars[varid].ValueType;

			if (ExpVars[varid].array)
			{
				sc.MustGetToken('[');
				data->Children[0] = ParseExpressionM (sc, cls);
				sc.MustGetToken(']');
			}
			return data;
			*/
		}
	}
	else
	{
		FString tokname = sc.TokenName(sc.TokenType, sc.String);
		sc.ScriptError ("Unexpected token %s", tokname.GetChars());
	}
	return NULL;
}

//
// EvalExpression
// [GRB] Evaluates previously stored expression
//

bool IsExpressionConst(int id)
{
	if (StateExpressions.Size() <= (unsigned int)id) return false;

	return StateExpressions[id]->isConstant();
}

int EvalExpressionI (int id, AActor *self, const PClass *cls)
{
	if (StateExpressions.Size() <= (unsigned int)id) return 0;

	if (cls == NULL && self != NULL)
	{
		cls = self->GetClass();
	}

	ExpVal val = StateExpressions[id]->EvalExpression (self, cls);

	switch (val.Type)
	{
	default:
	case VAL_Int:
		return val.Int;
	case VAL_Float:
		return (int)val.Float;
	}
}

double EvalExpressionF (int id, AActor *self, const PClass *cls)
{
	if (StateExpressions.Size() <= (unsigned int)id) return 0.f;

	if (cls == NULL && self != NULL)
	{
		cls = self->GetClass();
	}

	ExpVal val = StateExpressions[id]->EvalExpression (self, cls);

	switch (val.Type)
	{
	default:
	case VAL_Int:
		return (double)val.Int;
	case VAL_Float:
		return val.Float;
	}
}

fixed_t EvalExpressionFix (int id, AActor *self, const PClass *cls)
{
	if (StateExpressions.Size() <= (unsigned int)id) return 0;

	if (cls == NULL && self != NULL)
	{
		cls = self->GetClass();
	}

	ExpVal val = StateExpressions[id]->EvalExpression (self, cls);

	switch (val.Type)
	{
	default:
	case VAL_Int:
		return val.Int << FRACBITS;
	case VAL_Float:
		return fixed_t(val.Float*FRACUNIT);
	}
}

ExpVal FxExpression::EvalExpression (AActor *self, const PClass *cls)
{
	ExpVal val;

	val.Type = VAL_Int;		// Placate GCC

	switch (Type)
	{
	case EX_NOP:
		assert (Type != EX_NOP);
		val = Value;
		break;
	case EX_Var:
		if (!self)
		{
			I_FatalError ("Missing actor data");
		}
		else
		{
			int id = 0;
			if (ExpVars[Value.Int].array)
			{
				ExpVal idval = Children[0]->EvalExpression (self, cls);
				id = ((idval.Type == VAL_Int) ? idval.Int : (int)idval.Float) % ExpVars[Value.Int].array;
			}

			val = ExpVars[Value.Int].get (self, id);
		}
		break;
	case EX_Sin:
		{
			ExpVal a = Children[0]->EvalExpression (self, cls);
			angle_t angle = (a.Type == VAL_Int) ? (a.Int * ANGLE_1) : angle_t(a.Float * ANGLE_1);

			val.Type = VAL_Float;
			val.Float = FIXED2FLOAT (finesine[angle>>ANGLETOFINESHIFT]);
		}
		break;

	case EX_Cos:
		{
			ExpVal a = Children[0]->EvalExpression (self, cls);
			angle_t angle = (a.Type == VAL_Int) ? (a.Int * ANGLE_1) : angle_t(a.Float * ANGLE_1);

			val.Type = VAL_Float;
			val.Float = FIXED2FLOAT (finecosine[angle>>ANGLETOFINESHIFT]);
		}
		break;

	case EX_ActionSpecial:
		{
			int parms[5] = { 0, 0, 0, 0 };
			int i = 0;
			FxExpression *parm = this;
			
			while (parm != NULL && i < 5)
			{
				ExpVal val = parm->Children[0]->EvalExpression (self, cls);
				if (val.Type == VAL_Int)
				{
					parms[i] = val.Int;
				}
				else
				{
					parms[i] = (int)val.Float;
				}
				i++;
				parm = parm->Children[1];
			}

			val.Type = VAL_Int;
			val.Int = LineSpecials[Value.Int] (NULL, self, false,
				parms[0], parms[1], parms[2], parms[3], parms[4]);
		}
		break;

	case EX_Right:
		// This should never be a top-level expression.
		assert (Type != EX_Right);
		break;
	}

	return val;
}


bool FxExpression::isConstant() const
{
	return false;
}

FxExpression *FxExpression::Resolve(FCompileContext &ctx)
{
	if (Children[0]) Children[0] = Children[0]->Resolve(ctx);
	if (Children[1]) Children[1] = Children[1]->Resolve(ctx);
	return this;
}



/*
some stuff for later
static FxExpression *ParseExpressionA (FScanner &sc, const PClass *cls)
{
	else if (sc.CheckToken(TK_Identifier))
	{
		FName IdName = FName(sc.String);
		switch (IdName)
		{
		default:
		{
			FScriptPosition scriptpos(sc);
			if (sc.CheckToken('('))
			{
				// function call
				TArray<FxExpression *> arguments;

				do
				{
					FxExpression *data = ParseExpressionM(sc, cls);
					arguments.Push(data);
				}
				while (sc.CheckToken(','));
				return new FxFunctionCall(arguments, scriptpos);
			}
			else
			{
				FxExpression *data = new FxIdentifier(IdName, scriptpos);
				if (sc.CheckToken('['))
				{
					FxExpression *index = ParseExpressionM(sc, cls);
					sc.MustGetToken(']');
					data = new FxArrayElement(data, index);
				}
				return data;
			}
		}
		break;
		}
	}
	else
	{
		FString tokname = sc.TokenName(sc.TokenType, sc.String);
		sc.ScriptError ("Unexpected token %s", tokname.GetChars());
		return NULL;
	}
}

*/
