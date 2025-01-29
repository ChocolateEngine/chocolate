// =====================================================
// Steam abstraction for Sidury
// =====================================================
#pragma once

#include "core/core.h"


using SteamID64_t = u64;


enum ESteamAvatarSize
{
	ESteamAvatarSize_Small,
	ESteamAvatarSize_Medium,
	ESteamAvatarSize_Large,
};


// This is an interface defined by the client for the steam abstraction to get data from
// or to call when callbacks are receieved
class ISteamToGame
{
  public:
	virtual void OnRequestAvatarImage( SteamID64_t sSteamID, ESteamAvatarSize sSize, ch_handle_t sAvatar ) = 0;
	virtual void OnRequestProfileName( SteamID64_t sSteamID, const char* spName ) = 0;
};


class ISteamSystem : public ISystem
{
  public:
	// ==================================================
	// General Steam Abstraction Functions
	// ==================================================

	// Try to load the Steam API, while passing in your Interface for ISteamToGame
	virtual bool        LoadSteam( ISteamToGame* spSteamToGame )                           = 0;

	// Shutdown the Steam API
	virtual void        Shutdown()                                                         = 0;

	// Load the Steam Overlay
	virtual void        LoadSteamOverlay()                                                 = 0;

	// ==================================================
	// Steam User
	// ==================================================

	virtual const char* GetPersonaName()                                                   = 0;

	virtual SteamID64_t GetSteamID()                                                       = 0;

	// ==================================================
	// Steam Requests
	// ==================================================

	// Load the Avatar Image through the renderer dll
	virtual bool        RequestAvatarImage( ESteamAvatarSize sSize, SteamID64_t sSteamID ) = 0;

	virtual bool        RequestProfileName( SteamID64_t sSteamID )                         = 0;

};


#define ISTEAM_NAME "SteamAbstraction"
#define ISTEAM_VER  1

