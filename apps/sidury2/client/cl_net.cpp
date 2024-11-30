#include "main.h"
#include "game_shared.h"
#include "cl_main.h"
#include "input_system.h"
#include "map_manager.h"
#include "network/net_main.h"

// =======================================================================
// Client Networking
// =======================================================================

LOG_CHANNEL2( Client );

// How much time we have until we give up connecting if the server doesn't respond anymore
static double                     gClientConnectTimeout = 0.f;
static float                      gClientTimeout        = 0.f;

extern EClientState               gClientState;

// Console Commands to send to the server to process, like noclip
static std::vector< std::string > gCommandsToSend;
UserCmd_t                         gClientUserCmd{};

std::vector< CL_Client_t >        gClClients;


CONVAR( cl_connect_timeout_duration, 30.f, "How long we will wait for the server to send us connection information" );
CONVAR( cl_timeout_duration, 120.f, "How long we will wait for the server to start responding again before disconnecting" );
CONVAR( cl_timeout_threshold, 4.f, "If the server doesn't send anything after this amount of time, show the connection problem message" );

CONVAR( in_forward, 0, CVARF( INPUT ) );
CONVAR( in_back, 0, CVARF( INPUT ) );
CONVAR( in_left, 0, CVARF( INPUT ) );
CONVAR( in_right, 0, CVARF( INPUT ) );

CONVAR( in_duck, 0, CVARF( INPUT ) );
CONVAR( in_sprint, 0, CVARF( INPUT ) );
CONVAR( in_jump, 0, CVARF( INPUT ) );
CONVAR( in_zoom, 0, CVARF( INPUT ) );
CONVAR( in_flashlight, 0, CVARF( INPUT ) );


static Socket_t gClientSocket = CH_INVALID_SOCKET;
ch_sockaddr     gClientAddr;


CONCMD( cl_request_full_update )
{
	CL_SendFullUpdateRequest();
}


CONCMD( connect )
{
	if ( args.empty() )
	{
		Log_Msg( gLC_Client, "Type in an IP address after the word connect\n" );
		return;
	}

	if ( Game_GetCommandSource() == ECommandSource_Server )
	{
		// the server can only tell us to connect to localhost
		if ( args[ 0 ] != "localhost" )
		{
			Log_Msg( "connect called from server?\n" );
			return;
		}
	}

	CL_Connect( args[ 0 ].data() );
}


void CL_Disconnect( bool sSendReason, const char* spReason )
{
	if ( gClientSocket != CH_INVALID_SOCKET )
	{
		if ( sSendReason )
		{
			// Tell the server we are disconnecting
		}

		Net_CloseSocket( gClientSocket );
		gClientSocket = CH_INVALID_SOCKET;
	}

	memset( &gClientAddr, 0, sizeof( gClientAddr ) );

	if ( gClientState != EClientState_Idle )
	{
		if ( spReason )
			Log_DevF( 1, "Setting Client State to Idle (Disconnecting) - %s\n", spReason );
		else
			Log_Dev( 1, "Setting Client State to Idle (Disconnecting)\n" );
	}

	gClientState          = EClientState_Idle;
	gClientConnectTimeout = 0.f;
}


// TODO: should only be platform specific, needs to have sockaddr abstracted
extern void Net_NetadrToSockaddr( const NetAddr_t* spNetAddr, struct sockaddr* spSockAddr );

void        CL_Connect( const char* spAddress )
{
	PROF_SCOPE();

#if 0
	// Make sure we are not connected to a server already
	CL_Disconnect();

	Log_MsgF( gLC_Client, "Connecting to \"%s\"\n", spAddress );

	gClientSocket     = Net_OpenSocket( "0" );
	NetAddr_t netAddr = Net_GetNetAddrFromString( spAddress );

	Net_NetadrToSockaddr( &netAddr, (struct sockaddr*)&gClientAddr );

	int connectRet = Net_Connect( gClientSocket, gClientAddr );

	if ( connectRet != 0 )
		return;

	int write = Net_Write( gClientSocket, gClientAddr, TODO_WRITE_A_CONNECT_MESSAGE );

	if ( write > 0 )
	{
		// Continue connecting in CL_Update()
		gClientState          = EClientState_WaitForAccept;
		gClientTimeout        = cl_connect_timeout_duration;
		gClientConnectTimeout = Game_GetCurTime() + cl_connect_timeout_duration;
	}
	else
	{
		CL_Disconnect();
	}
#endif
}


