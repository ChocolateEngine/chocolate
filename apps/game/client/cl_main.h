#pragma once

#include "../steam/ch_isteam.h"

//
// The Client, always running unless a dedicated server
// 


using SteamID64_t = u64;
using Entity = size_t;

struct UserCmd_t;

enum ESteamAvatarSize;

extern UserCmd_t gClientUserCmd;


enum EClientState
{
	EClientState_Idle,
	EClientState_WaitForAccept,
	EClientState_WaitForFullUpdate,
	EClientState_Connecting,
	EClientState_Connected,
	EClientState_Count
};


struct CL_ServerData_t
{
	std::string name;
	std::string aMapName;
	u8          aClientCount;
	u8          aMaxClients;
};


// Information about each client that our client knows
struct CL_Client_t
{
	std::string name         = "[unnamed]";
	SteamID64_t aSteamID      = 0;
	Entity      aEntity       = CH_ENT_INVALID;

	ch_handle_t      aAvatarLarge  = CH_INVALID_HANDLE;
	ch_handle_t      aAvatarMedium = CH_INVALID_HANDLE;
	ch_handle_t      aAvatarSmall  = CH_INVALID_HANDLE;
};


extern CL_ServerData_t gClientServerData;

// =======================================================================
// General Client Functions
// =======================================================================

bool                   CL_Init();
void                   CL_Shutdown();
void                   CL_Update( float frameTime );
void                   CL_GameUpdate( float frameTime );

const char*            CL_GetUserName();
void                   CL_SetClientSteamAvatar( SteamID64_t sSteamID, ESteamAvatarSize sSize, ch_handle_t sAvatar );

// NOTE: Will only work when the avatar is already loaded
// though, maybe we can somehow pre-allocate that handle to a missing texture or something
// and then when we have downloaded the avatar from steam, we update the texture data without replacing the handle?
ch_handle_t                 CL_GetClientSteamAvatar( SteamID64_t sSteamID, ESteamAvatarSize sSize );

// Pick the best available steam avatar based on width
ch_handle_t                 CL_PickClientSteamAvatar( SteamID64_t sSteamID, int sWidth );

// =======================================================================
// Client Networking
// =======================================================================

void                   CL_Connect( const char* spAddress );
void                   CL_Disconnect( bool sSendReason = true, const char* spReason = nullptr );

// If we are running this command from the client, forward it to the server
bool                   CL_SendConVarIfClient( std::string_view sName, const std::vector< std::string >& srArgs = {} );

void                   CL_SendConVar( std::string_view sName, const std::vector< std::string >& srArgs = {} );
void                   CL_SendConVars();

int                    CL_WriteToServer( flatbuffers::FlatBufferBuilder& srBuilder );

void                   CL_HandleMsg_ClientInfo( const NetMsg_ServerClientInfo* spMessage );
void                   CL_HandleMsg_ServerInfo( const NetMsg_ServerInfo* spReader );

bool                   CL_WaitForAccept();
void                   CL_UpdateUserCmd();
void                   CL_BuildUserCmd( flatbuffers::FlatBufferBuilder& srBuilder );
void                   CL_SendUserCmd();
void                   CL_SendFullUpdateRequest();
void                   CL_GetServerMessages();

void                   CL_PrintStatus();

void                   CL_CreateServerEntity();

// =======================================================================
// Main Menu
// =======================================================================

bool                   CL_IsMenuShown();
void                   CL_UpdateMenuShown();

void                   CL_DrawMainMenu();

