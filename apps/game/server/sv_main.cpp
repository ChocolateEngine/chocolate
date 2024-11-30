#include "sv_main.h"
#include "game_shared.h"
#include "main.h"
#include "mapmanager.h"
#include "entity/entity.h"
#include "player.h"
#include "steam.h"

#include "testing.h"
#include "igui.h"

#include <unordered_set>

//
// The Server, only runs if the engine is a dedicated server, or hosting on the client
//

LOG_CHANNEL_REGISTER( Server, ELogColor_Green );

static const char* gServerPort = args_register( "41628", "Test Server Port", "--port" );

CONVAR_STRING( sv_server_name, "taco", CVARF_SERVER | CVARF_ARCHIVE, "Server Name" );
CONVAR_FLOAT( sv_client_timeout, 30.f, CVARF_SERVER | CVARF_ARCHIVE );
CONVAR_BOOL( sv_client_timeout_enable, 1, CVARF_SERVER | CVARF_ARCHIVE );
CONVAR_BOOL( sv_pause, 0, CVARF_SERVER, "Pauses the Server Code" );

CONVAR_INT_CMD( sv_max_clients, 32, CVARF_SERVER, "Max Clients the Server Allows" )
{
	if ( newValue > CH_MAX_CLIENTS )
	{
		Log_WarnF( gLC_Server, "Can't go over max internal client limit of %d\n", CH_MAX_CLIENTS );
		newValue = CH_MAX_CLIENTS;
	}
	else if ( newValue < gServerData.aClients.size() )
	{
		Log_WarnF( gLC_Server, "Can't reduce max client limit less than the amount of clients connected (%zd connected)\n", gServerData.aClients.size() );
		newValue = gServerData.aClients.size();
	}
	else if ( newValue < 1 )
	{
		Log_WarnF( gLC_Server, "Can't set max clients less than 1\n" );
		newValue = 1;
	}
}

static std::unordered_set< std::string_view > gReplicatedCmds;

bool CvarFReplicatedCallback( const std::string& sName, const std::vector< std::string >& args, const std::string& fullCommand )
{
	if ( args.empty() )
		return true;

	ConVarData_t* cvarData = Con_GetConVarData( sName.data() );

	if ( !cvarData )
	{
		Log_ErrorF( gLC_Server, "ConVar for CVARF_REPLICATED not found, may be a ConCommand: \"%s\"\n", sName.data() );
		return true;
	}

	// If were hosting a server, the message has to be from the console
	if ( SV_IsHosting() && Game_GetCommandSource() == ECommandSource_Console )
	{
		// Add this to a vector of convars to send values to the client
		gReplicatedCmds.emplace( sName );
		return true;
	}
	// The Message Has to be from the server otherwise
	else if ( Game_GetCommandSource() == ECommandSource_Server )
	{
		return true;
	}

	Log_ErrorF( gLC_Server, "Can't change Server Replicated ConVar if you're not the host! - \"%s\"\n", sName.data() );
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


int SV_Client_t::WriteFlatBuffer( flatbuffers::FlatBufferBuilder& srBuilder )
{
	return Net_WriteFlatBuffer( gServerSocket, aAddr, srBuilder );
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
			Entity_DeleteEntity( client.aEntity );

			// Remove this client from the list
			SV_FreeClient( client );
			i--;
			continue;
		}
	}

	SV_ProcessSocketMsgs();
	
	// Main game loop
	SV_GameUpdate( frameTime );

	// Send updated data to clients
	std::vector< flatbuffers::FlatBufferBuilder > messages( gReplicatedCmds.size() ? 3 : 2 );

	bool msgFailed = false;
	msgFailed |= !SV_BuildServerMsg( messages[ 0 ], EMsgSrc_Server_EntityList, false );
	msgFailed |= !SV_BuildServerMsg( messages[ 1 ], EMsgSrc_Server_ComponentList, false );

	if ( gReplicatedCmds.size() )
		msgFailed |= !SV_BuildServerMsg( messages[ 2 ], EMsgSrc_Server_ConVar, false );

	int writeSize = 0;

	if ( !msgFailed )
		writeSize = SV_BroadcastMsgs( messages );

	// Check to see if anyone needs a full update
	if ( gServerData.aClientsFullUpdate.size() )
	{
		// Build a Full Update and send it to all of them
		std::vector< flatbuffers::FlatBufferBuilder > messages( 3 );
		bool                                          msgFailed = false;

		msgFailed |= !SV_BuildServerMsg( messages[ 0 ], EMsgSrc_Server_EntityList, true );
		msgFailed |= !SV_BuildServerMsg( messages[ 1 ], EMsgSrc_Server_ComponentList, true );
		msgFailed |= !SV_BuildServerMsg( messages[ 2 ], EMsgSrc_Server_ConVar, true );

		int fullUpdateSize = 0;

		if ( !msgFailed )
			fullUpdateSize = SV_BroadcastMsgsToSpecificClients( messages, gServerData.aClientsFullUpdate );

		Log_DevF( gLC_Server, 1, "Writing Full Update to Clients: %d bytes\n", fullUpdateSize );

		gServerData.aClientsFullUpdate.clear();
	}

	// Update Entity and Component States after everything is processed
	Entity_UpdateStates();

	// if ( Game_IsClient() )
	{
		Log_DevF( gLC_Server, 2, "Data Written to Each Client: %d bytes", writeSize );
	}

	gReplicatedCmds.clear();

	Game_SetCommandSource( ECommandSource_Console );
}


