#pragma once

typedef void* Module;


#ifdef _WIN32
	#define EXT_DLL ".dll"
	#define DLL_EXPORT __declspec(dllexport)
	#define DLL_IMPORT __declspec(dllimport)
#elif __linux__
	#define EXT_DLL ".so"
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
	ECPU_Feature_SSE4a    = ( 1 << 7 ),
	ECPU_Feature_SSE5     = ( 1 << 8 ),

	ECPU_Feature_AVX      = ( 1 << 9 ),
	ECPU_Feature_AVX2     = ( 1 << 10 ),

	// wtf
	ECPU_Feature_AVX512_F = ( 1 << 11 ),  // Foundation
	// ECPU_Feature_AVX512_DQ           = ( 1 << 12 ),  // Doubleword and Quadword Instructions
	// ECPU_Feature_AVX512_IFMA         = ( 1 << 13 ),  // Integer Fused Multiply-Add Instructions
	// ECPU_Feature_AVX512_PF           = ( 1 << 14 ),  // Prefetch Instructions
	// ECPU_Feature_AVX512_ER           = ( 1 << 15 ),  // Exponential and Reciprocal Instructions
	// ECPU_Feature_AVX512_CD           = ( 1 << 16 ),  // Conflict Detection Instructions
	// ECPU_Feature_AVX512_BW           = ( 1 << 17 ),  // Byte and Word Instructions
	// ECPU_Feature_AVX512_VL           = ( 1 << 18 ),  // Vector Length Extensions
	// ECPU_Feature_AVX512_4vnniw       = ( 1 << 19 ),  // 4-register Neural Network Instructions
	// ECPU_Feature_AVX512_4fmaps       = ( 1 << 20 ),  // 4-register Multiply Accumulation Single precision
	// ECPU_Feature_AVX512_vp2intersect = ( 1 << 21 ),  // vector intersection instructions on 32/64-bit integers 
	// ECPU_Feature_AVX512_fp16         = ( 1 << 22 ),  // half-precision floating-point arithmetic instructions
	
	// all of AVX512
	// ECPU_Feature_AVX512         = 0,
};


struct cpu_info_t
{
	char*         vendor_id        = nullptr;
	char*         model_name       = nullptr;

	int           num_smt          = 0;
	int           num_cores        = 0;
	int           num_logical_cpus = 0;

	ECPU_Features features         = ECPU_Feature_None;
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

int CORE_API                Sys_GetCoreCount();

cpu_info_t CORE_API         Sys_GetCpuInfo();

// window management
#ifdef _WIN32
void            CORE_API*   Sys_CreateWindow( const char* spWindowName, int sWidth, int sHeight, bool maximize );
void            CORE_API    Sys_SetResizeCallback( FResizeCallback callback );

int             CORE_API    Sys_Execute( const char* spFile, const char* spArgs );
int             CORE_API    Sys_ExecuteV( const char* spFile, const char* spArgs, ... );

void CORE_API               Sys_CheckHeap();
#else
	#define Sys_CheckHeap()
#endif
