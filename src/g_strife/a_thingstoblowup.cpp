/*
#include "actor.h"
#include "m_random.h"
#include "p_local.h"
#include "c_console.h"
#include "p_enemy.h"
#include "a_action.h"
#include "gstrings.h"
#include "thingdef/thingdef.h"
#include "thingdef/thingdef.h"
#include "doomstat.h"
*/

static FRandom pr_bang4cloud ("Bang4Cloud");
static FRandom pr_lightout ("LightOut");

extern const PClass *QuestItemClasses[31];

DEFINE_ACTION_FUNCTION(AActor, A_Bang4Cloud)
{
	PARAM_ACTION_PROLOGUE;

	fixed_t spawnx, spawny;

	spawnx = self->x + (pr_bang4cloud.Random2() & 3) * 10240;
	spawny = self->y + (pr_bang4cloud.Random2() & 3) * 10240;

	Spawn("Bang4Cloud", spawnx, spawny, self->z, ALLOW_REPLACE);
	return 0;
}

// -------------------------------------------------------------------

DEFINE_ACTION_FUNCTION_PARAMS(AActor, A_GiveQuestItem)
{
	PARAM_ACTION_PROLOGUE;
	PARAM_INT(questitem);

	// Give one of these quest items to every player in the game
	if (questitem >= 0 && questitem < countof(QuestItemClasses))
	{
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if (playeringame[i])
			{
				AInventory *item = static_cast<AInventory *>(Spawn (QuestItemClasses[questitem - 1], 0,0,0, NO_REPLACE));
				if (!item->CallTryPickup (players[i].mo))
				{
					item->Destroy ();
				}
			}
		}
	}

	char messageid[64];

	mysnprintf(messageid, countof(messageid), "TXT_QUEST_%d", questitem);
	const char * name = GStrings[messageid];

	if (name != NULL)
	{
		C_MidPrint (SmallFont, name);
	}
	return 0;
}

// PowerCrystal -------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_ExtraLightOff)
{
	PARAM_ACTION_PROLOGUE;

	if (self->target != NULL && self->target->player != NULL)
	{
		self->target->player->extralight = 0;
	}
	return 0;
}

DEFINE_ACTION_FUNCTION(AActor, A_Explode512)
{
	PARAM_ACTION_PROLOGUE;

	P_RadiusAttack (self, self->target, 512, 512, NAME_None, true);
	if (self->target != NULL && self->target->player != NULL)
	{
		self->target->player->extralight = 5;
	}
	if (self->z <= self->floorz + (512<<FRACBITS))
	{
		P_HitFloor (self);
	}

	// Strife didn't do this next part, but it looks good
	self->RenderStyle = STYLE_Add;
	return 0;
}

DEFINE_ACTION_FUNCTION(AActor, A_LightGoesOut)
{
	PARAM_ACTION_PROLOGUE;

	AActor *foo;
	sector_t *sec = self->Sector;
	vertex_t *spot;
	fixed_t newheight;

	sec->SetLightLevel(0);

	newheight = sec->FindLowestFloorSurrounding (&spot);
	sec->floorplane.d = sec->floorplane.PointToDist (spot, newheight);

	for (int i = 0; i < 8; ++i)
	{
		foo = Spawn("Rubble1", self->x, self->y, self->z, ALLOW_REPLACE);
		if (foo != NULL)
		{
			int t = pr_lightout() & 15;
			foo->velx = (t - (pr_lightout() & 7)) << FRACBITS;
			foo->vely = (pr_lightout.Random2() & 7) << FRACBITS;
			foo->velz = (7 + (pr_lightout() & 3)) << FRACBITS;
		}
	}
	return 0;
}
