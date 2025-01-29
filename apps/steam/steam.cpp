#include "steam.h"

LOG_CHANNEL_REGISTER( Steam, ELogColor_Default );
LOG_CHANNEL_REGISTER( SteamAPI, ELogColor_Default );

SteamAbstraction*        gSteam        = new SteamAbstraction;
bool                     gSteamLoaded  = false;

ISteamToGame*            gpSteamToGame = nullptr;

// Used for loading avatar images, and other images
IRender*                 render        = nullptr;

static ModuleInterface_t gInterfaces[] = {
	{ gSteam, ISTEAM_NAME, ISTEAM_VER }
};

extern "C"
{
	DLL_EXPORT ModuleInterface_t* ch_get_interfaces( u8& srCount )
	{
		srCount = 1;
		return gInterfaces;
	}
}

static void SteamLoggingHook( int sSeverity, const char* sText )
{
	switch ( sSeverity )
	{
		case 0:
			Log_MsgF( gLC_SteamAPI, "%s\n", sText );
			break;

		case 1:
			Log_WarnF( gLC_SteamAPI, "%s\n", sText );
			break;

		default:
			Log_WarnF( gLC_SteamAPI, "LEVEL %d: %s\n", sText );
			break;
	}
}

static CSteamID                                     gSteamID;
static SteamID64_t                                  gSteamIDHandle = 0;

static std::unordered_map< int, ChVector< uint8 > > gSteamTextureData;
static std::unordered_map< int, ch_handle_t >            gSteamTextureHandles;

// ==================================================
// ConVars and ConCommands
// ==================================================


CONCMD( steam_profile_name )
{
	if ( !gSteamLoaded )
	{
		Log_Error( gLC_Steam, "Steam API is not initialized\n" );
		return;
	}

	Log_MsgF( gLC_Steam, "Steam Profile Name: %s\n", SteamFriends()->GetPersonaName() );
}


CONCMD( steam_load_overlay )
{
	if ( !gSteamLoaded )
	{
		Log_Error( gLC_Steam, "Steam API is not initialized\n" );
		return;
	}
	
	gSteam->LoadSteamOverlay();
}


// ==================================================
// ISystem Interfaces
// ==================================================


bool SteamAbstraction::Init()
{
	// Used for loading avatar images
	render = Mod_GetSystemCast< IRender >( IRENDER_NAME, IRENDER_VER );
	if ( render == nullptr )
	{
		Log_Error( gLC_Steam, "Failed to load Renderer\n" );
		return false;
	}

	return true;
}


void SteamAbstraction::Update( float sFrameTime )
{
	SteamAPI_RunCallbacks();
}


// ==================================================
// General Steam Abstraction Functions
// ==================================================


bool SteamAbstraction::LoadSteam( ISteamToGame* spSteamToGame )
{
	if ( !spSteamToGame )
	{
		Log_Error( gLC_Steam, "Failed Loading Steam API - ISteamToGame Interface is nullptr, we need this to talk to the game\n" );
		return false;
	}

	gpSteamToGame = spSteamToGame;

	gSteamLoaded  = SteamAPI_Init();

	if ( !gSteamLoaded )
		return false;

	SteamUtils()->SetWarningMessageHook( SteamLoggingHook );

	gSteamID         = SteamUser()->GetSteamID();
	gSteamIDHandle   = gSteamID.ConvertToUint64();

	return true;
}


void SteamAbstraction::Shutdown()
{
	if ( !gSteamLoaded )
		return;

	SteamAPI_Shutdown();
}


void SteamAbstraction::LoadSteamOverlay()
{
	if ( !gSteamLoaded )
		return;

	bool enabled = SteamUtils()->IsOverlayEnabled();

	// TODO:
	// GameOverlayUI.exe -pid 12345 -steampid 67890 -gameid chocolate_app_id

	// Only win32 because GetSteamInstallPath is windows only and deprecated
#ifdef _WIN32
	// Check if this is already loaded (It will be if we are running from steam)
	Module mod = sys_load_library( "GameOverlayRenderer64.dll" );
	if ( mod )
		return;

	// Not Loaded, so lets load it ourself
	const char* steamPath = SteamAPI_GetSteamInstallPath();

	std::string gameOverlayPath = steamPath;
	gameOverlayPath += "/GameOverlayRenderer64.dll";

	mod = sys_load_library( gameOverlayPath.c_str() );
#endif

	// SteamFriends()->ActivateGameOverlay( "Achievements" );
}


