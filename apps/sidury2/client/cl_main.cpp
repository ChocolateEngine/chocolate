#include "main.h"
#include "game_shared.h"
#include "cl_main.h"
#include "input_system.h"
#include "map_manager.h"
#include "network/net_main.h"

#include "igui.h"
#include "iinput.h"
#include "render/irender.h"

//
// The Client, always running unless a dedicated server
// 

LOG_REGISTER_CHANNEL2( Client, LogColor::White );

EClientState                      gClientState = EClientState_Idle;
CL_ServerData_t                   gClientServerData;

// NEW_CVAR_FLAG( CVARF_CLIENT );


CONVAR_CMD_EX( cl_username, "greg", CVARF_ARCHIVE, "Your Username" )
{
	if ( cl_username.GetValue().size() <= CH_MAX_USERNAME_LEN )
		return;

	Log_WarnF( gLC_Client, "Username is too long (%zd chars), max is %d chars\n", cl_username.GetValue().size(), CH_MAX_USERNAME_LEN );
	cl_username.SetValue( prevString );
}


CONVAR_CMD_EX( cl_username_use_steam, 1, CVARF_ARCHIVE, "Use username from steam instead of what cl_username contains" )
{
	// TODO: callback to send a username change to a server if we are connected to one
}


void CenterMouseOnScreen()
{
	int w, h;
	SDL_GetWindowSize( render->GetWindow(), &w, &h );
	SDL_WarpMouseInWindow( render->GetWindow(), w / 2, h / 2 );
}


bool CL_Init()
{
	return true;
}


void CL_Shutdown()
{
	CL_Disconnect();
} 


void CL_Update( float frameTime )
{
	PROF_SCOPE();

	Game_SetCommandSource( ECommandSource_Server );

	switch ( gClientState )
	{
		default:
		case EClientState_Idle:
			break;

		case EClientState_WaitForAccept:
		case EClientState_WaitForFullUpdate:
		{
			// Recieve Server Info first
			CL_GetServerMessages();
			break;
		}

		case EClientState_Connecting:
		{
			CL_GetServerMessages();

			gClientState = EClientState_Connected;

			// Tell the server we've connected

			break;
		}

		case EClientState_Connected:
		{
			// Process Stuff from server
			CL_GetServerMessages();

			// if ( !Game_IsPaused() && input->WindowHasFocus() && !CL_IsMenuShown() )
			// 	players.DoMouseLook( gLocalPlayer );

			CL_UpdateUserCmd();

			CL_GameUpdate( frameTime );

			// Send Console Commands
			CL_SendConVars();

			// Send UserCmd
			CL_SendUserCmd();

			break;
		}
	}

	// Update Entity and Component States

	if ( CL_IsMenuShown() )
	{
		CL_DrawMainMenu();
	}

	Game_SetCommandSource( ECommandSource_Console );
}


void CL_GameUpdate( float frameTime )
{
	PROF_SCOPE();

	CL_UpdateMenuShown();

	// Check connection timeout
	// if ( ( cl_timeout_duration - cl_timeout_threshold ) > ( gClientTimeout ) )
	// {
	// 	// Show Connection Warning
	// 	gui->DebugMessage( "CONNECTION PROBLEM - %.3f SECONDS LEFT\n", gClientTimeout );
	// }

	// Entity_InitCreatedComponents();
	// Entity_UpdateSystems();
	// 
	// players.UpdateLocalPlayer();

	if ( input->WindowHasFocus() && !CL_IsMenuShown() )
	{
		CenterMouseOnScreen();
	}
}


const char* CL_GetUserName()
{
	return cl_username;
}


