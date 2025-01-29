/*
 *    cnp.cpp    --    source for cnp protocol (chocolate net protocol)
 *
 *    A
 * 
 *    The definitions for the CNP protocol are found here.
 */
#include "cnp.h"

constexpr u64 CH_CNP_VER  = 0x0000000000000000;
constexpr u64 CH_CNP_STEP = 512;

#include "core/core.h"

/*
 *    Helper function to write to a buffer
 *
 *    @param char**   pBuf    The buffer to write to.
 *    @param u32&     len     The length of the buffer.
 *    @param u32&     maxLen  The maximum length of the buffer.
 *    @param void*    pData   The data to write.
 *    @param u32      dataLen The length of the data.
 */
void CNP_Write( char** pBuf, u32& len, u32 maxLen, void* pData, u32 dataLen )
{
    if ( pBuf == nullptr || *pBuf == nullptr)
        return;

    if ( len + sizeof( pData ) > maxLen )
    {
        *pBuf = (char *)realloc( *pBuf, maxLen + CH_CNP_STEP );

        maxLen += CH_CNP_STEP;

        if ( *pBuf == nullptr )
            return;
    }

    memcpy( *pBuf + len, pData, dataLen );
    len += dataLen;
}

/*
 *    Creates a packet for various requests
 *
 *    @param char*         pPacket    The packet type.
 *    @param u32&          len        The length of the packet.
 *    @param CNP_Data_t*   pData      The data to send.
 *    @param u32           dataLen    The length of the data, in units
 */
char* CNP_CreatePacket( char* pPacket, u32& len, CNP_Data_t* pData, u32 dataLen ) 
{
    u32 maxLen = CH_CNP_STEP;
    len        = 0;

    char *pResult = (char *)malloc( CH_CNP_STEP );

    if ( pResult == nullptr )
    {
        return nullptr;
    }

    CNP_Write( &pResult, len, maxLen, (void *)"CNP", 3 );
    CNP_Write( &pResult, len, maxLen, (void *)&CH_CNP_VER, sizeof( CH_CNP_VER ) );
    CNP_Write( &pResult, len, maxLen, (void *)pPacket, strlen( pPacket ) + 1 );

    /*
     *    CNP[cnp_ver]GREETER[player_info]PNC    Example connection packet
     *
     *    CNP[cnp_ver]USERCMD["vec3\0"][view_ang]["int4\0"][buttonstate]PNC   Example user command packet
     */
    for ( u32 i = 0; i < dataLen; i++ )
    {
        CNP_Write( &pResult, len, maxLen, (void *)pData[i].pType, strlen( pData[i].pType ) + 1 );
        CNP_Write( &pResult, len, maxLen, (void *)pData[i].pIden, strlen( pData[i].pIden ) + 1 );
        CNP_Write( &pResult, len, maxLen, pData[i].pData, pData[i].dataLen );
    }

    pResult = (char *)realloc( pResult, len );

    return pResult;
}