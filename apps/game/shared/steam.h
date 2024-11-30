#pragma once

#include "../steam/ch_isteam.h"

// =========================================================
// Sidury Steam To Game Interface
// =========================================================


class ISteamToGame;


#define CH_STEAM_CALL( steamFuncCall ) \
  if ( gSteamLoaded && steam )         \
  steam->steamFuncCall


inline bool IsSteamLoaded()
{
	return steam && gSteamLoaded;
}


class SteamToGame : public ISteamToGame
{
  public:
	void OnRequestAvatarImage( SteamID64_t sSteamID, ESteamAvatarSize sSize, ch_handle_t sAvatar ) override;
	void OnRequestProfileName( SteamID64_t sSteamID, const char* spName ) override;
};


// =========================================================
// Sidury Steam Helper Functions
// =========================================================


void Steam_Init();
void Steam_Shutdown();

