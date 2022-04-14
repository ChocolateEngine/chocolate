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


void CORE_API HandleAssert( const char* file, u32 line, const char* cond, const char* title, const char* msg );
void CORE_API HandleAssert( const char* file, u32 line, const char* cond, const char* msg );


#if !USE_ASSERTS
	#define Assert( cond )
	#define AssertMsg( cond, msg )
	#define AssertTitleMsg( cond, title, msg )

#else
	#define Assert( cond )                              if ( !(cond) ) HandleAssert( __FILE__, __LINE__, #cond, "" )
	#define AssertMsg( cond, msg )                      if ( !(cond) ) HandleAssert( __FILE__, __LINE__, #cond, msg )
	#define AssertTitleMsg( cond, title, msg )          if ( !(cond) ) HandleAssert( __FILE__, __LINE__, #cond, title, msg )

#endif