void SV_GameUpdate( float frameTime )
{
	PROF_SCOPE();

	Entity_InitCreatedComponents();

	MapManager_Update();

	players.Update( frameTime );

	Phys_Simulate( GetPhysEnv(), frameTime );

	Entity_UpdateSystems();

	// Update player positions after physics simulation
	// NOTE: This probably needs to be done for everything with physics
	for ( auto& player : players.aEntities )
	{
		players.apMove->UpdatePosition( player );
	}
}


bool SV_StartServer()
{
	SV_StopServer();

	// Create Server Physics and Entity System
	if ( !Entity_Init() )
	{
		Log_ErrorF( "Failed to Create Server Entity System\n" );
		return false;
	}

	Phys_CreateEnv();

	gServerSocket = Net_OpenSocket( gServerPort );

	if ( gServerSocket == CH_INVALID_SOCKET )
	{
		Log_Error( gLC_Server, "Failed to Open Test Server\n" );
		return false;
	}

	players.Init();

	gServerData.aActive = true;
	return true;
}


void SV_StopServer()
{
	for ( auto& client : gServerData.aClients )
	{
		SV_SendDisconnect( client );
	}

	MapManager_CloseMap();

	Entity_Shutdown();

	Phys_DestroyEnv();

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


int SV_BroadcastMsgsToSpecificClients( std::vector< flatbuffers::FlatBufferBuilder >& srMessages, const ChVector< SV_Client_t* >& srClients )
{
	PROF_SCOPE();

	int writeSize = 0;

	for ( size_t i = 0; i < srMessages.size(); i++ )
	{
		writeSize += srMessages[ i ].GetSize();
	}

	for ( auto client : srClients )
	{
		if ( !client )
			continue;

		// Kind of a hack
		if ( client->aState != ESV_ClientState_Connected && client->aState != ESV_ClientState_Connecting )
			continue;

		for ( size_t arrayIndex = 0; arrayIndex < srMessages.size(); arrayIndex++ )
		{
			int write = client->WriteFlatBuffer( srMessages[ arrayIndex ] );

			// If we failed to write, disconnect them?
			if ( write == 0 )
			{
				Log_ErrorF( gLC_Server, "Failed to write network data to client, marking client as disconnected: %s\n", Net_ErrorString() );
				client->aState = ESV_ClientState_Disconnected;
				break;
			}
		}
	}

	return writeSize;
}


int SV_BroadcastMsgs( std::vector< flatbuffers::FlatBufferBuilder >& srMessages )
{
	PROF_SCOPE();

	int writeSize = 0;

	for ( size_t i = 0; i < srMessages.size(); i++ )
	{
		writeSize += srMessages[ i ].GetSize();
	}

	for ( auto& client : gServerData.aClients )
	{
		// Kind of a hack
		if ( client.aState != ESV_ClientState_Connected && client.aState != ESV_ClientState_Connecting )
			continue;

		for ( size_t arrayIndex = 0; arrayIndex < srMessages.size(); arrayIndex++ )
		{
			int write = client.WriteFlatBuffer( srMessages[ arrayIndex ] );

			// If we failed to write, disconnect them?
			if ( write == 0 )
			{
				Log_ErrorF( gLC_Server, "Failed to write network data to client, marking client as disconnected: %s\n", Net_ErrorString() );
				client.aState = ESV_ClientState_Disconnected;
				break;
			}
		}
	}

	return writeSize;
}


int SV_BroadcastMsg( flatbuffers::FlatBufferBuilder& srMessage )
{
	PROF_SCOPE();

	for ( auto& client : gServerData.aClients )
	{
		// Kind of a hack
		if ( client.aState != ESV_ClientState_Connected && client.aState != ESV_ClientState_Connecting )
			continue;

		int write = client.WriteFlatBuffer( srMessage );

		// If we failed to write, disconnect them?
		if ( write == 0 )
		{
			Log_ErrorF( gLC_Server, "Failed to write network data to client, marking client as disconnected: %s\n", Net_ErrorString() );
			client.aState = ESV_ClientState_Disconnected;
		}
	}

	return srMessage.GetSize();
}


// hi im a server here's a taco - agrimar
void SV_BuildServerInfo( flatbuffers::FlatBufferBuilder& srMessage )
{
	Log_DevF( gLC_Server, 1, "Building Server Info\n" );

	auto serverName = srMessage.CreateString( sv_server_name );
	auto mapName    = srMessage.CreateString( MapManager_GetMapPath().data() );

	auto serverInfo = CreateNetMsg_ServerInfo( srMessage, serverName, gServerData.aClients.size(), sv_max_clients, mapName );
	srMessage.Finish( serverInfo );
}


bool SV_BuildServerMsg( flatbuffers::FlatBufferBuilder& srBuilder, EMsgSrc_Server sSrcType, bool sFullUpdate )
{
	PROF_SCOPE();

	flatbuffers::FlatBufferBuilder messageBuilder;
	bool                           wroteData = false;

	switch ( sSrcType )
	{
		case EMsgSrc_Server_Disconnect:
		{
			auto string        = messageBuilder.CreateString( "Saving chunks." );
			auto disconnectMsg = CreateNetMsg_Disconnect( messageBuilder, string );
			messageBuilder.Finish( disconnectMsg );
			wroteData = true;
			break;
		}
		case EMsgSrc_Server_Paused:
		{
			auto pausedMsg = CreateNetMsg_Paused( messageBuilder, Game_IsPaused() );
			messageBuilder.Finish( pausedMsg );
			wroteData = true;
			break;
		}
		case EMsgSrc_Server_ServerInfo:
		{
			SV_BuildServerInfo( messageBuilder );
			wroteData = true;
			break;
		}
		case EMsgSrc_Server_ConVar:
		{
			wroteData = SV_BuildConVarMsg( messageBuilder );
			break;
		}
		case EMsgSrc_Server_EntityList:
		{
			Entity_WriteEntityUpdates( messageBuilder );
			wroteData = true;
			//Log_DevF( gLC_Server, 2, "Sending ENTITY_LIST to Clients\n" );
			break;
		}
		case EMsgSrc_Server_ComponentList:
		{
			Entity_WriteComponentUpdates( messageBuilder, sFullUpdate );
			wroteData = true;
			//Log_DevF( gLC_Server, 2, "Sending COMPONENT_LIST to Clients\n" );
			break;
		}
		default:
		{
			Log_ErrorF( gLC_Server, "Invalid Server Source Message Type: %zd\n", sSrcType );
			return false;
		}
	}

	flatbuffers::Offset< flatbuffers::Vector< u8 > > dataVector{};

	if ( wroteData )
		dataVector = srBuilder.CreateVector( messageBuilder.GetBufferPointer(), messageBuilder.GetSize() );

	MsgSrc_ServerBuilder serverMsg( srBuilder );
	serverMsg.add_type( sSrcType );

	if ( wroteData )
		serverMsg.add_data( dataVector );

	srBuilder.Finish( serverMsg.Finish() );

	return true;
}


#if 0
void SV_BuildUpdatedData( bool sFullUpdate )
{
	// Send updated data to clients
	std::vector< flatbuffers::FlatBufferBuilder > messages( 2 );
	std::vector< kj::Array< capnp::word > >    arrays( 2 );

	SV_BuildServerMsg( messages[ 0 ], EMsgSrcServer::ENTITY_LIST, sFullUpdate );
	SV_BuildServerMsg( messages[ 1 ], EMsgSrcServer::COMPONENT_LIST, sFullUpdate );

	arrays[ 0 ]   = capnp::messageToFlatArray( messages[ 0 ] );
	arrays[ 1 ]   = capnp::messageToFlatArray( messages[ 1 ] );

	int writeSize = 0;

	for ( auto& client : gServerData.aClients )
	{
		if ( client.aState != ESV_ClientState_Connected )
			continue;

		writeSize = 0;

		for ( size_t arrayIndex = 0; arrayIndex < arrays.size(); arrayIndex++ )
		{
			int write = client.Write( arrays[ arrayIndex ].asChars().begin(), arrays[ arrayIndex ].size() * sizeof( capnp::word ) );

			// If we failed to write, disconnect them?
			if ( write == 0 )
			{
				Log_ErrorF( gLC_Server, "Failed to write network data to client, marking client as disconnected: %s\n", Net_ErrorString() );
				client.aState = ESV_ClientState_Disconnected;
			}
			else
			{
				writeSize += write;
			}
		}
	}
}
#endif


bool SV_SendMessageToClient( SV_Client_t& srClient, flatbuffers::FlatBufferBuilder& srMessage )
{
	int write = Net_WriteFlatBuffer( gServerSocket, srClient.aAddr, srMessage );

	if ( write < 1 )
	{
		// Failed to write to client, disconnect them
		srClient.aState = ESV_ClientState_Disconnected;
		Log_MsgF( gLC_Server, "Disconnecting Client: \"%s\"\n", srClient.name.c_str() );
		return false;
	}

	return true;
}


void SV_SendDisconnect( SV_Client_t& srClient )
{
	flatbuffers::FlatBufferBuilder message;

	bool msgFailed = !SV_BuildServerMsg( message, EMsgSrc_Server_Disconnect );

	if ( SV_SendMessageToClient( srClient, message ) )
	{
		srClient.aState = ESV_ClientState_Disconnected;
		Log_MsgF( gLC_Server, "Disconnecting Client: \"%s\"\n", srClient.name.c_str() );
	}
}


void SV_HandleMsg_ClientInfo( SV_Client_t& srClient, const NetMsg_ClientInfo* spMessage )
{
	if ( !spMessage )
		return;

	if ( spMessage->name() )
		srClient.name = spMessage->name()->str();

	srClient.aSteamID = spMessage->steam_id();
}


void SV_HandleMsg_UserCmd( SV_Client_t& srClient, const NetMsg_UserCmd* spMessage )
{
	if ( !spMessage )
		return;

	//Log_DevF( gLC_Server, 2, "Handling Message USER_CMD from Client \"%s\"\n", srClient.name.c_str() );

	NetHelper_ReadVec3( spMessage->angles(), srClient.aUserCmd.aAng );

	srClient.aUserCmd.aButtons    = spMessage->buttons();
	srClient.aUserCmd.aFlashlight = spMessage->flashlight();
	srClient.aUserCmd.aMoveType   = static_cast< EPlayerMoveType >( spMessage->move_type() );
}


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


template< typename T >
inline const T* SV_ReadMsg( EMsgSrc_Client sMsgType, flatbuffers::Verifier& srVerifier, const flatbuffers::Vector< u8 >* srMsgData )
{
	auto msg = flatbuffers::GetRoot< T >( srMsgData->data() );

	if ( !msg->Verify( srVerifier ) )
	{
		Log_WarnF( gLC_Server, "Message Data is not Valid: %s\n", CL_MsgToString( sMsgType ) );
		return nullptr;
	}

	return msg;
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

		flatbuffers::Verifier verifyMsg( reinterpret_cast< u8* >( data.data() ), data.size_bytes() );
		const MsgSrc_Client* clientMsg = flatbuffers::GetRoot< MsgSrc_Client >( data.data() );

		if ( !clientMsg->Verify( verifyMsg ) )
		{
			Log_Warn( gLC_Server, "Message Data is not Valid\n" );
			continue;
		}

		// Read the message sent from the client
		SV_ProcessClientMsg( *client, clientMsg );
	}
}


