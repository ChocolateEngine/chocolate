#ifdef __unix__

#include "net_main.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <linux/kd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>

LOG_REGISTER_CHANNEL2( Network, LogColor::DarkCyan );

static bool        gOfflineMode    = Args_Register( "Disable All Networking", "-offline" );

static bool        gNetInit        = false;

constexpr int      CH_NET_BUF_LEN  = 1000;


// CONVAR( net_lan, 0 );
// CONVAR( net_loopback, 0 );


static ChVector< Socket_t > gSockets;

//=============================================================================

const char* Net_ErrorString()
{
	return "fixme";
}

// temp
struct NetInterface_t
{
	unsigned long aIP;
	unsigned long aMask;
};

constexpr int                     CH_MAX_NET_ADAPTERS = 32;
static ChVector< NetInterface_t > gNetInterfaces;


void Net_InitAdapterInfo()
{

}


bool Net_Init()
{
	return gNetInit = true;
}


void Net_Shutdown()
{
	if ( gOfflineMode || !gNetInit )
		return;

	gNetInit = false;
}


bool Net_ParseIPv4NetAddr( NetAddr_t& srAddr, std::string_view sString )
{
	// Check for a port first
	const char* portCheck = strstr( sString.data(), ":" );

	if ( portCheck )
	{
		// is there actually something after the ":"?
		auto remain = &sString.back() - portCheck;

		if ( remain )
		{
			// parse the port
			long num = 0;
			if ( !ToLong3( portCheck + 1, num ) )
			{
				Log_WarnF( gLC_Network, "Failed to convert IPv4 Address Port to Integer: \"%s\"\n", portCheck + 1 );
				return false;
			}

			srAddr.aPort = num;
		}
	}

	const char* prev = sString.data();
	const char* cur  = sString.data();

	int         i    = 0;
	for ( i = 0; i < 4; i++ )
	{
		if ( !prev )
			break;

		cur      = strstr( prev, "." );
		std::string numStr;
		
		if ( cur )
		{
			numStr = std::string( prev, cur - prev );
		}
		else
		{
			if ( portCheck )
			{
				cur = strstr( prev, ":" );

				if ( !cur )
					break;

				numStr = std::string( prev, cur - prev );
			}
			else if ( prev )
			{
				numStr = std::string( prev );
			}
		}
		
		long num = 0;
		if ( !ToLong2( numStr, num ) )
		{
			Log_WarnF( gLC_Network, "Failed to convert part of IPv4 Address to Integer: \"%s\"\n", numStr.c_str() );
			return false;
		}

		// Assign it
		srAddr.aIPV4[ i ]  = num;

		auto remain        = &sString.back() - cur;
		if ( remain )
		{
			prev = cur + 1;
		}
		else
		{
			i++;
			break;
		}
	}

	if ( i != 4 )
	{
		Log_WarnF( gLC_Network, "Not all parts of IPv4 Address Found: \"%s\"\n", sString.data() );
		return false;
	}

	return true;
}


NetAddr_t Net_GetNetAddrFromString( std::string_view sString )
{
	NetAddr_t netAddr;
	netAddr.aPort = 0;

	if ( sString == "127.0.0.1" || sString == "localhost" )
	{
		netAddr.aType = ENetType_Loopback;
		netAddr.aPort = 27016;
		netAddr.aIPV4[ 0 ] = 127;
		netAddr.aIPV4[ 1 ] = 0;
		netAddr.aIPV4[ 2 ] = 0;
		netAddr.aIPV4[ 3 ] = 1;

		return netAddr;
	}
	
	// TODO: support IPv6

	const char* ipv4Test = strchr( sString.data(), '.' );

	if ( ipv4Test )
	{
		if ( Net_ParseIPv4NetAddr( netAddr, sString ) )
			netAddr.aType = ENetType_IP;
	}
	else
	{
		Log_Warn( gLC_Network, "TODO: Support IPV6 in Net_GetNetAddrFromString\n" );
	}
	
	return netAddr;
}


