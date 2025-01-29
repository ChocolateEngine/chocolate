#include "sv_main.h"
#include "game_shared.h"
#include "main.h"
#include "map_manager.h"

#include "igui.h"

#include <unordered_set>

//
// The Server, only runs if the engine is a dedicated server, or hosting on the client
//

LOG_REGISTER_CHANNEL2( Server, LogColor::Green );

static const char*                       gServerPort = Args_Register( "41628", "Test Server Port", "-port" );
static std::unordered_set< ConVarBase* > gReplicatedCmds;

CONVAR( sv_server_name, "taco", CVARF_SERVER | CVARF_ARCHIVE );
CONVAR( sv_client_timeout, 30.f, CVARF_SERVER | CVARF_ARCHIVE );
CONVAR( sv_client_timeout_enable, 1, CVARF_SERVER | CVARF_ARCHIVE );
CONVAR( sv_pause, 0, CVARF_SERVER, "Pauses the Server Code" );


CONVAR_CMD_EX( sv_max_clients, 32, CVARF_SERVER, "Max Clients the Server Allows" )
{
	if ( sv_max_clients.GetInt() > CH_MAX_CLIENTS )
	{
		Log_WarnF( gLC_Server, "Can't go over max internal client limit of %d\n", CH_MAX_CLIENTS );
		sv_max_clients.SetValue( CH_MAX_CLIENTS );
	}
	else if ( sv_max_clients < gServerData.aClients.size() )
	{
		Log_WarnF( gLC_Server, "Can't reduce max client limit less than the amount of clients connected (%zd connected)\n", gServerData.aClients.size() );
		sv_max_clients.SetValue( gServerData.aClients.size() );
	}
	else if ( sv_max_clients < 1 )
	{
		Log_WarnF( gLC_Server, "Can't set max clients less than 1\n" );
		sv_max_clients.SetValue( 1 );
	}
}


bool CvarFReplicatedCallback( ConVarBase* spBase, const std::vector< std::string >& args )
{
	if ( args.empty() )
		return true;

	if ( typeid( *spBase ) != typeid( ConVar ) )
	{
		Log_ErrorF( gLC_Server, "ConCommand or ConVarRef found using CVARF_REPLICATED: \"%s\"\n", spBase->aName );
		return true;
	}

	// If were hosting a server, the message has to be from the console
	if ( SV_IsHosting() && Game_GetCommandSource() == ECommandSource_Console )
	{
		// Add this to a vector of convars to send values to the client
		gReplicatedCmds.emplace( spBase );
		return true;
	}
	// The Message Has to be from the server otherwise
	else if ( Game_GetCommandSource() == ECommandSource_Server )
	{
		return true;
	}

	Log_ErrorF( gLC_Server, "Can't change Server Replicated ConVar if you're not the host! - \"%s\"\n", spBase->aName );
	return false;
}


ServerData_t        gServerData;

static Socket_t     gServerSocket   = CH_INVALID_SOCKET;

static SV_Client_t* gpCommandClient = nullptr;


CONCMD( pause )
{
	if ( !SV_IsHosting() )
		return;

	Game_SetPaused( !Game_IsPaused() );
}


int SV_Client_t::Read( char* spData, int sLen )
{
	return Net_Read( gServerSocket, spData, sLen, &aAddr );
}


int SV_Client_t::Write( const char* spData, int sLen )
{
	return Net_Write( gServerSocket, aAddr, spData, sLen );
}


int SV_Client_t::Write( const ChVector< char >& srData )
{
	return Net_Write( gServerSocket, aAddr, srData.begin(), srData.size_bytes() );
}


// capnp::FlatArrayMessageReader& SV_GetReader( Socket_t sSocket, ChVector< char >& srData, ch_sockaddr& srAddr )
// {
// 	int len = Net_Read( sSocket, srData.data(), srData.size(), &srAddr );
// 
// 	if ( len < 0 )
// 		return;
// 
// 	capnp::FlatArrayMessageReader reader( kj::ArrayPtr< const capnp::word >( (const capnp::word*)srData.data(), srData.size() ) );
// 	return reader;
// }


