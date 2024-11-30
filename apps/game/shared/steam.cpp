#include "main.h"
#include "steam.h"

#if CH_CLIENT
#include "../client/cl_main.h"
#endif

#include "game_shared.h"


// =========================================================
// Sidury Steam To Game Interface
// =========================================================


SteamToGame gSteamToGame;
bool        gSteamLoaded = false;


void SteamToGame::OnRequestAvatarImage( SteamID64_t sSteamID, ESteamAvatarSize sSize, ch_handle_t sAvatar )
{
#if CH_CLIENT
	CL_SetClientSteamAvatar( sSteamID, sSize, sAvatar );
#endif

	Log_Msg( "cool we got steam avatar\n" );
}


void SteamToGame::OnRequestProfileName( SteamID64_t sSteamID, const char* spName )
{
	Log_MsgF( "cool we got steam profile name: %s\n", spName );
}


// =========================================================
// Sidury Steam Helper Functions
// =========================================================


void Steam_Init()
{
	if ( !steam )
		return;

	gSteamLoaded = steam->LoadSteam( &gSteamToGame );

	// CH_STEAM_CALL( LoadSteamOverlay() );

	if ( IsSteamLoaded() )
	{
		Log_MsgF( "Steam Profile Name: %s\n", steam->GetPersonaName() );
	}
}


void Steam_Shutdown()
{
	if ( !steam )
		return;

	steam->Shutdown();
	gSteamLoaded = false;
}