bool CL_SendConVarIfClient( std::string_view sName, const std::vector< std::string >& srArgs )
{
	// Send the command to the server
	CL_SendConVar( sName, srArgs );
	return true;
}


void CL_SendConVar( std::string_view sName, const std::vector< std::string >& srArgs )
{
	PROF_SCOPE();

	// We have to rebuild this command to a normal string, fun
	// Allocate a command to send with the command name
	std::string& newCmd = gCommandsToSend.emplace_back( sName.data() );

	// Add the command arguments
	for ( const std::string& arg : srArgs )
	{
		newCmd += " " + arg;
	}
}


void CL_SendConVars()
{
	PROF_SCOPE();

	// Only send if we actually have commands to send
	if ( gCommandsToSend.empty() )
		return;

	std::string command;

	// Join it all into one string
	for ( auto& cmd : gCommandsToSend )
		command += cmd + ";";

	// Clear it
	gCommandsToSend.clear();

	// CL_WriteMsgDataToServer( convarBuffer, EMsgSrc_Client_ConVar );
}


void CL_UpdateUserCmd()
{
	PROF_SCOPE();

	// Get the transform component from the camera entity on the local player, and get the angles from it

	// Reset Values
	gClientUserCmd.aButtons    = 0;
	gClientUserCmd.aFlashlight = false;
	gClientUserCmd.aAng        = {};

	// Don't update buttons if the menu is shown
	if ( CL_IsMenuShown() )
		return;

	if ( in_duck )
		gClientUserCmd.aButtons |= EBtnInput_Duck;

	else if ( in_sprint )
		gClientUserCmd.aButtons |= EBtnInput_Sprint;

	if ( in_forward ) gClientUserCmd.aButtons |= EBtnInput_Forward;
	if ( in_back ) gClientUserCmd.aButtons |= EBtnInput_Back;
	if ( in_left ) gClientUserCmd.aButtons |= EBtnInput_Left;
	if ( in_right ) gClientUserCmd.aButtons |= EBtnInput_Right;
	if ( in_jump ) gClientUserCmd.aButtons |= EBtnInput_Jump;
	if ( in_zoom ) gClientUserCmd.aButtons |= EBtnInput_Zoom;

	if ( in_flashlight == IN_CVAR_JUST_PRESSED )
		gClientUserCmd.aFlashlight = true;
}


void CL_BuildUserCmd()
{
	PROF_SCOPE();
}


void CL_SendUserCmd()
{
	PROF_SCOPE();
}


void CL_SendFullUpdateRequest()
{
	PROF_SCOPE();
}


void CL_SendMessageToServer()
{
}


void CL_GetServerMessages()
{
	PROF_SCOPE();

	while ( true )
	{
		// TODO: SETUP FRAGMENT COMPRESSION !!!!!!!!
		ChVector< char > data( 81920 );
		int              len = Net_Read( gClientSocket, data.data(), data.size(), &gClientAddr );

		if ( len <= 0 )
		{
			gClientTimeout -= gFrameTime;

			// The server hasn't sent anything in a while, so just disconnect
			if ( gClientTimeout < 0.0 )
			{
				Log_Msg( gLC_Client, "Disconnecting From Server - Timeout period expired\n" );
				CL_Disconnect();
			}

			return;
		}

		data.resize( len );

		// Reset the connection timer
		gClientTimeout                  = cl_timeout_duration;

		// Read the message sent from the server
	}
}


void CL_PrintStatus()
{
	// if ( gClientState == EClientState_Connected )
	// {
	// 	size_t playerCount = players.aEntities.size();
	// 
	// 	Log_MsgF( "Connected To %s\n", Net_AddrToString( gClientAddr ) );
	// 
	// 	Log_MsgF( "Map: %s\n", MapManager_GetMapPath().data() );
	// 	Log_MsgF( "%zd Players Currently on Server\n", playerCount );
	// }
	// else
	// {
	// 	Log_Msg( "Not Connected to or is Hosting any Server\n" );
	// }
}


CONCMD( status_cl )
{
	CL_PrintStatus();
}