bool SV_Init()
{
	Con_SetCvarFlagCallback( CVARF_REPLICATED, CvarFReplicatedCallback );

	Game_SetCommandSource( ECommandSource_Console );

	return true;
}


void SV_Shutdown()
{
	SV_StopServer();
}


void SV_Update( float frameTime )
{
	PROF_SCOPE();

	if ( sv_pause )
		return;

	Game_SetCommandSource( ECommandSource_Client );

	// for ( auto& client : gServerData.aClients )
	for ( size_t i = 0; i < gServerData.aClients.size(); i++ )
	{
		SV_Client_t& client = gServerData.aClients[ i ];

		// Continue connecting clients if any are joining
		// if ( client.aState == ESV_ClientState_Connecting )
		// {
		// 	// SV_ConnectClientFinish( client );
		// 	continue;
		// }

		// Clean up Disconnected Clients
		if ( client.aState == ESV_ClientState_Disconnected )
		{
			// Remove their player entity
			// Entity_DeleteEntity( client.aEntity );

			// Remove this client from the list
			SV_FreeClient( client );
			i--;
			continue;
		}
	}

	SV_ProcessSocketMsgs();
	
	// Main game loop
	SV_GameUpdate( frameTime );

	// Write Data to Clients

	gReplicatedCmds.clear();

	Game_SetCommandSource( ECommandSource_Console );
}


void SV_GameUpdate( float frameTime )
{
	PROF_SCOPE();

	MapManager_Update();

	// players.Update( frameTime );

	Phys_Simulate( GetPhysEnv(), frameTime );

	// Entity_UpdateSystems();

	// Update player positions after physics simulation
	// NOTE: This probably needs to be done for everything with physics
	// for ( auto& player : players.aEntities )
	// {
	// 	players.apMove->UpdatePosition( player );
	// }
}


bool SV_StartServer()
{
	SV_StopServer();

	gServerSocket = Net_OpenSocket( gServerPort );

	if ( gServerSocket == CH_INVALID_SOCKET )
	{
		Log_Error( gLC_Server, "Failed to Open Test Server\n" );
		return false;
	}

	//players.Init();

	gServerData.aActive = true;
	return true;
}


void SV_StopServer()
{
	for ( auto& client : gServerData.aClients )
	{
		// SV_SendDisconnect( client );
	}

	MapManager_CloseMap();

	gServerData.aActive = false;
	gServerData.aClients.clear();

	Net_CloseSocket( gServerSocket );
	gServerSocket = CH_INVALID_SOCKET;
}


bool SV_IsHosting()
{
	return gServerData.aActive;
}


// --------------------------------------------------------------------
// Networking


SV_Client_t* SV_GetClientFromAddr( ch_sockaddr& srAddr )
{
	for ( auto& client : gServerData.aClients )
	{
		// if ( client.aAddr.sa_data == clientAddr.sa_data && client.aAddr.sa_family == clientAddr.sa_family )
		// if ( memcmp( client.aAddr.sa_data, clientAddr.sa_data ) == 0 && client.aAddr.sa_family == clientAddr.sa_family )
		if ( memcmp( client.aAddr.sa_data, srAddr.sa_data, sizeof( client.aAddr.sa_data ) ) == 0 )
		{
			return &client;
		}
	}

	return nullptr;
}


void SV_ProcessSocketMsgs()
{
	PROF_SCOPE();

	while ( true )
	{
		ChVector< char > data( 8192 );
		ch_sockaddr      clientAddr;
		int              len = Net_Read( gServerSocket, data.data(), data.size(), &clientAddr );

		if ( len <= 0 )
			return;

		data.resize( len );

		SV_Client_t* client = SV_GetClientFromAddr( clientAddr );

		if ( !client )
		{
			SV_ConnectClient( clientAddr, data );
			continue;
		}

		// Reset the connection timer
		client->aTimeout = Game_GetCurTime() + sv_client_timeout;

		// Parse Message

		// Read the message sent from the client
		//SV_ProcessClientMsg( *client );
	}
}


void SV_ProcessClientMsg( SV_Client_t& srClient )
{
	PROF_SCOPE();

	// Reset the connection timer
	srClient.aTimeout = Game_GetCurTime() + sv_client_timeout;

	// Read the message sent from the client
}


