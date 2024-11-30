#include "game_shared.h"


#if CH_CLIENT
#include "../client/cl_main.h"
#else
#include "../server/sv_main.h"
#endif


NEW_CVAR_FLAG( CVARF_CL_EXEC );
NEW_CVAR_FLAG( CVARF_SV_EXEC );

NEW_CVAR_FLAG( CVARF_SERVER );

// ConVar value is enforced on clients
NEW_CVAR_FLAG( CVARF_REPLICATED );


static ECommandSource gGameCommandSource = ECommandSource_Client;


ECommandSource Game_GetCommandSource()
{
	return gGameCommandSource;
}


void Game_SetCommandSource( ECommandSource sSource )
{
	gGameCommandSource = sSource;
}


void Game_ExecCommandsSafe( ECommandSource sSource, std::string_view sCommand )
{
	PROF_SCOPE();

	std::string                commandName;
	std::vector< std::string > args;

	for ( size_t i = 0; i < sCommand.size(); i++ )
	{
		commandName.clear();
		args.clear();

		Con_ParseCommandLineEx( sCommand, commandName, args, i );
		str_lower( commandName );

		ConVarBase* cvarBase = Con_GetConVarBase( commandName );

		if ( !cvarBase )
		{
			// how did this happen?
			Log_ErrorF( "Game_ExecCommandsSafe(): Failed to find command \"%s\"\n", commandName.c_str() );
			continue;
		}

		ConVarFlag_t flags = cvarBase->GetFlags();

		// if the command is from the server and we are the client, make sure they can execute it
#if CH_CLIENT
		if ( sSource == ECommandSource_Server )
		{
			// The Convar must have one of these flags
			if ( !(flags & CVARF_SV_EXEC) && !(flags & CVARF_SERVER) && !(flags & CVARF_REPLICATED) )
			{
				Log_WarnF( "Server Tried Executing Command without flag to allow it: \"%s\"\n", commandName.c_str() );
				continue;
			}
		}
#else
		// if the command is from the client and we are the server, make sure they can execute it
		if ( sSource == ECommandSource_Client )
		{
			// The Convar must have this flag
			if ( !(flags & CVARF_CL_EXEC) )
			{
				Log_WarnF( "Client Tried Executing Command without flag to allow it: \"%s\"\n", commandName.c_str() );
				continue;
			}
		}
#endif

		Con_RunCommandArgs( commandName, args );
	}
}


