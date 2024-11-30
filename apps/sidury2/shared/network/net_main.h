#pragma once


enum ENetType : char
{
	ENetType_Invalid,
	ENetType_Loopback,
	ENetType_Broadcast,
	ENetType_IP,
};


struct NetAddr_t
{
	ENetType       aType   = ENetType_Invalid;
	bool           aIsIPV6 = false;
	unsigned short aPort   = 0;

	union
	{
		unsigned char aIPV4[ 4 ];
		char          aIPV6[ 8 ][ 4 ];
	};
};


// mimicing sockaddr struct
struct ch_sockaddr
{
	short         sa_family;
	unsigned char sa_data[ 14 ];
};


struct NetChannel_t
{

};

#ifdef _WIN32

using Socket_t = void*;
  #define CH_INVALID_SOCKET ( Socket_t )( ~0 )

#elif __unix__

using Socket_t = int;
  #define CH_INVALID_SOCKET -1

#else

  #error "Need to Define Socket_t for this platform"

#endif


// ---------------------------------------------------------------------------
// General Network Functions


const char* Net_ErrorString();

bool        Net_Init();
void        Net_Shutdown();

NetAddr_t   Net_GetNetAddrFromString( std::string_view sString );
char*       Net_AddrToString( ch_sockaddr& addr );
void        Net_GetSocketAddr( Socket_t sSocket, ch_sockaddr& srAddr );
int         Net_GetSocketPort( ch_sockaddr& srAddr );
void        Net_SetSocketPort( ch_sockaddr& srAddr, unsigned short sPort );

Socket_t    Net_OpenSocket( const char* spPort );
void        Net_CloseSocket( Socket_t sSocket );
int         Net_Connect( Socket_t sSocket, ch_sockaddr& srAddr );

// Read Incoming Data from a Socket
int         Net_Read( Socket_t sSocket, char* spData, int sLen, ch_sockaddr* spFrom );

// Write Data to a Socket
int         Net_Write( Socket_t sSocket, ch_sockaddr& srAddr, const char* spData, int sLen );

int         Net_MakeSocketBroadcastCapable( Socket_t sSocket );


// ---------------------------------------------------------------------------
// Network Channels

