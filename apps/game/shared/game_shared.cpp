#include "game_shared.h"
#include "entity/entity.h"
#include "player.h"
#include "mapmanager.h"


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


// Convert a Client Source Message to String
const char* CL_MsgToString( EMsgSrc_Client sMsg )
{
	return EnumNameEMsgSrc_Client( sMsg );
}


// Convert a Server Source Message to String
const char* SV_MsgToString( EMsgSrc_Server sMsg )
{
	return EnumNameEMsgSrc_Server( sMsg );
}


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

	std::string                              commandName;
	std::string                              fullCommand;
	std::vector< std::string >               args;

	for ( size_t i = 0; i < sCommand.size(); i++ )
	{
		commandName.clear();
		args.clear();

		Con_ParseCommandLineEx( sCommand, commandName, args, fullCommand, i );
		str_lower( commandName );

		ConVarData_t* cvarData = Con_GetConVarData( commandName.data(), commandName.size() );

		if ( !cvarData )
		{
			// how did this happen?
			Log_ErrorF( "Game_ExecCommandsSafe(): Failed to find command \"%s\"\n", commandName.c_str() );
			continue;
		}

		// if the command is from the server and we are the client, make sure they can execute it
#if CH_CLIENT
		if ( sSource == ECommandSource_Server )
		{
			// The Convar must have one of these flags
			if ( !( cvarData->aFlags & CVARF_SV_EXEC ) && !( cvarData->aFlags & CVARF_SERVER ) && !( cvarData->aFlags & CVARF_REPLICATED ) )
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
			if ( !( cvarData->aFlags & CVARF_CL_EXEC ) )
			{
				Log_WarnF( "Client Tried Executing Command without flag to allow it: \"%s\"\n", commandName.c_str() );
				continue;
			}
		}
#endif

		Con_RunCommandArgs( commandName, args );
	}
}


void NetHelper_ReadVec2( const Vec2* spSource, glm::vec2& srVec )
{
	if ( !spSource )
		return;

	srVec.x = spSource->x();
	srVec.y = spSource->y();
}

void NetHelper_ReadVec3( const Vec3* spSource, glm::vec3& srVec )
{
	if ( !spSource )
		return;

	srVec.x = spSource->x();
	srVec.y = spSource->y();
	srVec.z = spSource->z();
}

void NetHelper_ReadVec4( const Vec4* spSource, glm::vec4& srVec )
{
	if ( !spSource )
		return;

	srVec.x = spSource->x();
	srVec.y = spSource->y();
	srVec.z = spSource->z();
	srVec.w = spSource->w();
}

void NetHelper_ReadQuat( const Quat* spSource, glm::quat& srQuat )
{
	if ( !spSource )
		return;

	srQuat.x = spSource->x();
	srQuat.y = spSource->y();
	srQuat.z = spSource->z();
	srQuat.w = spSource->w();
}

#if 0
void NetHelper_WriteVec3( Vec2Builder& srBuilder, const glm::vec2& srVec )
{
	srBuilder.add_x( srVec.x );
	srBuilder.add_y( srVec.y );
}

void NetHelper_WriteVec3( Vec3Builder& srBuilder, const glm::vec3& srVec )
{
	srBuilder.add_x( srVec.x );
	srBuilder.add_y( srVec.y );
	srBuilder.add_z( srVec.z );
}

void NetHelper_WriteVec4( Vec4Builder& srBuilder, const glm::vec4& srVec )
{
	srBuilder.add_x( srVec.x );
	srBuilder.add_y( srVec.y );
	srBuilder.add_z( srVec.z );
	srBuilder.add_z( srVec.w );
}
#endif

