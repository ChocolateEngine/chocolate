#include "game_rules.h"
//#include "cl_main.h"


GameRulesDatabase* GameRules()
{
	return nullptr;
}


template< typename T >
inline void sv_set_gamerule_dropdown_vartype(
  const std::vector< std::string >&    args,  // arguments currently typed in by the user
  std::vector< std::string >&          results,
  std::unordered_map< std::string, T > vars )
{
	for ( const auto& [ name, value ] : vars )
	{
		if ( args.size() )
		{
			if ( args[ 0 ].size() > name.size() )
				continue;

			if ( ch_strncasecmp( name.data(), args[ 0 ].data(), args[ 0 ].size() ) != 0 )
				continue;
		}

		// std::string value = vstring( "%s %d", name.data(), rule.AsString() );
		results.push_back( name.data() );
	}
}


static void sv_set_gamerule_dropdown(
  const std::vector< std::string >& args,         // arguments currently typed in by the user
  const std::string&                fullCommand,  // the full command line the user has typed in
  std::vector< std::string >&       results )     // results to populate the dropdown list with
{
#if 0
	for ( const auto& [ name, type ] : GameRules()->aVarTypes )
	{
		if ( args.size() )
		{
			if ( args[ 0 ].size() > name.size() )
				continue;

			if ( ch_strncasecmp( name.data(), args[ 0 ].data(), args[ 0 ].size() ) != 0 )
				continue;
		}

		// std::string value = vstring( "%s %d", name.data(), rule.AsString() );
		results.push_back( name.data() );
	}
#endif
}


#if CH_SERVER
CONCMD_DROP_VA( sv_set_gamerule, sv_set_gamerule_dropdown, CVARF( CL_EXEC ) | CVARF_CHEAT, "Set a Gamerule on the Server" )
{
	if ( args.size() == 0 )
	{
		// dump all gamerules
		Log_Msg( "TODO: dump all gamerules\n" );
		return;
	}
}
#endif


// ---------------------------------------------------------------------------------------------


#if 0
void GameRulesDatabase::DeleteVariable( std::string_view sName )
{
	auto itType = aVarTypes.find( sName.data() );
	
	if ( itType == aVarTypes.end() )
	{
		Log_ErrorF( "Failed to delete variable in GameStateDatabase: %s\n", sName.data() );
		return;
	}

	EGameRuleType type = itType->second;

	// what if the type is a vector
	aVarTypes.erase( itType );

	// blech
	switch ( itType->second )
	{
		case EGameRuleType_Bool:
		{
			auto it = aVarBools.find( sName.data() );
			if ( it == aVarBools.end() )
			{
				Log_ErrorF( "Failed to delete variable in GameStateDatabase: %s\n", sName.data() );
				return;
			}

			aVarBools.erase( it );
			break;
		}

		case EGameRuleType_Float:
		{
			auto it = aVarFloats.find( sName.data() );
			if ( it == aVarFloats.end() )
			{
				Log_ErrorF( "Failed to delete variable in GameStateDatabase: %s\n", sName.data() );
				return;
			}

			aVarFloats.erase( it );
			break;
		}

		case EGameRuleType_Double:
		{
			auto it = aVarDoubles.find( sName.data() );
			if ( it == aVarDoubles.end() )
			{
				Log_ErrorF( "Failed to delete variable in GameStateDatabase: %s\n", sName.data() );
				return;
			}

			aVarDoubles.erase( it );
			break;
		}

		case EGameRuleType_Int:
		{
			auto it = aVarInts.find( sName.data() );
			if ( it == aVarInts.end() )
			{
				Log_ErrorF( "Failed to delete variable in GameStateDatabase: %s\n", sName.data() );
				return;
			}

			aVarInts.erase( it );
			break;
		}

		case EGameRuleType_UnsignedInt:
		{
			auto it = aVarUnsignedInts.find( sName.data() );
			if ( it == aVarUnsignedInts.end() )
			{
				Log_ErrorF( "Failed to delete variable in GameStateDatabase: %s\n", sName.data() );
				return;
			}

			aVarUnsignedInts.erase( it );
			break;
		}

		case EGameRuleType_String:
		{
			auto it = aVarStrings.find( sName.data() );
			if ( it == aVarStrings.end() )
			{
				Log_ErrorF( "Failed to delete variable in GameStateDatabase: %s\n", sName.data() );
				return;
			}

			aVarStrings.erase( it );
			break;
		}
	}
}


