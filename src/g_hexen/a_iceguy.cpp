/*
#include "actor.h"
#include "info.h"
#include "p_local.h"
#include "s_sound.h"
#include "p_enemy.h"
#include "a_action.h"
#include "m_random.h"
#include "thingdef/thingdef.h"
*/

static FRandom pr_iceguylook ("IceGuyLook");
static FRandom pr_iceguychase ("IceGuyChase");

static const char *WispTypes[2] =
{
	"IceGuyWisp1",
	"IceGuyWisp2",
};

//============================================================================
//
// A_IceGuyLook
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_IceGuyLook)
{
	PARAM_ACTION_PROLOGUE;

	fixed_t dist;
	fixed_t an;

	CALL_ACTION(A_Look, self);
	if (pr_iceguylook() < 64)
	{
		dist = ((pr_iceguylook()-128)*self->radius)>>7;
		an = (self->angle+ANG90)>>ANGLETOFINESHIFT;

		Spawn (WispTypes[pr_iceguylook()&1],
			self->x+FixedMul(dist, finecosine[an]),
			self->y+FixedMul(dist, finesine[an]),
			self->z+60*FRACUNIT, ALLOW_REPLACE);
	}
	return 0;
}

//============================================================================
//
// A_IceGuyChase
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_IceGuyChase)
{
	PARAM_ACTION_PROLOGUE;

	fixed_t dist;
	fixed_t an;
	AActor *mo;

	A_Chase (stack, self);
	if (pr_iceguychase() < 128)
	{
		dist = ((pr_iceguychase()-128)*self->radius)>>7;
		an = (self->angle+ANG90)>>ANGLETOFINESHIFT;

		mo = Spawn (WispTypes[pr_iceguychase()&1],
			self->x+FixedMul(dist, finecosine[an]),
			self->y+FixedMul(dist, finesine[an]),
			self->z+60*FRACUNIT, ALLOW_REPLACE);
		if (mo)
		{
			mo->velx = self->velx;
			mo->vely = self->vely;
			mo->velz = self->velz;
			mo->target = self;
		}
	}
	return 0;
}

//============================================================================
//
// A_IceGuyAttack
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_IceGuyAttack)
{
	PARAM_ACTION_PROLOGUE;

	fixed_t an;

	if(!self->target) 
	{
		return 0;
	}
	an = (self->angle+ANG90)>>ANGLETOFINESHIFT;
	P_SpawnMissileXYZ(self->x+FixedMul(self->radius>>1,
		finecosine[an]), self->y+FixedMul(self->radius>>1,
		finesine[an]), self->z+40*FRACUNIT, self, self->target,
		PClass::FindClass ("IceGuyFX"));
	an = (self->angle-ANG90)>>ANGLETOFINESHIFT;
	P_SpawnMissileXYZ(self->x+FixedMul(self->radius>>1,
		finecosine[an]), self->y+FixedMul(self->radius>>1,
		finesine[an]), self->z+40*FRACUNIT, self, self->target,
		PClass::FindClass ("IceGuyFX"));
	S_Sound (self, CHAN_WEAPON, self->AttackSound, 1, ATTN_NORM);
	return 0;
}

//============================================================================
//
// A_IceGuyDie
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_IceGuyDie)
{
	PARAM_ACTION_PROLOGUE;

	self->velx = 0;
	self->vely = 0;
	self->velz = 0;
	self->height = self->GetDefault()->height;
	CALL_ACTION(A_FreezeDeathChunks, self);
	return 0;
}

//============================================================================
//
// A_IceGuyMissileExplode
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_IceGuyMissileExplode)
{
	PARAM_ACTION_PROLOGUE;

	AActor *mo;
	int i;

	for (i = 0; i < 8; i++)
	{
		mo = P_SpawnMissileAngleZ (self, self->z+3*FRACUNIT, 
			PClass::FindActor("IceGuyFX2"), i*ANG45, (fixed_t)(-0.3*FRACUNIT));
		if (mo)
		{
			mo->target = self->target;
		}
	}
	return 0;
}