#if 0
int Net_GetAddrFromName( char* name, ch_sockaddr* addr )
{
	struct hostent* hostentry;

	if ( name[ 0 ] >= '0' && name[ 0 ] <= '9' )
		return PartialIPAddress( name, addr );

	hostentry = pgethostbyname( name );
	if ( !hostentry )
		return -1;

	addr->sa_family                                = AF_INET;
	( (struct sockaddr_in*)addr )->sin_port        = htons( (unsigned short)net_hostport );
	( (struct sockaddr_in*)addr )->sin_addr.s_addr = *(int*)hostentry->h_addr_list[ 0 ];

	return 0;
}
#endif

void Net_NetadrToSockaddr( const NetAddr_t* spNetAddr, struct sockaddr* spSockAddr )
{
	memset( spSockAddr, 0, sizeof( *spSockAddr ) );

	if ( spNetAddr->aType == ENetType_Broadcast )
	{
		( (struct sockaddr_in*)spSockAddr )->sin_family      = AF_INET;
		( (struct sockaddr_in*)spSockAddr )->sin_addr.s_addr = INADDR_BROADCAST;
	}
	else if ( spNetAddr->aType == ENetType_IP || spNetAddr->aType == ENetType_Loopback )
	{
		// uhhh
		( (struct sockaddr_in*)spSockAddr )->sin_family      = AF_INET;
		( (struct sockaddr_in*)spSockAddr )->sin_addr.s_addr = *(int*)&spNetAddr->aIPV4;
	}

	( (struct sockaddr_in*)spSockAddr )->sin_port = htons( (short)spNetAddr->aPort );
}


// from quake
char* Net_AddrToString( ch_sockaddr& addr )
{
	static char buffer[ 22 ];
	int         haddr = ntohl( ( (struct sockaddr_in*)&addr )->sin_addr.s_addr );

	sprintf( buffer, "%d.%d.%d.%d:%d", ( haddr >> 24 ) & 0xff, ( haddr >> 16 ) & 0xff, ( haddr >> 8 ) & 0xff, haddr & 0xff, ntohs( ( (struct sockaddr_in*)&addr )->sin_port ) );
	return buffer;
}


void Net_GetSocketAddr( Socket_t sSocket, ch_sockaddr& srAddr )
{
	socklen_t    addrLen = sizeof( srAddr );
	int          ret     = getsockname( sSocket, (sockaddr*)&srAddr, &addrLen );

	unsigned int a       = ( (sockaddr_in*)&srAddr )->sin_addr.s_addr;

	if ( a == 0 || a == inet_addr( "127.0.0.1" ) )
	{
		// ( (struct sockaddr_in*)addr )->sin_addr.s_addr = myAddr;
	}

	return;
}


int Net_GetSocketPort( ch_sockaddr& srAddr )
{
	return ntohs( ( (sockaddr_in*)&srAddr )->sin_port );
}


void Net_SetSocketPort( ch_sockaddr& srAddr, unsigned short sPort )
{
	( (sockaddr_in*)&srAddr )->sin_port = htons( sPort );
	// ( (sockaddr_in*)&srAddr )->sin_port = sPort;
}