// ==================================================
// SteamFriends
// ==================================================


const char* SteamAbstraction::GetPersonaName()
{
	if ( !gSteamLoaded )
		return nullptr;

	return SteamFriends()->GetPersonaName();
}


SteamID64_t SteamAbstraction::GetSteamID()
{
	if ( !gSteamLoaded )
		return 0ull;

	return gSteamIDHandle;
}


// ==================================================
// Steam Requests
// ==================================================


bool SteamAbstraction::RequestAvatarImage( ESteamAvatarSize sSize, SteamID64_t sSteamID )
{
	if ( !gSteamLoaded )
		return CH_INVALID_HANDLE;

	CSteamID steamID( sSteamID );

	if ( !steamID.IsValid() )
	{
		Log_ErrorF( gLC_Steam, "Failed to GetAvatarImage: Invalid Steam ID: %zd\n", sSteamID );
		return false;
	}

	int avatarHandle = 0;

	switch ( sSize )
	{
		case ESteamAvatarSize_Small:
			avatarHandle = SteamFriends()->GetSmallFriendAvatar( steamID );
			break;

		case ESteamAvatarSize_Medium:
			avatarHandle = SteamFriends()->GetMediumFriendAvatar( steamID );
			break;

		default:
		case ESteamAvatarSize_Large:
			avatarHandle = SteamFriends()->GetLargeFriendAvatar( steamID );
			break;
	}

	if ( avatarHandle == -1 )
		return true;

	// Before we do anything, check if we loaded this image already
	auto it = gSteamTextureHandles.find( avatarHandle );
	if ( it != gSteamTextureHandles.end() )
	{
		gpSteamToGame->OnRequestAvatarImage( sSteamID, sSize, it->second );
		return true;
	}

	AvatarImageLoaded_t param;
	param.m_iImage    = avatarHandle;
	param.m_steamID = steamID;

	switch ( sSize )
	{
		case ESteamAvatarSize_Small:
			param.m_iTall = param.m_iWide = 32;
			break;

		case ESteamAvatarSize_Medium:
			param.m_iTall = param.m_iWide = 64;
			break;

		default:
		case ESteamAvatarSize_Large:
			param.m_iTall = param.m_iWide = 184;
			break;
	}

	OnAvatarImageLoaded( &param );

	return true;
}


bool SteamAbstraction::RequestProfileName( SteamID64_t sSteamID )
{
	if ( !gSteamLoaded )
		return CH_INVALID_HANDLE;

	CSteamID steamID( sSteamID );

	if ( !steamID.IsValid() )
	{
		Log_ErrorF( gLC_Steam, "Failed to RequestProfileName: Invalid Steam ID: %zd\n", sSteamID );
		return false;
	}

	if ( !SteamFriends()->RequestUserInformation( steamID, true ) )
	{
		// gpSteamToGame->OnRequestProfileName();
	}

	return true;
}


// ==================================================
// Internal Functions
// ==================================================


CSteamID SteamAbstraction::GetSteamIDFromHandle( SteamID64_t sHandle )
{
	return CSteamID( sHandle );
}


// void SteamAbstraction::OnRequestAvatarImage( ESteamAvatarSize sSize, SteamID64_t sSteamID )
// {
// }


void SteamAbstraction::LoadAvatarImage( AvatarImageLoaded_t* spParam )
{
#if 0
	ChVector< uint8 >& data = gSteamTextureData[ spParam->m_iImage ];

	data.resize( spParam->m_iTall * spParam->m_iWide * 4, true );

	if ( !SteamUtils()->GetImageRGBA( spParam->m_iImage, data.data(), data.size_bytes() ) )
	{
		Log_Error( gLC_Steam, "Failed to GetImageRGBA for RequestAvatarImage\n" );
		return;
	}

	std::string texName = vstring( "steam_avatar_image_%d", spParam->m_iImage );

	TextureCreateInfo_t create{};

	create.apName = new char[ texName.size() ];
	strcpy( const_cast< char* >( create.apName ), texName.data() );

	create.aSize.x   = spParam->m_iWide;
	create.aSize.y   = spParam->m_iTall;
	create.aFormat   = GraphicsFmt::RGBA8888_SRGB;
	create.aViewType = EImageView_2D;
	create.apData    = (void*)data.data();
	create.aDataSize = data.size_bytes();
	
	TextureCreateData_t createData{};
	createData.aUseMSAA = false;
	createData.aUsage   = EImageUsage_Sampled;
	createData.aFilter  = EImageFilter_Nearest;
	
	ch_handle_t texture = render->CreateTexture( create, createData );

	if ( texture == CH_INVALID_HANDLE )
		return;

	gSteamTextureHandles[ spParam->m_iImage ] = texture;

	gpSteamToGame->OnRequestAvatarImage( spParam->m_steamID.ConvertToUint64(), texture );
#endif
}