SV_Client_t* SV_AllocateClient()
{
	if ( gServerData.aClients.size() >= sv_max_clients )
		return nullptr;

	// Allocate a new client
	SV_Client_t&   client = gServerData.aClients.emplace_back();

	// Generate a random number to use as a Handle
	ClientHandle_t handle = CH_INVALID_CLIENT;

	while ( true )
	{
		handle  = ( rand() % 0xFFFFFFFE ) + 1;
	
		auto it = gServerData.aClientIDs.find( handle );
		if ( it == gServerData.aClientIDs.end() )
			break;
	}

	// Add it to the resource list to allow for changing indexes and max clients
	gServerData.aClientIDs[ handle ]    = &client;
	gServerData.aClientToIDs[ &client ] = handle;

	return &client;
}


void SV_FreeClient( SV_Client_t& srClient )
{
	auto it = gServerData.aClientToIDs.find( &srClient );
	
	if ( it == gServerData.aClientToIDs.end() )
		return;

	gServerData.aClientIDs.erase( it->second );
	gServerData.aClientToIDs.erase( it );

	// Remove this client from the list
	auto clientIT = std::find( gServerData.aClients.begin(), gServerData.aClients.end(), srClient );
	if ( clientIT != gServerData.aClients.end() )
	{
		size_t index = clientIT - gServerData.aClients.begin();

		if ( index != SIZE_MAX )
			vec_remove_index( gServerData.aClients, index );
		else
			Log_Msg( "um\n" );
	}
}


void SV_ConnectClientFinish( SV_Client_t& srClient )
{
	if ( srClient.aState != ESV_ClientState_Connecting )
	{
		//Log_MsgF( gLC_Server, "Recieved Conn Connected: \"%s\"\n", srClient.aName.c_str() );
		return;
	}

	srClient.aState = ESV_ClientState_Connected;

	gServerData.aClientsConnecting.erase( &srClient );
	
	// They just got here, so send them a full update
	gServerData.aClientsFullUpdate.push_back( &srClient );

	Log_MsgF( gLC_Server, "Client Connected: \"%s\"\n", srClient.aName.c_str() );

	// Spawn the player in!

	// Tell the clients someone else connected
	
	// Reset the connection timer
	srClient.aTimeout = Game_GetCurTime() + sv_client_timeout;
}


void SV_ConnectClient( ch_sockaddr& srAddr, ChVector< char >& srData )
{
	PROF_SCOPE();

	// First thing's first, make sure our protocol is the same

	// Make an entity for them

	// Add them to the client list for the playerInfo component
	SV_Client_t* client = SV_AllocateClient();

	if ( !client )
	{
		Log_ErrorF( gLC_Server, "Failed to Connect Client - At Max Players Limit: \"%s\"\n", Net_AddrToString( srAddr ) );
		// Entity_DeleteEntity( entity );
		return;
	}

	client->aAddr   = srAddr;
	client->aState  = ESV_ClientState_WaitForClientInfo;
	// client->aEntity = entity;

	Log_MsgF( gLC_Server, "Connecting Client: \"%s\"\n", Net_AddrToString( srAddr ) );

	// Add the playerInfo Component
	// void* playerInfo = Entity_AddComponent( entity, "playerInfo" );
	// 
	// if ( playerInfo == nullptr )
	// {
	// 	Log_MsgF( gLC_Server, "Failed to Connect Client - Failed to create a playerInfo component: \"%s\"\n", Net_AddrToString( srAddr ) );
	// 	Entity_DeleteEntity( entity );
	// 	client->aState = ESV_ClientState_Disconnected;
	// 	return;
	// }

	// Send them the accept message and server info

	// Build Server Connect Response
	// int write = Net_Write( gServerSocket, srAddr, data, dataLen );
	// 
	// if ( write > 0 )
	// {
	// 	gServerData.aClientsConnecting.push_back( client );
	// }
	// else
	// {
	// 	// drop (kick) them if we failed to write a message to them
	// 	client->aState = ESV_ClientState_Disconnected;
	// 	Log_ErrorF( gLC_Server, "Failed to Connect Client - Failed to send server info to them: \"%s\"\n", Net_AddrToString( srAddr ) );
	// 	return;
	// }

	// This next one should work
	// write = Net_WriteFlatBuffer( gServerSocket, srAddr, builders[ 1 ] );
}


