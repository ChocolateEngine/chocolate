/*
asserts.h ( Authored by Demez )

Custom Assert Handler and Error Dialog
*/

#pragma once

#include "core/platform.h"

#ifndef USE_ASSERTS
	#ifdef _DEBUG
		#define USE_ASSERTS 1
	#else
		#define USE_ASSERTS 0
	#endif
#endif


void CORE_API HandleAssert( const char* file, unsigned int line, const char* cond, const char* title, const char* msg );
void CORE_API HandleAssert( const char* file, unsigned int line, const char* cond, const char* msg );

bool CORE_API IfAssert( bool sResult, const char* file, unsigned int line, const char* cond, const char* title, const char* msg );
bool CORE_API IfAssert( bool sResult, const char* file, unsigned int line, const char* cond, const char* msg );


#if !USE_ASSERTS
  #define Assert( cond )
  #define AssertMsg( cond, msg )
  #define AssertTitleMsg( cond, title, msg )

  #define CH_IF_ASSERT( cond )                       ( !( cond ) )
  #define CH_IF_ASSERT_MSG( cond, msg )              ( !( cond ) )
  #define CH_IF_ASSERT_TITLE_MSG( cond, title, msg ) ( !( cond ) )

#else
  #define Assert( cond ) \
	if ( !( cond ) ) HandleAssert( __FILE__, __LINE__, #cond, "" )

  #define AssertMsg( cond, msg ) \
	if ( !( cond ) ) HandleAssert( __FILE__, __LINE__, #cond, msg )

  #define AssertTitleMsg( cond, title, msg ) \
	if ( !( cond ) ) HandleAssert( __FILE__, __LINE__, #cond, title, msg )

  #define CH_IF_ASSERT( cond )                       ( IfAssert( !( cond ), __FILE__, __LINE__, #cond, "" ) )
  #define CH_IF_ASSERT_MSG( cond, msg )              ( IfAssert( !( cond ), __FILE__, __LINE__, #cond, msg ) )
  #define CH_IF_ASSERT_TITLE_MSG( cond, title, msg ) ( IfAssert( !( cond ), __FILE__, __LINE__, #cond, title, msg ) )

#endif

#define CH_ASSERT( cond )                       Assert( cond )
#define CH_ASSERT_MSG( cond, msg )              AssertMsg( cond, msg )
#define CH_ASSERT_TITLE_MSG( cond, title, msg ) AssertTitleMsg( cond, title, msg )