// ==================================================
// Callback Functions
// ==================================================


static ESteamAvatarSize GetAvatarSizeFromWidth( int sWidth )
{
	switch ( sWidth )
	{
		default:
		case 32:
			return ESteamAvatarSize_Small;

		case 64:
			return ESteamAvatarSize_Medium;

		case 184:
			return ESteamAvatarSize_Large;
	}
}


void SteamAbstraction::OnAvatarImageLoaded( AvatarImageLoaded_t* spParam )
{
	// Before we do anything, check if we loaded this image already
	auto it = gSteamTextureHandles.find( spParam->m_iImage );
	if ( it != gSteamTextureHandles.end() )
	{
		ESteamAvatarSize size = GetAvatarSizeFromWidth( spParam->m_iWide );
		gpSteamToGame->OnRequestAvatarImage( spParam->m_steamID.ConvertToUint64(), size, it->second );
		return;
	}

	ChVector< uint8 >& data = gSteamTextureData[ spParam->m_iImage ];

	data.resize( spParam->m_iTall * spParam->m_iWide * 4, true );

	if ( !SteamUtils()->GetImageRGBA( spParam->m_iImage, data.data(), data.size_bytes() ) )
	{
		Log_Error( gLC_Steam, "Failed to GetImageRGBA for RequestAvatarImage\n" );
		return;
	}

	std::string texName = vstring( "steam_avatar_image_%d", spParam->m_iImage );

	TextureCreateInfo_t create{};

	create.apName = new char[ texName.size() ];
	strcpy( const_cast< char* >( create.apName ), texName.data() );

	create.aSize.x   = spParam->m_iWide;
	create.aSize.y   = spParam->m_iTall;
	create.aFormat   = GraphicsFmt::RGBA8888_SRGB;
	create.aViewType = EImageView_2D;
	create.apData    = (void*)data.data();
	create.aDataSize = data.size_bytes();
	
	TextureCreateData_t createData{};
	createData.aUseMSAA = false;
	createData.aUsage   = EImageUsage_Sampled;
	createData.aFilter  = EImageFilter_Nearest;
	
	ch_handle_t texture = render->CreateTexture( create, createData );

	if ( texture == CH_INVALID_HANDLE )
		return;

	Log_Msg( gLC_Steam, "Loaded Steam Avatar\n" );

	gSteamTextureHandles[ spParam->m_iImage ] = texture;

	ESteamAvatarSize size                     = GetAvatarSizeFromWidth( spParam->m_iWide );
	gpSteamToGame->OnRequestAvatarImage( spParam->m_steamID.ConvertToUint64(), size, texture );
}


void SteamAbstraction::OnProfileDataLoaded( PersonaStateChange_t* spParam )
{
	CSteamID steamID( spParam->m_ulSteamID );

	if ( !steamID.IsValid() )
	{
		Log_ErrorF( gLC_Steam, "Invalid Steam ID: %zd\n", spParam->m_ulSteamID );
		return;
	}

	// Log_DevF( gLC_Steam, 1, "Persona State Changed for Steam User \"%s\"\n", steamID.Render() );
	// Log_DevF( gLC_Steam, 1, "Persona State Changed for Steam User \"%zd\"\n", spParam->m_ulSteamID );

	if ( spParam->m_nChangeFlags & k_EPersonaChangeName )
	{
		const char* name = SteamFriends()->GetFriendPersonaName( steamID );

		if ( name )
		{
			Log_DevF( gLC_Steam, 1, "Persona Name for Steam User: \"%s\"\n", name );
			gpSteamToGame->OnRequestProfileName( spParam->m_ulSteamID, name );
		}
	}
}

