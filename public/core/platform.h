#pragma once

typedef void* Module;


#ifdef _WIN32
	#define EXT_DLL ".dll"
	#define EXT_DLL_LEN 4
	#define DLL_EXPORT __declspec(dllexport)
	#define DLL_IMPORT __declspec(dllimport)

	#define PATH_SEP_STR "\\"
	#define CH_PATH_SEP_STR "\\"

	#define stat            _stat

constexpr char PATH_SEP    = '\\';
constexpr char CH_PATH_SEP = '\\';

#elif __linux__
	#define EXT_DLL ".so"
	#define EXT_DLL_LEN 3
	#define DLL_EXPORT __attribute__((__visibility__("default")))
	#define DLL_IMPORT

	#define PATH_SEP_STR    "/"
	#define CH_PATH_SEP_STR "/"

constexpr char PATH_SEP    = '/';
constexpr char CH_PATH_SEP = '/';
#else
	#error "Library loading not setup for this platform"
#endif

#ifndef CORE_DLL
	#define CORE_API DLL_IMPORT
#endif


#define CH_PLAT_FOLDER_SEP CH_PLAT_FOLDER CH_PATH_SEP_STR
//#define CH_PLAT_FOLDER_SEP_LEN ( strlen( CH_PLAT_FOLDER_SEP ) - 1 )


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


CORE_API void        sys_init();
CORE_API void        sys_shutdown();

// these are defined in platform_lib_loader.cpp, separated because of the launcher
CORE_API Module      sys_load_library( const char* path );
CORE_API void        sys_close_library( Module mod );
CORE_API void*       sys_load_func( Module mod, const char* path );
CORE_API const char* sys_get_error();
CORE_API void        sys_print_last_error( const char* userErrorMessage );

// not implemented on linux yet
#if 0
// get folder exe is stored in
CORE_API char*       sys_get_exe_folder( size_t* len = nullptr );

// get the full path of the exe
CORE_API char*       sys_get_exe_path();
#endif

// get current working directory
CORE_API char*       sys_get_cwd();

// sleep for x milliseconds
CORE_API void        sys_sleep( float ms );

// console functions
CORE_API void*       sys_get_console_window();
CORE_API int         sys_allow_console_input();

// wait for a debugger to attach
CORE_API void        sys_wait_for_debugger();
CORE_API void        sys_debug_break();

CORE_API int         sys_get_core_count();

CORE_API cpu_info_t  sys_get_cpu_info();

// window management
#ifdef _WIN32

// TODO: use a struct for this probably
CORE_API void* Sys_CreateWindow( const char* spWindowName, int sWidth, int sHeight, bool maximize );
// CORE_API bool Sys_CreateWindow( void* native_window, SDL_Window* sdl_window, const char* spWindowName, int sWidth, int sHeight, bool maximize );

CORE_API void  Sys_SetResizeCallback( FResizeCallback callback );

CORE_API int   Sys_Execute( const char* spFile, const char* spArgs );
CORE_API int   Sys_ExecuteV( const char* spFile, const char* spArgs, ... );

CORE_API void  Sys_CheckHeap();
#else
	#define Sys_CheckHeap()
#endif