void SV_ProcessClientMsg( SV_Client_t& srClient, const MsgSrc_Client* spMessage )
{
	PROF_SCOPE();

	// TODO: move this connection timer elsewhere
#if 0
	// Timer here for each client to make sure they are connected
	// anything received from the client will reset their connection timer
	if ( len <= 0 )
	{
		// They haven't sent anything in a while, disconnect them
		if ( sv_client_timeout_enable && Game_GetCurTime() > srClient.aTimeout )
			SV_SendDisconnect( srClient );

		srClient.aTimeout -= gFrameTime;

		return;
	}
#endif

	// Reset the connection timer
	srClient.aTimeout = Game_GetCurTime() + sv_client_timeout;

	if ( !spMessage )
	{
		// Log_Warn();
		return;
	}

	// Read the message sent from the client
	EMsgSrc_Client msgType = spMessage->type();

	// First, check types without any data attached to them
	switch ( msgType )
	{
		// Client is Disconnecting
		case EMsgSrc_Client_Disconnect:
		{
			srClient.aState = ESV_ClientState_Disconnected;
			return;
		}

		case EMsgSrc_Client_FullUpdate:
		{
			// They want a full update, add them to the full update list
			gServerData.aClientsFullUpdate.push_back( &srClient );
			return;
		}
		case EMsgSrc_Client_ConnectFinish:
		{
			if ( srClient.aState == ESV_ClientState_Connecting )
				SV_ConnectClientFinish( srClient );

			return;
		}

		default:
			break;
	}

	// Next, check messages that are expected to have message data

	auto msgData = spMessage->data();

	if ( CH_IF_ASSERT( msgData && msgData->size() ) )
	{
		Log_ErrorF( gLC_Server, "Invalid Message with no data attached - %s\n", CL_MsgToString( msgType ) );
		return;
	}

	flatbuffers::Verifier verifyMsg( msgData->data(), msgData->size() );

	switch ( msgType )
	{
		case EMsgSrc_Client_ClientInfo:
		{
			auto clientMsg = SV_ReadMsg< NetMsg_ClientInfo >( msgType, verifyMsg, msgData );
			if ( clientMsg )
			{
				SV_HandleMsg_ClientInfo( srClient, clientMsg );

				// Check if we were waiting for this
				if ( srClient.aState == ESV_ClientState_WaitForClientInfo )
				{
					// Add the playerInfo Component
					void* playerInfo = Entity_AddComponent( srClient.aEntity, "playerInfo" );

					if ( playerInfo == nullptr )
					{
						Log_ErrorF( gLC_Server, "Failed to Connect Client - Failed to create a playerInfo component: \"%s\"\n", Net_AddrToString( srClient.aAddr ) );
						Entity_DeleteEntity( srClient.aEntity );
						srClient.aState = ESV_ClientState_Disconnected;
						return;
					}

					// We were, now set them to connecting and wait for EMsgSrc_Client_ConnectFinish
					srClient.aState = ESV_ClientState_Connecting;

					// Give them the server info
					flatbuffers::FlatBufferBuilder message;
					SV_BuildServerMsg( message, EMsgSrc_Server_ServerInfo );
					SV_SendMessageToClient( srClient, message );

					// We also send them a full update
					gServerData.aClientsFullUpdate.push_back( &srClient );
				}
			}

			break;
		}

		case EMsgSrc_Client_ConVar:
		{
			if ( srClient.aState == ESV_ClientState_WaitForClientInfo )
				break;

			SV_SetCommandClient( &srClient );
			auto clientMsg = SV_ReadMsg< NetMsg_ConVar >( msgType, verifyMsg, msgData );

			if ( clientMsg && clientMsg->command() )
			{
				Game_ExecCommandsSafe( ECommandSource_Client, clientMsg->command()->string_view() );
			}

			SV_SetCommandClient( nullptr );  // Clear it
			break;
		}

		case EMsgSrc_Client_UserCmd:
		{
			if ( srClient.aState == ESV_ClientState_WaitForClientInfo )
				break;

			if ( auto clientMsg = SV_ReadMsg< NetMsg_UserCmd >( msgType, verifyMsg, msgData ) )
				SV_HandleMsg_UserCmd( srClient, clientMsg );

			break;
		}

		default:
			Log_WarnF( gLC_Server, "Unknown Message Type from Client: %s\n", CL_MsgToString( msgType ) );
			break;
	}
}


