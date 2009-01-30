/*
** g_level.cpp
** Parses MAPINFO
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
** Copyright 2009 Christoph Oelckers
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

#include <assert.h>
#include "templates.h"
#include "g_level.h"
#include "sc_man.h"
#include "w_wad.h"
#include "m_menu.h"
#include "cmdlib.h"
#include "v_video.h"
#include "p_lnspec.h"
#include "p_setup.h"
#include "i_system.h"
#include "gi.h"

int FindEndSequence (int type, const char *picname);


int FindWadLevelInfo (const char *name)
{
	for (unsigned int i = 0; i < wadlevelinfos.Size(); i++)
		if (!strnicmp (name, wadlevelinfos[i].mapname, 8))
			return i;
		
	return -1;
}

int FindWadClusterInfo (int cluster)
{
	for (unsigned int i = 0; i < wadclusterinfos.Size(); i++)
		if (wadclusterinfos[i].cluster == cluster)
			return i;
		
	return -1;
}

//==========================================================================
//
//
//==========================================================================

void level_info_t::Reset()
{
	mapname[0] = 0;
	levelnum = 0;
	pname[0] = 0;
	nextmap[0] = 0;
	secretmap[0] = 0;
	strcpy (skypic1, "-NOFLAT-");
	strcpy (skypic2, "-NOFLAT-");
	cluster = 0;
	partime = 0;
	sucktime = 0;
	flags = 0;
	flags2 = gameinfo.gametype == GAME_Hexen? 0 : LEVEL2_LAXMONSTERACTIVATION;
	Music = "";
	LevelName = "";
	strcpy (fadetable, "COLORMAP");
	WallHorizLight = -8;
	WallVertLight = +8;
	f1[0] = 0;
	musicorder = 0;
	snapshot = NULL;
	snapshotVer = 0;
	defered = 0;
	skyspeed1 = skyspeed2 = 0.f;
	fadeto = 0;
	outsidefog = 0xff000000;
	cdtrack = 0;
	cdid = 0;
	gravity = 0.f;
	aircontrol = 0.f;
	WarpTrans = 0;
	airsupply = 20;
	compatflags = 0;
	compatmask = 0;
	Translator = "";
	RedirectType = 0;
	RedirectMap[0] = 0;
	EnterPic = "";
	ExitPic = "";
	InterMusic = "";
	intermusicorder = 0;
	SoundInfo = "";
	SndSeq = "";
	strcpy (bordertexture, gameinfo.borderFlat);
	teamdamage = 0.f;
	specialactions.Clear();
}


//==========================================================================
//
//
//==========================================================================

void cluster_info_t::Reset()
{
	cluster = 0;
	finaleflat[0] = 0;
	ExitText = "";
	EnterText = "";
	MessageMusic = "";
	musicorder = 0;
	flags = 0;
	cdtrack = 0;
	ClusterName = "";
	cdid = 0;
}


//==========================================================================
//
// ParseCluster
// Parses a cluster definition
//
//==========================================================================

void FMapInfoParser::ParseCluster()
{
	sc.MustGetNumber ();
	int clusterindex = FindWadClusterInfo (sc.Number);
	if (clusterindex == -1)
	{
		clusterindex = wadclusterinfos.Reserve(1);
	}

	cluster_info_t *clusterinfo = &wadclusterinfos[clusterindex];
	clusterinfo->Reset();
	clusterinfo->cluster = sc.Number;

	ParseOpenBrace();

	while (sc.GetString())
	{
		if (sc.Compare("name"))
		{
			ParseOpenParen();
			sc.MustGetString();
			clusterinfo->ClusterName = sc.String;
			ParseCloseParen();
		}
		else if (sc.Compare("entertext"))
		{
			ParseOpenParen();
			sc.MustGetString();
			clusterinfo->EnterText = sc.String;
			ParseCloseParen();
		}
		else if (sc.Compare("exittext"))
		{
			ParseOpenParen();
			sc.MustGetString();
			clusterinfo->ExitText = sc.String;
			ParseCloseParen();
		}
		else if (sc.Compare("music"))
		{
			int order = 0;

			ParseOpenParen();
			sc.MustGetString();

			char *colon = strchr (sc.String, ':');
			if (colon)
			{
				order = atoi(colon+1);
				*colon = 0;
			}
			clusterinfo->MessageMusic = sc.String;
			if (!colon && CheckNumber())
			{
				order = sc.Number;
			}
			clusterinfo->musicorder = order;
			ParseCloseParen();
		}
		else if (sc.Compare("flat"))
		{
			ParseOpenParen();
			sc.MustGetString();
			uppercopy(clusterinfo->finaleflat, sc.String);
			ParseCloseParen();
		}
		else if (sc.Compare("pic"))
		{
			ParseOpenParen();
			sc.MustGetString();
			uppercopy(clusterinfo->finaleflat, sc.String);
			clusterinfo->flags |= CLUSTER_FINALEPIC;
			ParseCloseParen();
		}
		else if (sc.Compare("hub"))
		{
			clusterinfo->flags |= CLUSTER_HUB;
		}
		else if (sc.Compare("cdtrack"))
		{
			ParseOpenParen();
			sc.MustGetNumber();
			clusterinfo->cdtrack = sc.Number;
			ParseCloseParen();
		}
		else if (sc.Compare("cdid"))
		{
			ParseOpenParen();
			sc.MustGetString();
			clusterinfo->cdid = strtoul (sc.String, NULL, 16);
			ParseCloseParen();
		}
		else if (sc.Compare("entertextislump"))
		{
			clusterinfo->flags |= CLUSTER_LOOKUPENTERTEXT;
		}
		else if (sc.Compare("exittextislump"))
		{
			clusterinfo->flags |= CLUSTER_LOOKUPEXITTEXT;
		}
		else if (!ParseCloseBrace())
		{
			// Unknown
			sc.ScriptMessage("Unknown property '%s' found in map definition\n", sc.String);
			SkipToNext();
		}
		else
		{
			break;
		}
	}
}


//==========================================================================
//
// ParseNextMap
// Parses a next map field
//
//==========================================================================

void FMapInfoParser::ParseNextMap(char *mapname)
{
	EndSequence newSeq;
	bool useseq = false;

	if (sc.CheckNumber())
	{
		if (HexenHack)
		{
			mysnprintf (mapname, 9, "&wt@%02d", sc.Number);
		}
		else
		{
			mysnprintf (mapname, 9, "MAP%02d", sc.Number);
		}
	}
	else
	{
		sc.MustGetString();

		if (sc.Compare("endgame"))
		{
			newSeq.Advanced = true;
			newSeq.EndType = END_Pic1;
			newSeq.PlayTheEnd = false;
			newSeq.MusicLooping = true;
			sc.MustGetStringName("{");
			while (!sc.CheckString("}"))
			{
				sc.MustGetString();
				if (sc.Compare("pic"))
				{
					ParseOpenParen();
					sc.MustGetString();
					newSeq.EndType = END_Pic;
					newSeq.PicName = sc.String;
					ParseCloseParen();
				}
				else if (sc.Compare("hscroll"))
				{
					ParseOpenParen();
					newSeq.EndType = END_Bunny;
					sc.MustGetString();
					newSeq.PicName = sc.String;
					ParseComma();
					sc.MustGetString();
					newSeq.PicName2 = sc.String;
					if (CheckNumber())
						newSeq.PlayTheEnd = !!sc.Number;
					ParseCloseParen();
				}
				else if (sc.Compare("vscroll"))
				{
					ParseOpenParen();
					newSeq.EndType = END_Demon;
					sc.MustGetString();
					newSeq.PicName = sc.String;
					ParseComma();
					sc.MustGetString();
					newSeq.PicName2 = sc.String;
					ParseCloseParen();
				}
				else if (sc.Compare("cast"))
				{
					newSeq.EndType = END_Cast;
				}
				else if (sc.Compare("music"))
				{
					ParseOpenParen();
					sc.MustGetString();
					newSeq.Music = sc.String;
					if (CheckNumber())
					{
						newSeq.MusicLooping = !!sc.Number;
					}
					ParseCloseParen();
				}
			}
			useseq = true;
		}
		else if (strnicmp (sc.String, "EndGame", 7) == 0)
		{
			int type;
			switch (sc.String[7])
			{
			case '1':	type = END_Pic1;		break;
			case '2':	type = END_Pic2;		break;
			case '3':	type = END_Bunny;		break;
			case 'C':	type = END_Cast;		break;
			case 'W':	type = END_Underwater;	break;
			case 'S':	type = END_Strife;		break;
			default:	type = END_Pic3;		break;
			}
			newSeq.EndType = type;
			useseq = true;
		}
		else if (sc.Compare("endpic"))
		{
			ParseComma();
			sc.MustGetString ();
			newSeq.EndType = END_Pic;
			newSeq.PicName = sc.String;
			useseq = true;
		}
		else if (sc.Compare("endbunny"))
		{
			newSeq.EndType = END_Bunny;
			useseq = true;
		}
		else if (sc.Compare("endcast"))
		{
			newSeq.EndType = END_Cast;
			useseq = true;
		}
		else if (sc.Compare("enddemon"))
		{
			newSeq.EndType = END_Demon;
			useseq = true;
		}
		else if (sc.Compare("endchess"))
		{
			newSeq.EndType = END_Chess;
			useseq = true;
		}
		else if (sc.Compare("endunderwater"))
		{
			newSeq.EndType = END_Underwater;
			useseq = true;
		}
		else if (sc.Compare("endbuystrife"))
		{
			newSeq.EndType = END_BuyStrife;
			useseq = true;
		}
		else
		{
			strncpy (mapname, sc.String, 8);
		}
		if (useseq)
		{
			int seqnum = -1;

			if (!newSeq.Advanced)
			{
				seqnum = FindEndSequence (newSeq.EndType, newSeq.PicName);
			}

			if (seqnum == -1)
			{
				seqnum = (int)EndSequences.Push (newSeq);
			}
			strcpy (mapname, "enDSeQ");
			*((WORD *)(mapname + 6)) = (WORD)seqnum;
		}
	}
}

//==========================================================================
//
//
//==========================================================================

void FMapInfoParser::ParseLumpOrTextureName(char *name)
{
	sc.MustGetString();
	uppercopy(name, sc.String);
}

//==========================================================================
//
// Map options
//
//==========================================================================

DEFINE_MAP_OPTION(levelnum, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetNumber();
	info->levelnum = parse.sc.Number;
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(next, true)
{
	parse.ParseOpenParen();
	parse.ParseNextMap(info->nextmap);
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(secretnext, true)
{
	parse.ParseOpenParen();
	parse.ParseNextMap(info->secretmap);
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(cluster, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetNumber();
	info->cluster = parse.sc.Number;

	// If this cluster hasn't been defined yet, add it. This is especially needed
	// for Hexen, because it doesn't have clusterdefs. If we don't do this, every
	// level on Hexen will sometimes be considered as being on the same hub,
	// depending on the check done.
	if (FindWadClusterInfo (parse.sc.Number) == -1)
	{
		unsigned int clusterindex = wadclusterinfos.Reserve(1);
		cluster_info_t *clusterinfo = &wadclusterinfos[clusterindex];
		memset (clusterinfo, 0, sizeof(cluster_info_t));
		clusterinfo->cluster = parse.sc.Number;
		if (parse.HexenHack)
		{
			clusterinfo->flags |= CLUSTER_HUB;
		}
	}

	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(sky1, true)
{
	parse.ParseOpenParen();
	parse.ParseLumpOrTextureName(info->skypic1);
	if (parse.CheckFloat())
	{
		if (parse.HexenHack)
		{
			parse.sc.Float /= 256;
		}
		info->skyspeed1 = parse.sc.Float;
	}
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(sky2, true)
{
	parse.ParseOpenParen();
	parse.ParseLumpOrTextureName(info->skypic2);
	if (parse.CheckFloat())
	{
		if (parse.HexenHack)
		{
			parse.sc.Float /= 256;
		}
		info->skyspeed2 = parse.sc.Float;
	}
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(fade, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetString();
	info->fadeto = V_GetColor(NULL, parse.sc.String);
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(outsidefog, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetString();
	info->outsidefog = V_GetColor(NULL, parse.sc.String);
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(titlepatch, true)
{
	parse.ParseOpenParen();
	parse.ParseLumpOrTextureName(info->pname);
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(partime, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetNumber();
	info->partime = parse.sc.Number;
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(sucktime, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetNumber();
	info->sucktime = parse.sc.Number;
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(music, true)
{
	int order = 0;
	parse.ParseOpenParen();
	parse.sc.MustGetString ();
	char *colon = strchr (parse.sc.String, ':');
	if (colon)
	{
		order = atoi(colon+1);
		*colon = 0;
	}
	info->Music = parse.sc.String;
	if (!colon && parse.CheckNumber())
	{
		order = parse.sc.Number;
	}
	info->musicorder = order;
	// Flag the level so that the $MAP command doesn't override this.
	info->flags2 |= LEVEL2_MUSICDEFINED;
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(intermusic, true)
{
	int order = 0;
	parse.ParseOpenParen();
	parse.sc.MustGetString ();
	char *colon = strchr (parse.sc.String, ':');
	if (colon)
	{
		order = atoi(colon+1);
		*colon = 0;
	}
	info->InterMusic = parse.sc.String;
	if (!colon && parse.CheckNumber())
	{
		order = parse.sc.Number;
	}
	info->intermusicorder = order;
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(fadetable, true)
{
	parse.ParseOpenParen();
	parse.ParseLumpOrTextureName(info->fadetable);
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(evenlighting, true)
{
	info->WallVertLight = info->WallHorizLight = 0;
}

DEFINE_MAP_OPTION(cdtrack, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetNumber();
	info->cdtrack = parse.sc.Number;
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(cdid, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetString();
	info->cdid = strtoul (parse.sc.String, NULL, 16);
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(warptrans, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetNumber();
	info->WarpTrans = parse.sc.Number;
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(vertwallshade, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetNumber();
	info->WallVertLight = (SBYTE)clamp (parse.sc.Number / 2, -128, 127);
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(horizwallshade, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetNumber();
	info->WallHorizLight = (SBYTE)clamp (parse.sc.Number / 2, -128, 127);
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(gravity, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetFloat();
	info->gravity = parse.sc.Float;
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(aircontrol, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetFloat();
	info->aircontrol = parse.sc.Float;
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(airsupply, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetNumber();
	info->airsupply = parse.sc.Number;
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(interpic, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetString();
	info->ExitPic = parse.sc.String;
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(exitpic, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetString();
	info->ExitPic = parse.sc.String;
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(enterpic, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetString();
	info->EnterPic = parse.sc.String;
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(specialaction, true)
{
	parse.ParseOpenParen();

	FSpecialAction *sa = &info->specialactions[info->specialactions.Reserve(1)];
	int min_arg, max_arg;
	if (parse.format_type == parse.FMT_Old) parse.sc.SetCMode(true);
	parse.sc.MustGetString();
	sa->Type = FName(parse.sc.String);
	parse.sc.CheckString(",");
	parse.sc.MustGetString();
	sa->Action = P_FindLineSpecial(parse.sc.String, &min_arg, &max_arg);
	if (sa->Action == 0 || min_arg < 0)
	{
		parse.sc.ScriptError("Unknown specialaction '%s'");
	}
	int j = 0;
	while (j < 5 && parse.sc.CheckString(","))
	{
		parse.sc.MustGetNumber();
		sa->Args[j++] = parse.sc.Number;
	}
	if (parse.format_type == parse.FMT_Old) parse.sc.SetCMode(false);
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(redirect, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetString();
	info->RedirectType = parse.sc.String;
	parse.ParseComma();
	parse.ParseLumpOrTextureName(info->RedirectMap);
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(sndseq, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetString();
	info->SndSeq = parse.sc.String;
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(sndinfo, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetString();
	info->SoundInfo = parse.sc.String;
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(soundinfo, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetString();
	info->SoundInfo = parse.sc.String;
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(translator, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetString();
	info->Translator = parse.sc.String;
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(bordertexture, true)
{
	parse.ParseOpenParen();
	parse.ParseLumpOrTextureName(info->bordertexture);
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(f1, true)
{
	parse.ParseOpenParen();
	parse.ParseLumpOrTextureName(info->f1);
	parse.ParseCloseParen();
}

DEFINE_MAP_OPTION(teamdamage, true)
{
	parse.ParseOpenParen();
	parse.sc.MustGetFloat();
	info->teamdamage = parse.sc.Float;
	parse.ParseCloseParen();
}

//==========================================================================
//
// All flag based map options 
//
//==========================================================================

enum EMIType
{
	MITYPE_IGNORE,
	MITYPE_EATNEXT,
	MITYPE_SETFLAG,
	MITYPE_CLRFLAG,
	MITYPE_SCFLAGS,
	MITYPE_SETFLAG2,
	MITYPE_CLRFLAG2,
	MITYPE_SCFLAGS2,
	MITYPE_COMPATFLAG,
};

struct MapInfoFlagHandler
{
	const char *name;
	EMIType type;
	DWORD data1, data2;
}
MapFlagHandlers[] =
{
	{ "nointermission",					MITYPE_SETFLAG,	LEVEL_NOINTERMISSION, 0 },
	{ "intermission",					MITYPE_CLRFLAG,	LEVEL_NOINTERMISSION, 0 },
	{ "doublesky",						MITYPE_SETFLAG,	LEVEL_DOUBLESKY, 0 },
	{ "nosoundclipping",				MITYPE_IGNORE,	0, 0 },	// was nosoundclipping
	{ "allowmonstertelefrags",			MITYPE_SETFLAG,	LEVEL_MONSTERSTELEFRAG, 0 },
	{ "map07special",					MITYPE_SETFLAG,	LEVEL_MAP07SPECIAL, 0 },
	{ "baronspecial",					MITYPE_SETFLAG,	LEVEL_BRUISERSPECIAL, 0 },
	{ "cyberdemonspecial",				MITYPE_SETFLAG,	LEVEL_CYBORGSPECIAL, 0 },
	{ "spidermastermindspecial",		MITYPE_SETFLAG,	LEVEL_SPIDERSPECIAL, 0 },
	{ "minotaurspecial",				MITYPE_SETFLAG,	LEVEL_MINOTAURSPECIAL, 0 },
	{ "dsparilspecial",					MITYPE_SETFLAG,	LEVEL_SORCERER2SPECIAL, 0 },
	{ "ironlichspecial",				MITYPE_SETFLAG,	LEVEL_HEADSPECIAL, 0 },
	{ "specialaction_exitlevel",		MITYPE_SCFLAGS,	0, ~LEVEL_SPECACTIONSMASK },
	{ "specialaction_opendoor",			MITYPE_SCFLAGS,	LEVEL_SPECOPENDOOR, ~LEVEL_SPECACTIONSMASK },
	{ "specialaction_lowerfloor",		MITYPE_SCFLAGS,	LEVEL_SPECLOWERFLOOR, ~LEVEL_SPECACTIONSMASK },
	{ "specialaction_killmonsters",		MITYPE_SETFLAG,	LEVEL_SPECKILLMONSTERS, 0 },
	{ "lightning",						MITYPE_SETFLAG,	LEVEL_STARTLIGHTNING, 0 },
	{ "smoothlighting",					MITYPE_SETFLAG2,	LEVEL2_SMOOTHLIGHTING, 0 },
	{ "noautosequences",				MITYPE_SETFLAG,	LEVEL_SNDSEQTOTALCTRL, 0 },
	{ "autosequences",					MITYPE_CLRFLAG,	LEVEL_SNDSEQTOTALCTRL, 0 },
	{ "forcenoskystretch",				MITYPE_SETFLAG,	LEVEL_FORCENOSKYSTRETCH, 0 },
	{ "skystretch",						MITYPE_CLRFLAG,	LEVEL_FORCENOSKYSTRETCH, 0 },
	{ "allowfreelook",					MITYPE_SCFLAGS,	LEVEL_FREELOOK_YES, ~LEVEL_FREELOOK_NO },
	{ "nofreelook",						MITYPE_SCFLAGS,	LEVEL_FREELOOK_NO, ~LEVEL_FREELOOK_YES },
	{ "allowjump",						MITYPE_CLRFLAG,	LEVEL_JUMP_NO, 0 },
	{ "nojump",							MITYPE_SETFLAG,	LEVEL_JUMP_NO, 0 },
	{ "fallingdamage",					MITYPE_SCFLAGS,	LEVEL_FALLDMG_HX, ~LEVEL_FALLDMG_ZD },
	{ "oldfallingdamage",				MITYPE_SCFLAGS,	LEVEL_FALLDMG_ZD, ~LEVEL_FALLDMG_HX },
	{ "forcefallingdamage",				MITYPE_SCFLAGS,	LEVEL_FALLDMG_ZD, ~LEVEL_FALLDMG_HX },
	{ "strifefallingdamage",			MITYPE_SETFLAG,	LEVEL_FALLDMG_ZD|LEVEL_FALLDMG_HX, 0 },
	{ "nofallingdamage",				MITYPE_SCFLAGS,	0, ~(LEVEL_FALLDMG_ZD|LEVEL_FALLDMG_HX) },
	{ "noallies",						MITYPE_SETFLAG,	LEVEL_NOALLIES, 0 },
	{ "filterstarts",					MITYPE_SETFLAG,	LEVEL_FILTERSTARTS, 0 },
	{ "activateowndeathspecials",		MITYPE_SETFLAG,	LEVEL_ACTOWNSPECIAL, 0 },
	{ "killeractivatesdeathspecials",	MITYPE_CLRFLAG,	LEVEL_ACTOWNSPECIAL, 0 },
	{ "missilesactivateimpactlines",	MITYPE_SETFLAG2,	LEVEL2_MISSILESACTIVATEIMPACT, 0 },
	{ "missileshootersactivetimpactlines",MITYPE_CLRFLAG2,	LEVEL2_MISSILESACTIVATEIMPACT, 0 },
	{ "noinventorybar",					MITYPE_SETFLAG,	LEVEL_NOINVENTORYBAR, 0 },
	{ "deathslideshow",					MITYPE_SETFLAG2,	LEVEL2_DEATHSLIDESHOW, 0 },
	{ "strictmonsteractivation",		MITYPE_CLRFLAG2,	LEVEL2_LAXMONSTERACTIVATION, LEVEL2_LAXACTIVATIONMAPINFO },
	{ "laxmonsteractivation",			MITYPE_SETFLAG2,	LEVEL2_LAXMONSTERACTIVATION, LEVEL2_LAXACTIVATIONMAPINFO },
	{ "additive_scrollers",				MITYPE_COMPATFLAG, COMPATF_BOOMSCROLL},
	{ "keepfullinventory",				MITYPE_SETFLAG2,	LEVEL2_KEEPFULLINVENTORY, 0 },
	{ "monsterfallingdamage",			MITYPE_SETFLAG2,	LEVEL2_MONSTERFALLINGDAMAGE, 0 },
	{ "nomonsterfallingdamage",			MITYPE_CLRFLAG2,	LEVEL2_MONSTERFALLINGDAMAGE, 0 },
	{ "clipmidtextures",				MITYPE_SETFLAG2,	LEVEL2_CLIPMIDTEX, 0 },
	{ "wrapmidtextures",				MITYPE_SETFLAG2,	LEVEL2_WRAPMIDTEX, 0 },
	{ "allowcrouch",					MITYPE_CLRFLAG,	LEVEL_CROUCH_NO, 0 },
	{ "nocrouch",						MITYPE_SETFLAG,	LEVEL_CROUCH_NO, 0 },
	{ "pausemusicinmenus",				MITYPE_SCFLAGS2,	LEVEL2_PAUSE_MUSIC_IN_MENUS, 0 },
	{ "noinfighting",					MITYPE_SCFLAGS2,	LEVEL2_NOINFIGHTING, ~LEVEL2_TOTALINFIGHTING },
	{ "normalinfighting",				MITYPE_SCFLAGS2,	0, ~(LEVEL2_NOINFIGHTING|LEVEL2_TOTALINFIGHTING)},
	{ "totalinfighting",				MITYPE_SCFLAGS2,	LEVEL2_TOTALINFIGHTING, ~LEVEL2_NOINFIGHTING },
	{ "infiniteflightpowerup",			MITYPE_SETFLAG2,	LEVEL2_INFINITE_FLIGHT, 0 },
	{ "noinfiniteflightpowerup",		MITYPE_CLRFLAG2,	LEVEL2_INFINITE_FLIGHT, 0 },
	{ "allowrespawn",					MITYPE_SETFLAG2,	LEVEL2_ALLOWRESPAWN, 0 },
	{ "teamplayon",						MITYPE_SCFLAGS2,	LEVEL2_FORCETEAMPLAYON, ~LEVEL2_FORCETEAMPLAYOFF },
	{ "teamplayoff",					MITYPE_SCFLAGS2,	LEVEL2_FORCETEAMPLAYOFF, ~LEVEL2_FORCETEAMPLAYON },
	{ "checkswitchrange",				MITYPE_SETFLAG2,	LEVEL2_CHECKSWITCHRANGE, 0 },
	{ "nocheckswitchrange",				MITYPE_CLRFLAG2,	LEVEL2_CHECKSWITCHRANGE, 0 },
	{ "unfreezesingleplayerconversations",MITYPE_SETFLAG2,	LEVEL2_CONV_SINGLE_UNFREEZE, 0 },
	{ "nobotnodes",						MITYPE_IGNORE,	0, 0 },		// Skulltag option: nobotnodes
	{ "compat_shorttex",				MITYPE_COMPATFLAG, COMPATF_SHORTTEX},
	{ "compat_stairs",					MITYPE_COMPATFLAG, COMPATF_STAIRINDEX},
	{ "compat_limitpain",				MITYPE_COMPATFLAG, COMPATF_LIMITPAIN},
	{ "compat_nopassover",				MITYPE_COMPATFLAG, COMPATF_NO_PASSMOBJ},
	{ "compat_notossdrops",				MITYPE_COMPATFLAG, COMPATF_NOTOSSDROPS},
	{ "compat_useblocking", 			MITYPE_COMPATFLAG, COMPATF_USEBLOCKING},
	{ "compat_nodoorlight",				MITYPE_COMPATFLAG, COMPATF_NODOORLIGHT},
	{ "compat_ravenscroll",				MITYPE_COMPATFLAG, COMPATF_RAVENSCROLL},
	{ "compat_soundtarget",				MITYPE_COMPATFLAG, COMPATF_SOUNDTARGET},
	{ "compat_dehhealth",				MITYPE_COMPATFLAG, COMPATF_DEHHEALTH},
	{ "compat_trace",					MITYPE_COMPATFLAG, COMPATF_TRACE},
	{ "compat_dropoff",					MITYPE_COMPATFLAG, COMPATF_DROPOFF},
	{ "compat_boomscroll",				MITYPE_COMPATFLAG, COMPATF_BOOMSCROLL},
	{ "compat_invisibility",			MITYPE_COMPATFLAG, COMPATF_INVISIBILITY},
	{ "compat_silent_instant_floors",	MITYPE_COMPATFLAG, COMPATF_SILENT_INSTANT_FLOORS},
	{ "compat_sectorsounds",			MITYPE_COMPATFLAG, COMPATF_SECTORSOUNDS},
	{ "compat_missileclip",				MITYPE_COMPATFLAG, COMPATF_MISSILECLIP},
	{ "compat_crossdropoff",			MITYPE_COMPATFLAG, COMPATF_CROSSDROPOFF},
	{ "cd_start_track",					MITYPE_EATNEXT,	0, 0 },
	{ "cd_end1_track",					MITYPE_EATNEXT,	0, 0 },
	{ "cd_end2_track",					MITYPE_EATNEXT,	0, 0 },
	{ "cd_end3_track",					MITYPE_EATNEXT,	0, 0 },
	{ "cd_intermission_track",			MITYPE_EATNEXT,	0, 0 },
	{ "cd_title_track",					MITYPE_EATNEXT,	0, 0 },
	{ NULL, MITYPE_IGNORE, 0}
};

//==========================================================================
//
// ParseMapDefinition
// Parses the body of a map definition, including defaultmap etc.
//
//==========================================================================

void FMapInfoParser::ParseMapDefinition(level_info_t &info)
{
	int index;

	ParseOpenBrace();

	while (sc.GetString())
	{
		if ((index = sc.MatchString(&MapFlagHandlers->name, sizeof(*MapFlagHandlers))) >= 0)
		{
			MapInfoFlagHandler *handler = &MapFlagHandlers[index];
			switch (handler->type)
			{
			case MITYPE_EATNEXT:
				ParseOpenParen();
				sc.MustGetString();
				ParseCloseParen();
				break;

			case MITYPE_IGNORE:
				break;

			case MITYPE_SETFLAG:
				info.flags |= handler->data1;
				info.flags |= handler->data2;
				break;

			case MITYPE_CLRFLAG:
				info.flags &= ~handler->data1;
				info.flags |= handler->data2;
				break;

			case MITYPE_SCFLAGS:
				info.flags = (info.flags & handler->data2) | handler->data1;
				break;

			case MITYPE_SETFLAG2:
				info.flags2 |= handler->data1;
				info.flags2 |= handler->data2;
				break;

			case MITYPE_CLRFLAG2:
				info.flags2 &= ~handler->data1;
				info.flags2 |= handler->data2;
				break;

			case MITYPE_SCFLAGS2:
				info.flags2 = (info.flags2 & handler->data2) | handler->data1;
				break;

			case MITYPE_COMPATFLAG:
			{
				int set = 1;
				if (format_type == FMT_New)
				{
					if (CheckOpenParen())
					{
						sc.MustGetNumber();
						set = sc.Number;
					}
				}
				else
				{
					if (sc.CheckNumber()) set = sc.Number;
				}

				if (set) info.compatflags |= handler->data1;
				else info.compatflags &= ~handler->data1;
				info.compatmask |= handler->data1;
			}
			break;

			default:
				// should never happen
				assert(false);
				break;
			}
		}


		else if (!ParseCloseBrace())
		{
			// Unknown
			sc.ScriptMessage("Unknown property '%s' found in map definition\n", sc.String);
			SkipToNext();
		}
		else
		{
			break;
		}
	}
}


//==========================================================================
//
// GetDefaultLevelNum
// Gets a default level num from a map name.
//
//==========================================================================

static int GetDefaultLevelNum(const char *mapname)
{
	if (!strnicmp (mapname, "MAP", 3) && strlen(mapname) <= 5)
	{
		int mapnum = atoi (mapname + 3);

		if (mapnum >= 1 && mapnum <= 99)
			return mapnum;
	}
	else if (mapname[0] == 'E' &&
			mapname[1] >= '0' && mapname[1] <= '9' &&
			mapname[2] == 'M' &&
			mapname[3] >= '0' && mapname[3] <= '9')
	{
		int epinum = mapname[1] - '1';
		int mapnum = mapname[3] - '0';
		return epinum*10 + mapnum;
	}
	return 0;
}

//==========================================================================
//
// ParseMapHeader
// Parses the header of a map definition ('map mapxx mapname')
//
//==========================================================================

level_info_t *FMapInfoParser::ParseMapHeader(level_info_t &defaultinfo)
{
	FName mapname;
	bool HexenHack = false;

	if (sc.CheckNumber())
	{	// MAPNAME is a number; assume a Hexen wad
		char maptemp[8];
		mysnprintf (maptemp, countof(maptemp), "MAP%02d", sc.Number);
		mapname = maptemp;
		HexenHack = true;
	}
	else 
	{
		mapname = sc.String;
	}
	int levelindex = FindWadLevelInfo (mapname);
	if (levelindex == -1)
	{
		levelindex = wadlevelinfos.Reserve(1);
	}
	level_info_t *levelinfo = &wadlevelinfos[levelindex];
	*levelinfo = defaultinfo;
	if (HexenHack)
	{
		levelinfo->WallHorizLight = levelinfo->WallVertLight = 0;

		// Hexen levels are automatically nointermission,
		// no auto sound sequences, falling damage,
		// monsters activate their own specials, and missiles
		// are always the activators of impact lines.
		levelinfo->flags |= LEVEL_NOINTERMISSION
						 | LEVEL_SNDSEQTOTALCTRL
						 | LEVEL_FALLDMG_HX
						 | LEVEL_ACTOWNSPECIAL;
		levelinfo->flags2|= LEVEL2_HEXENHACK
						 | LEVEL2_INFINITE_FLIGHT
						 | LEVEL2_MISSILESACTIVATEIMPACT
						 | LEVEL2_MONSTERFALLINGDAMAGE;

	}

	uppercopy (levelinfo->mapname, mapname);
	sc.MustGetString ();
	if (sc.String[0] == '$')
	{
		// For consistency with other definitions allow $Stringtablename here, too.
		levelinfo->flags |= LEVEL_LOOKUPLEVELNAME;
		levelinfo->LevelName = sc.String + 1;
	}
	else
	{
		if (sc.Compare ("lookup"))
		{
			sc.MustGetString ();
			levelinfo->flags |= LEVEL_LOOKUPLEVELNAME;
		}
		levelinfo->LevelName = sc.String;
	}

	// Set up levelnum now so that you can use Teleport_NewMap specials
	// to teleport to maps with standard names without needing a levelnum.
	levelinfo->levelnum = GetDefaultLevelNum(levelinfo->mapname);

	return levelinfo;
}


//==========================================================================
//
// Episode definitions start with the header "episode <start-map>"
// and then can be followed by any of the following:
//
// name "Episode name as text"
// picname "Picture to display the episode name"
// key "Shortcut key for the menu"
// noskillmenu
// remove
//
//==========================================================================

void FMapInfoParser::ParseEpisodeInfo ()
{
	int i;
	char map[9];
	char *pic = NULL;
	bool picisgfx = false;	// Shut up, GCC!!!!
	bool remove = false;
	char key = 0;
	bool noskill = false;
	bool optional = false;
	bool extended = false;

	// Get map name
	sc.MustGetString ();
	uppercopy (map, sc.String);
	map[8] = 0;

	if (sc.CheckString ("teaser"))
	{
		sc.MustGetString ();
		if (gameinfo.flags & GI_SHAREWARE)
		{
			uppercopy (map, sc.String);
		}
		sc.MustGetString ();
	}

	ParseOpenBrace();

	while (sc.GetString())
	{
		if (sc.Compare ("optional"))
		{
			// For M4 in Doom
			optional = true;
		}
		else if (sc.Compare ("extended"))
		{
			// For M4 and M5 in Heretic
			extended = true;
		}
		else if (sc.Compare ("name"))
		{
			ParseOpenParen();
			sc.MustGetString ();
			ReplaceString (&pic, sc.String);
			picisgfx = false;
			ParseCloseParen();
		}
		else if (sc.Compare ("picname"))
		{
			ParseOpenParen();
			sc.MustGetString ();
			ReplaceString (&pic, sc.String);
			picisgfx = true;
			ParseCloseParen();
		}
		else if (sc.Compare ("remove"))
		{
			remove = true;
		}
		else if (sc.Compare ("key"))
		{
			ParseOpenParen();
			sc.MustGetString ();
			key = sc.String[0];
			ParseCloseParen();
		}
		else if (sc.Compare("noskillmenu"))
		{
			noskill = true;
		}
		else if (!ParseCloseBrace())
		{
			// Unknown
			sc.ScriptMessage("Unknown property '%s' found in episode definition\n", sc.String);
			SkipToNext();
		}
		else
		{
			break;
		}
	}

	if (extended && !(gameinfo.flags & GI_MENUHACK_EXTENDED))
	{ // If the episode is for the extended Heretic, but this is
	  // not the extended Heretic, ignore it.
		return;
	}

	if (optional && !remove)
	{
		if (!P_CheckMapData(map))
		{
			// If the episode is optional and the map does not exist
			// just ignore this episode definition.
			return;
		}
	}


	for (i = 0; i < EpiDef.numitems; ++i)
	{
		if (strncmp (EpisodeMaps[i], map, 8) == 0)
		{
			break;
		}
	}

	if (remove)
	{
		// If the remove property is given for an episode, remove it.
		if (i < EpiDef.numitems)
		{
			if (i+1 < EpiDef.numitems)
			{
				memmove (&EpisodeMaps[i], &EpisodeMaps[i+1],
					sizeof(EpisodeMaps[0])*(EpiDef.numitems - i - 1));
				memmove (&EpisodeMenu[i], &EpisodeMenu[i+1],
					sizeof(EpisodeMenu[0])*(EpiDef.numitems - i - 1));
				memmove (&EpisodeNoSkill[i], &EpisodeNoSkill[i+1], 
					sizeof(EpisodeNoSkill[0])*(EpiDef.numitems - i - 1));
			}
			EpiDef.numitems--;
		}
	}
	else
	{
		if (pic == NULL)
		{
			pic = copystring (map);
			picisgfx = false;
		}

		if (i == EpiDef.numitems)
		{
			if (EpiDef.numitems == MAX_EPISODES)
			{
				i = EpiDef.numitems - 1;
			}
			else
			{
				i = EpiDef.numitems++;
			}
		}
		else
		{
			delete[] const_cast<char *>(EpisodeMenu[i].name);
		}

		EpisodeMenu[i].name = pic;
		EpisodeMenu[i].alphaKey = tolower(key);
		EpisodeMenu[i].fulltext = !picisgfx;
		EpisodeNoSkill[i] = noskill;
		strncpy (EpisodeMaps[i], map, 8);
	}
}


//==========================================================================
//
// Clears episode definitions
//
//==========================================================================

void ClearEpisodes()
{
	for (int i = 0; i < EpiDef.numitems; ++i)
	{
		delete[] const_cast<char *>(EpisodeMenu[i].name);
		EpisodeMenu[i].name = NULL;
	}
	EpiDef.numitems = 0;
}

//==========================================================================
//
// SetLevelNum
// Avoid duplicate levelnums. The level being set always has precedence.
//
//==========================================================================

static void SetLevelNum (level_info_t *info, int num)
{
	for (unsigned int i = 0; i < wadlevelinfos.Size(); ++i)
	{
		if (wadlevelinfos[i].levelnum == num)
			wadlevelinfos[i].levelnum = 0;
	}
	info->levelnum = num;
}

//==========================================================================
//
// G_DoParseMapInfo
// Parses a single MAPINFO lump
// data for wadlevelinfos and wadclusterinfos.
//
//==========================================================================

void FMapInfoParser::ParseMapInfo (int lump, level_info_t &gamedefaults)
{
	level_info_t defaultinfo;

	sc.OpenLumpNum(lump);

	defaultinfo = gamedefaults;
	HexenHack = false;

	while (sc.GetString ())
	{
		if (sc.Compare("gamedefaults"))
		{
			gamedefaults.Reset();
			ParseMapDefinition(gamedefaults);
			defaultinfo = gamedefaults;
		}
		else if (sc.Compare("defaultmap"))
		{
			defaultinfo = gamedefaults;
			ParseMapDefinition(defaultinfo);
		}
		else if (sc.Compare("adddefaultmap"))
		{
			// Same as above but adds to the existing definitions instead of replacing them completely
			ParseMapDefinition(defaultinfo);
		}
		else if (sc.Compare("map"))
		{
			level_info_t *levelinfo = ParseMapHeader(defaultinfo);

			ParseMapDefinition(*levelinfo);

			// When the second sky is -NOFLAT-, make it a copy of the first sky
			if (strcmp (levelinfo->skypic2, "-NOFLAT-") == 0)
			{
				strcpy (levelinfo->skypic2, levelinfo->skypic1);
			}
			SetLevelNum (levelinfo, levelinfo->levelnum);	// Wipe out matching levelnums from other maps.
		}
		else if (sc.Compare("clusterdef"))
		{
			ParseCluster();
		}
		else if (sc.Compare("episode"))
		{
			ParseEpisodeInfo();
		}
		else if (sc.Compare("clearepisodes"))
		{
			ClearEpisodes();
		}
		else if (sc.Compare("skill"))
		{
			ParseSkill();
		}
		else if (sc.Compare("clearskills"))
		{
			AllSkills.Clear();
		}
	}
}


//==========================================================================
//
// G_ParseMapInfo
// Parses the MAPINFO lumps of all loaded WADs and generates
// data for wadlevelinfos and wadclusterinfos.
//
//==========================================================================

void G_ParseMapInfo ()
{
	int lump, lastlump = 0;
	level_info_t gamedefaults;

	// Parse the default MAPINFO for the current game.
	for(int i=0; i<2; i++)
	{
		if (gameinfo.mapinfo[i] != NULL)
		{
			FMapInfoParser parse;
			parse.ParseMapInfo(Wads.GetNumForFullName(gameinfo.mapinfo[i]), gamedefaults);
		}
	}

	// Parse any extra MAPINFOs.
	while ((lump = Wads.FindLump ("MAPINFO", &lastlump)) != -1)
	{
		FMapInfoParser parse;
		parse.ParseMapInfo(lump, gamedefaults);
	}
	EndSequences.ShrinkToFit ();

	if (EpiDef.numitems == 0)
	{
		I_FatalError ("You cannot use clearepisodes in a MAPINFO if you do not define any new episodes after it.");
	}
	if (AllSkills.Size()==0)
	{
		I_FatalError ("You cannot use clearskills in a MAPINFO if you do not define any new skills after it.");
	}
}