bool GameRulesDatabase::SetVarInternal( std::string_view sName, EGameRuleType sType )
{
	if ( CH_IF_ASSERT( Game_ProcessingServer() ) )
		return false;

	auto it = aVarTypes.find( sName.data() );

	if ( it == aVarTypes.end() )
	{
		aVarTypes[ sName.data() ] = sType;
		return true;
	}

	if ( CH_IF_ASSERT_MSG( it->second == sType, "Invalid Game State Type" ) )
		return false;

	return true;
}
#endif


void GameRulesDatabase::DeleteBool( std::string_view sName )
{
	auto it = aVarBools.find( sName.data() );

	if ( it == aVarBools.end() )
	{
		Log_ErrorF( "Failed to delete bool variable in GameStateDatabase: %s\n", sName.data() );
		return;
	}

	aVarBools.erase( it );
}


bool GameRulesDatabase::SetBool( std::string_view sName, bool sValue )
{
	//if ( CH_IF_ASSERT( Game_ProcessingServer() ) )
	//	return false;

	auto it = aVarBools.find( sName.data() );

	if ( it == aVarBools.end() )
	{
		aVarBools[ sName.data() ] = sValue;
		return true;
	}

	it->second = sValue;
	return true;
}


void GameRulesDatabase::DeleteFloat( std::string_view sName )
{
	auto it = aVarFloats.find( sName.data() );

	if ( it == aVarFloats.end() )
	{
		Log_ErrorF( "Failed to delete float variable in GameStateDatabase: %s\n", sName.data() );
		return;
	}

	aVarFloats.erase( it );
}


void GameRulesDatabase::DeleteDouble( std::string_view sName )
{
	auto it = aVarDoubles.find( sName.data() );

	if ( it == aVarDoubles.end() )
	{
		Log_ErrorF( "Failed to delete double variable in GameStateDatabase: %s\n", sName.data() );
		return;
	}

	aVarDoubles.erase( it );
}


void GameRulesDatabase::DeleteInt( std::string_view sName )
{
	auto it = aVarInts.find( sName.data() );

	if ( it == aVarInts.end() )
	{
		Log_ErrorF( "Failed to delete int variable in GameStateDatabase: %s\n", sName.data() );
		return;
	}

	aVarInts.erase( it );
}


void GameRulesDatabase::DeleteUnsignedInt( std::string_view sName )
{
	auto it = aVarUnsignedInts.find( sName.data() );

	if ( it == aVarUnsignedInts.end() )
	{
		Log_ErrorF( "Failed to delete unsigned int variable in GameStateDatabase: %s\n", sName.data() );
		return;
	}

	aVarUnsignedInts.erase( it );
}


void GameRulesDatabase::DeleteString( std::string_view sName )
{
	auto it = aVarStrings.find( sName.data() );

	if ( it == aVarStrings.end() )
	{
		Log_ErrorF( "Failed to delete string variable in GameStateDatabase: %s\n", sName.data() );
		return;
	}

	aVarStrings.erase( it );
}


// Returns a pointer to the var data if we can set it
//inline GameRule_t* SetVarInternal( std::string_view sName, EGameRuleType sType )
//{
//	if ( CH_IF_ASSERT( Game_ProcessingServer() ) )
//		return nullptr;
//
//	auto it = aVars.find( sName.data() );
//
//	if ( it == aVars.end() )
//	{
//		GameRule_t& var = aVars[ sName.data() ];
//		var.name       = sName.data();
//		var.aType       = sType;
//
//		return &var;
//	}
//	else
//	{
//		GameRule_t& var = it->second;
//
//		if ( CH_IF_ASSERT_MSG( var.aType == sType, "Invalid Game State Type" ) )
//			return nullptr;
//
//		return &var;
//	}
//}


// Returns a pointer to the var data if it exists as this type
//inline GameRule_t* GetVarInternal( std::string_view sName, EGameRuleType sType )
//{
//	auto it = aVars.find( sName.data() );
//
//	if ( it != aVars.end() )
//	{
//		GameRule_t& var = it->second;
//
//		if ( CH_IF_ASSERT_MSG( var.aType == sType, "Invalid Game State Type" ) )
//			return nullptr;
//
//		return &var;
//	}
//
//	return nullptr;
//}