// void SV_SendConVar( std::string_view sName, const std::vector< std::string >& srArgs )
// {
// 	// We have to rebuild this command to a normal string, fun
// 	// Allocate a command to send with the command name
// 	std::string& newCmd = gSvCommandsToSend.emplace_back( sName.data() );
// 
// 	// Add the command arguments
// 	for ( const std::string& arg : srArgs )
// 	{
// 		newCmd += " " + arg;
// 	}
// }


void SV_SendConVar( ConVarBase* spConVar )
{
	gReplicatedCmds.emplace( spConVar );
}


bool SV_BuildConVarMsg( bool sFullUpdate )
{
	// We have to send EVERY ConVar with CVARF_REPLICATED in a Full Update
	if ( sFullUpdate )
	{
		for ( uint32_t i = 0; i < Con_GetConVarCount(); i++ )
		{
			ConVarBase* cvarBase = Con_GetConVar( i );
			if ( !cvarBase )
				continue;

			// Only ConVars here, no ConCommands, that will be in another tab
			if ( typeid( *cvarBase ) != typeid( ConVar ) && cvarBase->aFlags & CVARF_REPLICATED )
				continue;

			gReplicatedCmds.emplace( cvarBase );
		}
	}
	
	// Only send if we actually have commands to send
	if ( gReplicatedCmds.empty() )
		return false;

	std::string command;

	// Join it all into one string
	for ( auto& cmd : gReplicatedCmds )
	{
		ConVar* cvar = static_cast< ConVar* >( cmd );
		command += vstring( "%s %s;", cvar->aName, cvar->GetChar() );
	}

	// Clear it
	gReplicatedCmds.clear();

	// Build Message

	return true;
}


// --------------------------------------------------------------------
// Helper Functions


SV_Client_t* SV_GetClient( ClientHandle_t sClient )
{
	auto it = gServerData.aClientIDs.find( sClient );
	if ( it == gServerData.aClientIDs.end() )
		return nullptr;
	
	return it->second;
}


void SV_SetCommandClient( SV_Client_t* spClient )
{
	gpCommandClient = spClient;
}


SV_Client_t* SV_GetCommandClient()
{
	return gpCommandClient;
}


// Entity SV_GetCommandClientEntity()
// {
// 	SV_Client_t* client = SV_GetCommandClient();
// 
// 	if ( !client )
// 	{
// 		Log_Error( gLC_Server, "SV_GetCommandClientEntity(): No Command Client currently set!\n" );
// 		return CH_ENT_INVALID;
// 	}
// 
// 	return client->aEntity;
// }
// 
// 
// SV_Client_t* SV_GetClientFromEntity( Entity sEntity )
// {
// 	// oh boy
// 	for ( SV_Client_t& client : gServerData.aClients )
// 	{
// 		if ( client.aEntity == sEntity )
// 			return &client;
// 	}
// 
// 	Log_ErrorF( gLC_Server, "SV_GetClientFromEntity(): Failed to find entity attached to a client! (Entity %zd)\n", sEntity );
// 	return nullptr;
// }
// 
// 
// Entity SV_GetPlayerEntFromIndex( size_t sIndex )
// {
// 	if ( gServerData.aClients.empty() || sIndex > gServerData.aClients.size() )
// 		return CH_ENT_INVALID;
// 
// 	return gServerData.aClients[ sIndex ].aEntity;
// }
// 
// 
// Entity SV_GetPlayerEnt( ClientHandle_t sClient )
// {
// 	if ( gServerData.aClientIDs.empty() )
// 		return CH_ENT_INVALID;
// 	
// 	auto it = gServerData.aClientIDs.find( sClient );
// 	if ( it == gServerData.aClientIDs.end() )
// 		return CH_ENT_INVALID;
// 	
// 	return it->second->aEntity;
// }


void SV_PrintStatus()
{
	Log_MsgF( "Hosting on Port %s\n", gServerPort );
	Log_MsgF( "Map: %s\n", MapManager_GetMapPath().data() );
	Log_MsgF( "%zd Players Currently on Server\n", gServerData.aClients.size() );
}