Socket_t Net_OpenSocket( const char* spPort )
{
	// if ( !spPort )
	// 	return CH_INVALID_SOCKET;

	struct addrinfo* result = NULL;
	struct addrinfo  hints;

	int              iSendResult;
	char             recvbuf[ CH_NET_BUF_LEN ];
	int              recvbuflen = CH_NET_BUF_LEN;

	memset( &hints, 0, sizeof( hints ) );
	hints.ai_family   = AF_INET;
	// hints.ai_socktype = SOCK_STREAM;
	hints.ai_socktype = SOCK_DGRAM;
	// hints.ai_protocol = IPPROTO_TCP;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags    = AI_PASSIVE;

	// Resolve the server address and port
	int iResult        = getaddrinfo( NULL, spPort, &hints, &result );
	if ( iResult != 0 )
	{
		Log_ErrorF( "getaddrinfo failed with error: %d\n", iResult );
		return CH_INVALID_SOCKET;
	}

	// Create a SOCKET for the server to listen for client connections.
	Socket_t newSocket = socket( result->ai_family, result->ai_socktype, result->ai_protocol );
	if ( newSocket == -1 )
	{
		Log_ErrorF( gLC_Network, "socket failed with error: %s\n", Net_ErrorString() );
		freeaddrinfo( result );
		return CH_INVALID_SOCKET;
	}

	// Make this socket non-blocking
	unsigned long nonBlock = 1;
	if ( ioctl( newSocket, FIONBIO, &nonBlock ) == -1 )
	{
		Log_ErrorF( gLC_Network, "Failed to make socket non-blocking ioctl FIONBIO: %s\n", Net_ErrorString() );
		return CH_INVALID_SOCKET;
	}

	// Make this socket broadcast capable
	// HACK FOR CLIENT
	// if ( strcmp( spPort, "0" ) != 0 )
	{
		int i = 1;
		if ( setsockopt( newSocket, SOL_SOCKET, SO_BROADCAST, (char*)&i, sizeof( i ) ) == -1 )
		{
			Log_ErrorF( gLC_Network, "Failed to set socket option SO_BROADCAST: %s\n", Net_ErrorString() );
			return CH_INVALID_SOCKET;
		}
	}

	iResult = bind( newSocket, result->ai_addr, (int)result->ai_addrlen );
	if ( iResult == -1 )
	{
		Log_ErrorF( gLC_Network, "Failed to bind socket to address \"%s\": %s\n", Net_AddrToString( (ch_sockaddr&)result->ai_addr ), Net_ErrorString() );
		freeaddrinfo( result );
		close( newSocket );
		return CH_INVALID_SOCKET;
	}

	Log_DevF( gLC_Network, 1, "Opened Socket on Port \"%s\"\n", spPort );

	freeaddrinfo( result );

	return newSocket;
}


void Net_CloseSocket( Socket_t sSocket )
{
	if ( !sSocket )
		return;

	int ret = close( sSocket );
	if ( ret != 0 )
	{
		Log_ErrorF( gLC_Network, "Failed to close socket: %s\n", Net_ErrorString() );
	}
}


int Net_Connect( Socket_t sSocket, ch_sockaddr& srAddr )
{
	int ret = connect( sSocket, (const sockaddr*)&srAddr, sizeof( ch_sockaddr ) );

	// check error
	if ( ret != 0 )
	{
		Log_ErrorF( gLC_Network, "Failed to connect: %s\n", Net_ErrorString() );
	}

	return ret;
}


// Read Incoming Data from a Socket
int Net_Read( Socket_t sSocket, char* spData, int sLen, ch_sockaddr* spFrom )
{
	socklen_t addrlen = sizeof( ch_sockaddr );

	int ret = recvfrom( sSocket, spData, sLen, 0, (struct sockaddr*)spFrom, &addrlen );

	if ( ret == -1 )
	{
		int errno_ = errno;

		if ( errno_ == EWOULDBLOCK || errno_ == ECONNREFUSED )
			return 0;

		Log_ErrorF( gLC_Network, "Failed to read from socket: %s\n", Net_ErrorString() );
	}

	return ret;
}


// Write Data to a Socket
int Net_Write( Socket_t sSocket, ch_sockaddr& srAddr, const char* spData, int sLen )
{
	int ret = sendto( sSocket, spData, sLen, 0, (struct sockaddr*)&srAddr, sizeof( ch_sockaddr ) );

	if ( ret == -1 )
	{
		if ( errno == EWOULDBLOCK )
			return 0;

		Log_ErrorF( gLC_Network, "Failed to write to socket: %s\n", Net_ErrorString() );
	}

	return ret;
}


int Net_MakeSocketBroadcastCapable( Socket_t sSocket )
{
	int i = 1;

	// make this socket broadcast capable
	int ret = setsockopt( sSocket, SOL_SOCKET, SO_BROADCAST, (char*)&i, sizeof( i ) );

	if ( ret < 0 )
	{
		Log_ErrorF( gLC_Network, "Failed to set socket option SO_BROADCAST: %s\n", Net_ErrorString() );
		return -1;
	}

	// net_broadcastsocket = socket;

	return 0;
}

#endif