SV_Client_t* SV_AllocateClient()
{
	if ( gServerData.aClients.size() >= sv_max_clients )
		return nullptr;

	// Allocate a new client
	SV_Client_t&   client = gServerData.aClients.emplace_back();

	// Generate a random number to use as a ch_handle_t
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
		//Log_MsgF( gLC_Server, "Recieved Conn Connected: \"%s\"\n", srClient.name.c_str() );
		return;
	}

	if ( srClient.aSteamID && IsSteamLoaded() )
	{
		steam->RequestProfileName( srClient.aSteamID );
	}

	srClient.aState = ESV_ClientState_Connected;

	gServerData.aClientsConnecting.erase( &srClient );
	
	// They just got here, so send them a full update
	gServerData.aClientsFullUpdate.push_back( &srClient );

	Log_MsgF( gLC_Server, "Client Connected: \"%s\"\n", srClient.name.c_str() );

	// Spawn the player in!
	players.Spawn( srClient.aEntity );

	// Tell the clients someone else connected
	flatbuffers::FlatBufferBuilder message;
	SV_BuildServerMsg( message, EMsgSrc_Server_ServerInfo );
	SV_BroadcastMsg( message );

	// Reset the connection timer
	srClient.aTimeout = Game_GetCurTime() + sv_client_timeout;
}


