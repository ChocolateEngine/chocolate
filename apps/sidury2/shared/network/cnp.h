/*
 *    cnp.h    --    header for cnp protocol (chocolate net protocol)
 *
 *    A
 * 
 *    This file declares the functions and types for the CNP
 *    protocol.  CNP is a simple protocol that is used to
 *    communicate between the client and the server.
 */
#pragma once

#include "util.h"

struct CNP_Data_t
{
    char* pType;
    char* pIden;
    void* pData;
    u32   dataLen;
};


/*
 *    Creates a packet for various requests
 *
 *    @param char*         pPacket    The packet type.
 *    @param u32&          len        The length of the packet.
 *    @param CNP_Data_t*   pData      The data to send.
 *    @param u32           dataLen    The length of the data, in units
 */
char* CNP_CreatePacket( char* pPacket, u32& len, CNP_Data_t* pData, u32 dataLen );