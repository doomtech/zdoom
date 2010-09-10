#ifndef EGAMETYPE
#define EGAMETYPE
enum EGameType
{
	GAME_Any	 = 0,
	GAME_Doom	 = 1,
	GAME_Heretic = 2,
	GAME_Hexen	 = 4,
	GAME_Strife	 = 8,
	GAME_Chex	 = 16, // Chex is basically Doom, but we need to have a different set of actors.
	GAME_Doom64  = 32, // Like Chex, it needs a different set of actors, even if they're mostly the same.

	GAME_Raven			= GAME_Heretic|GAME_Hexen,
	GAME_DoomStrife		= GAME_Doom|GAME_Strife,
	GAME_DoomChex		= GAME_Doom|GAME_Chex,
	GAME_DoomStrifeChex	= GAME_Doom|GAME_Strife|GAME_Chex,
	GAME_NotRaven		= GAME_DoomStrifeChex|GAME_Doom64
};
#endif

