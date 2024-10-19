#pragma once

typedef void* Module;


#ifdef _WIN32
	#define EXT_DLL ".dll"
	#define EXT_DLL_LEN 4
	#define DLL_EXPORT __declspec(dllexport)
	#define DLL_IMPORT __declspec(dllimport)
#elif __linux__
	#define EXT_DLL ".so"
	#define EXT_DLL_LEN 3
	#define DLL_EXPORT __attribute__((__visibility__("default")))
	#define DLL_IMPORT
#else
	#error "Library loading not setup for this platform"
#endif

#ifndef CORE_DLL
	#define CORE_API DLL_IMPORT
#else
	#define CORE_API DLL_EXPORT
#endif


#define CH_PLAT_FOLDER_SEP CH_PLAT_FOLDER CH_PATH_SEP_STR
//#define CH_PLAT_FOLDER_SEP_LEN ( strlen( CH_PLAT_FOLDER_SEP ) - 1 )


#ifdef _WIN32

// platform specific unicode character type
//using uchar = wchar_t;

	// Make this widechar on windows
	//#define USTR( str )     L##str

	// Make this widechar on windows
	//#define _U( str )       L##str

	#define ch_stat         _stat
//	#define ch_ustat        _wstat
//	#define ch_unprintf     _snwprintf
//	#define ch_urename 	    _wrename
//
//	#define ch_ustrcasecmp  _wcsicmp
//	#define ch_ustrncasecmp _wcsnicmp
//	#define ch_ustrcmp      wcscmp
//	#define ch_ustrncmp     wcsncmp
//	#define ch_ustrlen      wcslen
//	#define ch_ustrcat      wcscat
//	#define ch_ustrchr      wcschr
//
//	#define ch_uopen 	    _wopen
//
//	#define ch_ufopen( path, mode ) _wfopen( path, L##mode )

#else

// platform specific unicode character type
//using uchar = char;

	// Make this widechar on windows
	//#define USTR( str )  L##str

	// Make this widechar on windows
	//#define _U( str )    L##str

	#define ch_stat      stat
//	#define ch_ustat     stat
//	#define ch_unprintf  snprintf
//	#define ch_urename   rename
//
//	#define ch_ustricmp  strcasecmp
//	#define ch_ustrnicmp strncasecmp
//	#define ch_ustrcmp   strcmp
//	#define ch_ustrncmp  strncmp
//	#define ch_ustrlen   strlen
//	#define ch_ustrcat   strcat
//	#define ch_ustrchr   strchr
//
//	#define ch_uopen     open
//
//	#define ch_ufopen( path, mode ) fopen( path, mode )

#endif


using FResizeCallback = void( void* window );

using ECPU_Features = unsigned int;

enum ECPU_Features_ : unsigned int
{
	ECPU_Feature_None     = 0,

	ECPU_Feature_HTT      = ( 1 << 0 ),

	ECPU_Feature_SSE      = ( 1 << 1 ),
	ECPU_Feature_SSE2     = ( 1 << 2 ),
	ECPU_Feature_SSE3     = ( 1 << 3 ),
	ECPU_Feature_SSSE3    = ( 1 << 4 ),
	ECPU_Feature_SSE4_1   = ( 1 << 5 ),
	ECPU_Feature_SSE4_2   = ( 1 << 6 ),
//	ECPU_Feature_SSE4a    = ( 1 << 7 ),
	ECPU_Feature_SSE5     = ( 1 << 8 ),

	ECPU_Feature_F16C     = ( 1 << 9 ),

	ECPU_Feature_AVX      = ( 1 << 10 ),
	ECPU_Feature_AVX2     = ( 1 << 11 ),

	// wtf
	ECPU_Feature_AVX512_F = ( 1 << 12 ),  // Foundation
	// ECPU_Feature_AVX512_DQ           = ( 1 << 13 ),  // Doubleword and Quadword Instructions
	// ECPU_Feature_AVX512_IFMA         = ( 1 << 14 ),  // Integer Fused Multiply-Add Instructions
	// ECPU_Feature_AVX512_PF           = ( 1 << 15 ),  // Prefetch Instructions
	// ECPU_Feature_AVX512_ER           = ( 1 << 16 ),  // Exponential and Reciprocal Instructions
	// ECPU_Feature_AVX512_CD           = ( 1 << 17 ),  // Conflict Detection Instructions
	// ECPU_Feature_AVX512_BW           = ( 1 << 18 ),  // Byte and Word Instructions
	// ECPU_Feature_AVX512_VL           = ( 1 << 19 ),  // Vector Length Extensions
	// ECPU_Feature_AVX512_4vnniw       = ( 1 << 20 ),  // 4-register Neural Network Instructions
	// ECPU_Feature_AVX512_4fmaps       = ( 1 << 21 ),  // 4-register Multiply Accumulation Single precision
	// ECPU_Feature_AVX512_vp2intersect = ( 1 << 22 ),  // vector intersection instructions on 32/64-bit integers 
	// ECPU_Feature_AVX512_fp16         = ( 1 << 23 ),  // half-precision floating-point arithmetic instructions
	
	// all of AVX512
	// ECPU_Feature_AVX512         = 0,
};


struct cpu_info_t
{
	ECPU_Features features         = ECPU_Feature_None;

	int           num_smt          = 0;
	int           num_cores        = 0;
	int           num_logical_cpus = 0;

	char          vendor_id[ 13 ];
	char          model_name[ 48 ];
};


// why is msvc like this with this dllexport/dllimport stuff aaaa
void            CORE_API    sys_init();
void            CORE_API    sys_shutdown();

Module          CORE_API    sys_load_library( const char* path );
void            CORE_API    sys_close_library( Module mod );
void            CORE_API*   sys_load_func( Module mod, const char* path );
const char      CORE_API*   sys_get_error();
void            CORE_API    sys_print_last_error( const char* userErrorMessage );

// sleep for x milliseconds
void            CORE_API    sys_sleep( float ms );

// console functions
void            CORE_API*   sys_get_console_window();
int             CORE_API    sys_allow_console_input();

// wait for a debugger to attach
void            CORE_API    sys_wait_for_debugger();
void            CORE_API    sys_debug_break();

int CORE_API                sys_get_core_count();

struct cpu_info_t CORE_API         sys_get_cpu_info();

//uchar*                      Sys_ToWideChar( const char* spStr, int sSize );
//char*                       Sys_ToMultiByte( const uchar* spStr, int sSize );

//void                        Sys_FreeConvertedString( const uchar* spStr );
//void                        Sys_FreeConvertedString( const char* spStr );

// window management
#ifdef _WIN32
	#define CH_SYS_TO_UNICODE( str, len ) Sys_ToWideChar( str, len )
	#define CH_SYS_TO_ANSI( str, len )    Sys_ToMultiByte( str, len )

// TODO: use a struct for this probably
void            CORE_API*   Sys_CreateWindow( const char* spWindowName, int sWidth, int sHeight, bool maximize );
void            CORE_API    Sys_SetResizeCallback( FResizeCallback callback );

int             CORE_API    Sys_Execute( const char* spFile, const char* spArgs );
int             CORE_API    Sys_ExecuteV( const char* spFile, const char* spArgs, ... );

void CORE_API               Sys_CheckHeap();
#else
	#define CH_SYS_TO_UNICODE( str, len ) str
	#define CH_SYS_TO_ANSI( str, len )    str

	#define Sys_CheckHeap()
#endif
