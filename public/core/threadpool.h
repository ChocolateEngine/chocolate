/*
 *    threadpool.h    --    Threadpool
 *
 *    Authored by Karl "p0lyh3dron" Kreuze on March 5, 2022
 * 
 *    This files declares the threadpool, as well as other
 *    types and functionsrelated to thread management.
 */
#pragma once

#include <SDL.h>

#include "platform.h"

class CORE_API AutoMutex
{
    SDL_mutex *apMutex;
public:
    /*
     *    Construct an auto mutex.
     */
    AutoMutex( SDL_mutex *pMutex ) : apMutex( pMutex )
    {
        Lock();
    }
    /*
     *    Destruct the auto mutex.
     */
    ~AutoMutex()
    {
        Unlock();
    }
    /*
     *    Lock the mutex.
     */
    void Lock()
    {
        SDL_LockMutex( apMutex );
    }
    /*
     *    Unlock the mutex.
     */
    void Unlock()
    {
        SDL_UnlockMutex( apMutex );
    }
};
