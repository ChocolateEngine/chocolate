#pragma once

#include "ch_isteam.h"
#include "steam_api.h"

#include "render/irender.h"


extern IRender* render;


#define CH_STEAM_LOADED_RET( failReturn ) \
	if ( !gSteamLoaded ) \
		failReturn


class SteamAbstraction : public ISteamSystem
{
  public:
	// ==================================================
	// Base ISystem Functions
	// ==================================================

	virtual bool            Init() override;
	virtual void            Update( float sFrameTime ) override;

	// ==================================================
	// General Steam Abstraction Functions
	// ==================================================

	// Try to load the Steam API
	virtual bool            LoadSteam( ISteamToGame* spSteamToGame ) override;

	// Shutdown the Steam API
	virtual void            Shutdown() override;

	// Load the Steam Overlay
	virtual void            LoadSteamOverlay() override;

	// ==================================================
	// SteamFriends
	// ==================================================

	virtual const char*     GetPersonaName() override;

	virtual SteamID64_t     GetSteamID() override;

	// ==================================================
	// Steam Requests
	// ==================================================

	// Load the Avatar Image through the renderer dll
	virtual bool            RequestAvatarImage( ESteamAvatarSize sSize, SteamID64_t sSteamID ) override;

	virtual bool            RequestProfileName( SteamID64_t sSteamID ) override;

	// ==================================================
	// Internal Functions
	// ==================================================

	CSteamID                GetSteamIDFromHandle( SteamID64_t sHandle );

	void                    LoadAvatarImage( AvatarImageLoaded_t* spParam );

	// ==================================================
	// Callback Functions
	// ==================================================

	//void                    OnAvatarImageLoaded( AvatarImageLoaded_t* spParam );
	//void                    OnProfileDataLoaded( PersonaStateChange_t* spParam );

	STEAM_CALLBACK( SteamAbstraction, OnAvatarImageLoaded, AvatarImageLoaded_t );
	STEAM_CALLBACK( SteamAbstraction, OnProfileDataLoaded, PersonaStateChange_t );
};

extern SteamAbstraction* gSteam;

