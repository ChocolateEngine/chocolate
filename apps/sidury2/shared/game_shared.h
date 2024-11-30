#pragma once

#include "network/net_main.h"


struct Transform;
struct TransformSmall;
struct CCamera;
struct CPlayerMoveData;
enum class PlayerMoveType;


EXT_CVAR_FLAG( CVARF_CL_EXEC );
EXT_CVAR_FLAG( CVARF_SV_EXEC );

EXT_CVAR_FLAG( CVARF_SERVER );
EXT_CVAR_FLAG( CVARF_REPLICATED );

#define CVARF_SERVER_REPLICATED     CVARF_SERVER | CVARF_REPLICATED
#define CVARF_DEF_SERVER_REPLICATED CVARF( SERVER ) | CVARF( REPLICATED )

#define CVARF_CHEAT CVARF( CHEAT )


#if CH_CLIENT
	#define CH_MODULE_NAME "Client"

	#define GAME_CONCMD( name )                                           \
		void       name##_func( const std::vector< std::string >& args ); \
		ConCommand name##_cmd( "cl_" #name, name##_func );                \
		void       name##_func( const std::vector< std::string >& args )

	#define GAME_CONCMD_VA( name, ... )                            \
		void       name( const std::vector< std::string >& args ); \
		ConCommand name##_cmd( "cl_" #name, name, __VA_ARGS__ );   \
		void       name( const std::vector< std::string >& args )

	#define GAME_CONCMD_DROP( name, func )                         \
		void       name( const std::vector< std::string >& args ); \
		ConCommand name##_cmd( "cl_" #name, name, func );          \
		void       name( const std::vector< std::string >& args )

	#define GAME_CONCMD_DROP_VA( name, func, ... )                     \
		void       name( const std::vector< std::string >& args );     \
		ConCommand name##_cmd( "cl_" #name, name, __VA_ARGS__, func ); \
		void       name( const std::vector< std::string >& args )
#else
	#define CH_MODULE_NAME "Server"

	#define GAME_CONCMD( name ) \
		void       name##_func( const std::vector< std::string >& args ); \
		ConCommand name##_cmd( "sv_" #name, name##_func );                \
		void       name##_func( const std::vector< std::string >& args )

	#define GAME_CONCMD_VA( name, ... )                            \
		void       name( const std::vector< std::string >& args ); \
		ConCommand name##_cmd( "sv_" #name, name, __VA_ARGS__ );   \
		void       name( const std::vector< std::string >& args )

	#define GAME_CONCMD_DROP( name, func )                         \
		void       name( const std::vector< std::string >& args ); \
		ConCommand name##_cmd( "sv_" #name, name, func );          \
		void       name( const std::vector< std::string >& args )

	#define GAME_CONCMD_DROP_VA( name, func, ... )                     \
		void       name( const std::vector< std::string >& args );     \
		ConCommand name##_cmd( "sv_" #name, name, __VA_ARGS__, func ); \
		void       name( const std::vector< std::string >& args )
#endif


enum ECommandSource
{
	// Console Command came from the Console
	ECommandSource_Console,

	// Console Command was sent to us from the Client
	ECommandSource_Client,

	// Console Command was sent to us from the Server
	ECommandSource_Server,
};


// Keep in sync with NetMsgUserCmd in sidury.capnp
struct UserCmd_t
{
	glm::vec3      aAng;
	int            aButtons;
	PlayerMoveType aMoveType;
	bool           aFlashlight;  // Temp, toggles the flashlight on/off
};


// Utility Functions

// Convert a Client Source Message to String
// const char*           CL_MsgToString( EMsgSrc_Client sMsg );

// Convert a Server Source Message to String
// const char*           SV_MsgToString( EMsgSrc_Server sMsg );

ECommandSource        Game_GetCommandSource();
void                  Game_SetCommandSource( ECommandSource sSource );
void                  Game_ExecCommandsSafe( ECommandSource sSource, std::string_view sCommand );

