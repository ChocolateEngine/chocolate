#include "flatbuffers/flatbuffers.h"
#include "net_main.h"


#if 0
int Net_ReadPacked( Socket_t sSocket, char* spData, int sLen, ch_sockaddr* spFrom )
{
	// NetOutputStream outStream;

	int read = Net_Read( sSocket, spData, sLen, spFrom );

	if ( read == 0 )
		return 0;

	NetBufferedInputStream     inputStream( spData, read );
	capnp::PackedMessageReader reader( inputStream );

	return read;
}
#endif


int Net_WriteFlatBuffer( Socket_t sSocket, ch_sockaddr& srAddr, flatbuffers::FlatBufferBuilder& srBuilder )
{
	return Net_Write( sSocket, srAddr, reinterpret_cast< const char* >( srBuilder.GetBufferPointer() ), srBuilder.GetSize() );
}


#if 0
void Net_TestPacked()
{
	capnp::MallocMessageBuilder message;
	NetMsgUserCmd::Builder      clientInfoBuild = message.initRoot< NetMsgUserCmd >();

	clientInfoBuild.setButtons( 47 );
	clientInfoBuild.setFlashlight( true );
	clientInfoBuild.setMoveType( EEPlayerMoveType_NO_CLIP );

	NetOutputStream outputStream;

	capnp::writePackedMessage( outputStream, message );

	auto                       array       = capnp::messageToFlatArray( message );

	size_t                     computeSize = capnp::computeSerializedSizeInWords( message );

	NetBufferedInputStream     inputStream( outputStream.aBuffer );
	
	size_t                     computeSize2 = capnp::computeUnpackedSizeInWords( inputStream.aReadBuffer );

	// TEMP TESTING !!!!!!!!!
	// NetBufferedInputStream     inputStream( outputStream.aBuffer.data(), outputStream.aBuffer.size() );
	capnp::PackedMessageReader reader( inputStream );
	NetMsgUserCmd::Reader      clientInfoRead = reader.getRoot< NetMsgUserCmd >();

	Log_Msg( "FUASDIO\n" );
}
#endif


#if 0

LOG_CHANNEL_REGISTER( Network, ELogColor_DarkRed );

static bool gOfflineMode = args_register( "Disable All Networking", "-offline" );
static bool gNetInit     = false;


// CONVAR( net_lan, 0 );
// CONVAR( net_loopback, 0 );


#ifdef WIN32
bool Net_InitWindows()
{
	WSADATA wsaData{};
	int     ret = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );

	if ( ret != 0 )
	{
		// We could not find a usable Winsock DLL
		Log_ErrorF( gLC_Network, "WSAStartup failed with error: %d\n", ret );
		return false;
	}

	// Confirm that the WinSock DLL supports 2.2.
	// Note that if the DLL supports versions greater
	// than 2.2 in addition to 2.2, it will still return
	// 2.2 in wVersion since that is the version we requested.                                       
	if ( LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 2 )
	{
		Log_Error( gLC_Network, "Could not find a usable version of Winsock.dll\n" );
		WSACleanup();
		return false;
	}

	return true;
}


void Net_ShutdownWindows()
{
	WSACleanup();
}
#endif // WIN32


bool Net_Init()
{
	// just return true if we don't want networking
	if ( gOfflineMode )
		return true;

	bool ret = false;

#ifdef WIN32
	ret = Net_InitWindows();
#endif

	if ( !ret )
		Log_Error( gLC_Network, "Failed to Init Networking\n" );

	gNetInit = ret;
	return ret;
}


void Net_Shutdown()
{
	if ( gOfflineMode || !gNetInit )
		return;

#ifdef WIN32
	Net_ShutdownWindows();
#endif

	gNetInit = false;
}


void Net_OpenSocket()
{
}


bool Net_GetPacket( NetAddr_t& srFrom, void* spData, int& sSize, int sMaxSize )
{
	return false;
}


bool Net_GetPacketBlocking( NetAddr_t& srFrom, void* spData, int& sSize, int sMaxSize, int sTimeOut )
{
	return false;
}


void Net_SendPacket( const NetAddr_t& srTo, const void* spData, int sSize )
{
}

#endif