void SV_ConnectClient( ch_sockaddr& srAddr, ChVector< char >& srData )
{
	PROF_SCOPE();

	// Get Client Info
	//NetMsg_ClientConnect       msgClientConnect();

	flatbuffers::Verifier       verifyMsg( (u8*)srData.data(), srData.size() );
	const NetMsg_ClientConnect* clientMsg = flatbuffers::GetRoot< NetMsg_ClientConnect >( srData.data() );

	// This is invalid for some reason?
	// if ( !clientMsg->Verify( verifyMsg ) );
	// 	return;

	// First thing's first, make sure our protocol is the same
	if ( clientMsg->protocol() != ESiduryProtocolVer_Value )
	{
		Log_ErrorF( gLC_Server, "Failed to start Connecting Client - Protocol Difference: %zd, Expected %zd\n", clientMsg->protocol(), ESiduryProtocolVer_Value );

		// Try to tell them
		// flatbuffers::FlatBufferBuilder message;
		// NetMsgDisconnect::Builder   disconnectMsg = message.initRoot< NetMsgDisconnect >();
		// 
		// disconnectMsg.setReason( vstring( "Invalid Protocol, Got %zd, Exptected %zd", clientInfoRead.getProtocol(), ESiduryProtocolVer_Value ) );
		// 
		// // We don't care if it fails
		// Net_WriteFlatBuffer( gServerSocket, srAddr, message );
		return;
	}

	// Make an entity for them
	Entity      entity  = Entity_CreateEntity();

	if ( entity == CH_ENT_INVALID )
	{
		Log_ErrorF( gLC_Server, "Failed to start Connecting Client - Failed to create an entity for them: \"%s\"\n", Net_AddrToString( srAddr ) );
		return;
	}

	// Add them to the client list for the playerInfo component
	SV_Client_t* client = SV_AllocateClient();

	if ( !client )
	{
		Log_ErrorF( gLC_Server, "Failed to Connect Client - At Max Players Limit: \"%s\"\n", Net_AddrToString( srAddr ) );
		Entity_DeleteEntity( entity );
		return;
	}

	client->aAddr   = srAddr;
	client->aState  = ESV_ClientState_WaitForClientInfo;
	client->aEntity = entity;

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
	std::vector< flatbuffers::FlatBufferBuilder > builders( 2 );

	// Build Server Connect Response
	{
		flatbuffers::FlatBufferBuilder messageBuilder;

		NetMsg_ServerConnectResponseBuilder serverResponseBuild( messageBuilder );
		serverResponseBuild.add_client_entity_id( entity );

		messageBuilder.Finish( serverResponseBuild.Finish() );

		auto                 vector = builders[ 0 ].CreateVector( messageBuilder.GetBufferPointer(), messageBuilder.GetSize() );

		MsgSrc_ServerBuilder serverMsg( builders[ 0 ] );
		serverMsg.add_type( EMsgSrc_Server_ConnectResponse );
		serverMsg.add_data( vector );
		builders[ 0 ].Finish( serverMsg.Finish() );
	}

	// SV_BuildServerMsg( builders[ 1 ], EMsgSrc_Server_ServerInfo, true );

	// send them this information on the listen socket, and with the port, the client and switch to that one for their connection
	int write = Net_WriteFlatBuffer( gServerSocket, srAddr, builders[ 0 ] );

	if ( write > 0 )
	{
		gServerData.aClientsConnecting.push_back( client );
	}
	else
	{
		// drop (kick) them if we failed to write a message to them
		client->aState = ESV_ClientState_Disconnected;
		Log_ErrorF( gLC_Server, "Failed to Connect Client - Failed to send server info to them: \"%s\"\n", Net_AddrToString( srAddr ) );
		return;
	}

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


void SV_SendConVar( std::string_view sConVar )
{
	gReplicatedCmds.emplace( sConVar );
}


bool SV_BuildConVarMsg( flatbuffers::FlatBufferBuilder& srMessage, bool sFullUpdate )
{
	// We have to send EVERY ConVar with CVARF_REPLICATED in a Full Update
	if ( sFullUpdate )
	{
		for ( const auto& [ cvarName, cvarData ] : Con_GetConVarMap() )
		{
			// Only ConVars here, no ConCommands, that will be in another tab
			if ( cvarData->aFlags & CVARF_REPLICATED )
				continue;

			gReplicatedCmds.emplace( cvarName );
		}
	}
	
	// Only send if we actually have commands to send
	if ( gReplicatedCmds.empty() )
		return false;

	std::string command;

	// Join it all into one string
	for ( auto& cmd : gReplicatedCmds )
	{
		command += vstring( "%s %s;", cmd.data(), Con_GetConVarValueStr( cmd.data() ).data() );
	}

	// Clear it
	gReplicatedCmds.clear();

	auto                 stringCommand = srMessage.CreateString( command );
	NetMsg_ConVarBuilder cvarRoot( srMessage );
	cvarRoot.add_command( stringCommand );
	srMessage.Finish( cvarRoot.Finish() );

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


Entity SV_GetCommandClientEntity()
{
	SV_Client_t* client = SV_GetCommandClient();

	if ( !client )
	{
		Log_Error( gLC_Server, "SV_GetCommandClientEntity(): No Command Client currently set!\n" );
		return CH_ENT_INVALID;
	}

	return client->aEntity;
}


SV_Client_t* SV_GetClientFromEntity( Entity sEntity )
{
	// oh boy
	for ( SV_Client_t& client : gServerData.aClients )
	{
		if ( client.aEntity == sEntity )
			return &client;
	}

	Log_ErrorF( gLC_Server, "SV_GetClientFromEntity(): Failed to find entity attached to a client! (Entity %zd)\n", sEntity );
	return nullptr;
}


Entity SV_GetPlayerEntFromIndex( size_t sIndex )
{
	if ( gServerData.aClients.empty() || sIndex > gServerData.aClients.size() )
		return CH_ENT_INVALID;

	return gServerData.aClients[ sIndex ].aEntity;
}


Entity SV_GetPlayerEnt( ClientHandle_t sClient )
{
	if ( gServerData.aClientIDs.empty() )
		return CH_ENT_INVALID;
	
	auto it = gServerData.aClientIDs.find( sClient );
	if ( it == gServerData.aClientIDs.end() )
		return CH_ENT_INVALID;
	
	return it->second->aEntity;
}


void SV_PrintStatus()
{
	Log_MsgF( "Hosting on Port %s\n", gServerPort );
	Log_MsgF( "Map: %s\n", MapManager_GetMapPath().data() );
	Log_MsgF( "%zd Players Currently on Server\n", gServerData.aClients.size() );
}

