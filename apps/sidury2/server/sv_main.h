#pragma once

#include "main.h"
#include "game_shared.h"
#include "network/net_main.h"

// 
// The Server, only runs if the engine is a dedicated server, or hosting on the client
// 


using ClientHandle_t = unsigned int;

// Absolute Max Client Limit
constexpr ClientHandle_t CH_MAX_CLIENTS    = 32;

// Invalid ClientHandle_t
constexpr ClientHandle_t CH_INVALID_CLIENT = 0;


using SteamID64_t = u64;

struct UserCmd_t;


enum ESVClientState
{
	ESV_ClientState_Disconnected,
	ESV_ClientState_WaitForClientInfo,  // We sent the client NetMsg_ServerConnectResponse, and we are waiting for their client info
	ESV_ClientState_Connecting,         // We send them a full update and server info, so we are now waiting for them to finish loading
	ESV_ClientState_Connected,
};


// Data for each client
struct SV_Client_t
{
	ch_sockaddr    aAddr;

	SteamID64_t    aSteamID = 0;

	std::string    aName    = "[unnamed]";
	ESVClientState aState;

	double         aTimeout;

	// Entity         aEntity = CH_ENT_INVALID;

	UserCmd_t      aUserCmd;

	int            Read( char* spData, int sLen );

	int            Write( const char* spData, int sLen );
	int            Write( const ChVector< char >& srData );

	bool           operator==( const SV_Client_t& srOther )
	{
		if ( this == &srOther )
			return true;

		if ( memcmp( this, &srOther, sizeof( SV_Client_t ) ) == 0 )
			return true;

		if ( memcmp( aAddr.sa_data, srOther.aAddr.sa_data, sizeof( aAddr.sa_data ) ) != 0 )
			return false;

		if ( aAddr.sa_family != srOther.aAddr.sa_family )
			return false;

		if ( aName != srOther.aName )
			return false;

		if ( aTimeout != aTimeout )
			return false;

		// if ( aEntity != aEntity )
		// 	return false;

		if ( memcmp( &aUserCmd, &srOther.aUserCmd, sizeof( aUserCmd ) ) != 0 )
			return false;

		return true;
	}
};


struct ServerData_t
{
	bool                                               aActive;

	std::vector< SV_Client_t >                         aClients;

	// Fixed handles to a client, so the indexes can easily change
	// This also allows you to change max clients live in game
	std::unordered_map< ClientHandle_t, SV_Client_t* > aClientIDs;
	std::unordered_map< SV_Client_t*, ClientHandle_t > aClientToIDs;

	// Clients that are still connecting
	ChVector< SV_Client_t* >                           aClientsConnecting;

	// Clients that want a full update
	ChVector< SV_Client_t* >                           aClientsFullUpdate;
};

// --------------------------------------------------------------------

bool                SV_Init();
void                SV_Shutdown();
void                SV_Update( float frameTime );
void                SV_GameUpdate( float frameTime );

bool                SV_StartServer();
void                SV_StopServer();
bool                SV_IsHosting();

// --------------------------------------------------------------------
// Networking

// int                 SV_BroadcastMsgsToSpecificClients( std::vector< flatbuffers::FlatBufferBuilder >& srMessages, const ChVector< SV_Client_t* >& srClients );
// int                 SV_BroadcastMsgs( std::vector< flatbuffers::FlatBufferBuilder >& srMessages );
// int                 SV_BroadcastMsg( flatbuffers::FlatBufferBuilder& srMessage );
// 
// bool                SV_SendMessageToClient( SV_Client_t& srClient, flatbuffers::FlatBufferBuilder& srMessage );
void                SV_SendDisconnect( SV_Client_t& srClient );

// bool                SV_BuildServerMsg( flatbuffers::FlatBufferBuilder& srMessage, EMsgSrc_Server sSrcType, bool sFullUpdate = false );

void                SV_ProcessSocketMsgs();
// void                SV_ProcessClientMsg( SV_Client_t& srClient, const MsgSrc_Client* spMessage );

SV_Client_t*        SV_AllocateClient();
void                SV_FreeClient( SV_Client_t& srClient );

void                SV_ConnectClient( ch_sockaddr& srAddr, ChVector< char >& srData );
void                SV_ConnectClientFinish( SV_Client_t& srClient );

// void                SV_SendConVar( std::string_view sName, const std::vector< std::string >& srArgs );
void                SV_SendConVar( ConVarBase* spConVar );
bool                SV_BuildConVarMsg( bool sFullUpdate = false );

// --------------------------------------------------------------------
// Helper Functions

// size_t              SV_GetClientCount();
SV_Client_t*        SV_GetClient( ClientHandle_t sClient );

void                SV_SetCommandClient( SV_Client_t* spClient );
SV_Client_t*        SV_GetCommandClient();

// Entity              SV_GetCommandClientEntity();
// SV_Client_t*        SV_GetClientFromEntity( Entity sEntity );
// 
// Entity              SV_GetPlayerEntFromIndex( size_t sIndex );
// Entity              SV_GetPlayerEnt( ClientHandle_t sClient );

void                SV_PrintStatus();

// --------------------------------------------------------------------

extern ServerData_t gServerData;

