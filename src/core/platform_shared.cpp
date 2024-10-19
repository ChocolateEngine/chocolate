#include "core/core.h"


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#elif __unix__
#endif


void sys_print_last_error( const char* userErrorMessage )
{
	fprintf( stderr, "System Error: %s\n%s\n", sys_get_error(), userErrorMessage );
}


// https://gist.github.com/leiless/8b8603ae31c00fe38d2e97d94462a5a5


struct cpu_id2_t
{
	u32 eax;
	u32 ebx;
	u32 ecx;
	u32 edx;
};

using cpu_id_t = int[ 4 ];


constexpr int EAX = 0;
constexpr int EBX = 1;
constexpr int ECX = 2;
constexpr int EDX = 3;


void sys_cpu_id( cpu_id_t& cpu_id, int function_id, int subfunction_id )
{
#if _WIN32
	__cpuidex( cpu_id, function_id, subfunction_id );
#elif __unix__
#endif
}


cpu_id2_t sys_cpu_id2( int function_id, int subfunction_id )
{
	cpu_id2_t cpu_id{};
#if _WIN32
	u32 regs[ 4 ];
	__cpuidex( (int*)regs, function_id, subfunction_id );

	cpu_id.eax = regs[ 0 ];
	cpu_id.ebx = regs[ 1 ];
	cpu_id.ecx = regs[ 2 ];
	cpu_id.edx = regs[ 3 ];

	// __cpuidex( &cpu_id.eax, function_id, subfunction_id );
#elif __unix__
#endif
	return cpu_id;
}


//#define CHECK_FEATURE_EDX( feature, bit ) \
//	if ( cpu_id.edx & ( 1 << bit ) ) cpu_info.features |= feature;
//
//#define CHECK_FEATURE_ECX( feature, bit ) \
//	if ( cpu_id.ecx & ( 1 << bit ) ) cpu_info.features |= feature;

#define CHECK_FEATURE_EDX( cpuid, feature, bit ) \
	if ( cpuid[ EDX ] & ( 1 << bit ) ) cpu_info.features |= feature;

#define CHECK_FEATURE_ECX( cpuid, feature, bit ) \
	if ( cpuid[ ECX ] & ( 1 << bit ) ) cpu_info.features |= feature;


cpu_info_t sys_get_cpu_info()
{
	static cpu_info_t cpu_info{};

	if ( cpu_info.vendor_id[ 0 ] )
		return cpu_info;

#if _WIN32

	SYSTEM_INFO sys_info;
	GetSystemInfo( &sys_info );

	cpu_info.num_cores = sys_info.dwNumberOfProcessors;

	// cpu_id_t cpu_id{};
	// __cpuid( &cpu_id.eax, 1 );

	cpu_id_t cpu_id_vendor{};
	sys_cpu_id( cpu_id_vendor, 0, 0 );

	// Get the Vendor ID
	// https://learn.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=msvc-170#example
	( (unsigned int*)cpu_info.vendor_id )[ 0 ] = cpu_id_vendor[ EBX ];
	( (unsigned int*)cpu_info.vendor_id )[ 1 ] = cpu_id_vendor[ EDX ];
	( (unsigned int*)cpu_info.vendor_id )[ 2 ] = cpu_id_vendor[ ECX ];
	cpu_info.vendor_id[ 12 ]                   = '\0';

	// Get the model name (NOTE: what are these magic numbers?)
	for ( int i = 0x80000002, j = 0; i < 0x80000005; ++i, j += 16 )
	{
		cpu_id_t cpu_id{};
		sys_cpu_id( cpu_id, i, 0 );

		memcpy( &cpu_info.model_name[ j ],      &cpu_id[ EAX ], 4 );
		memcpy( &cpu_info.model_name[ j + 4 ],  &cpu_id[ EBX ], 4 );
		memcpy( &cpu_info.model_name[ j + 8 ],  &cpu_id[ ECX ], 4 );
		memcpy( &cpu_info.model_name[ j + 12 ], &cpu_id[ EDX ], 4 );
	}

	// Get the CPU Features
	// id's from https://en.wikipedia.org/wiki/CPUID

	cpu_id_t cpu_id_sse{};
	__cpuidex( cpu_id_sse, 1, 0 );

//	bool is_htt = ( cpu_id_sse[ EDX ] & AVX_POS );

	// EDX Data
	CHECK_FEATURE_EDX( cpu_id_sse, ECPU_Feature_HTT, 28 );
	CHECK_FEATURE_EDX( cpu_id_sse, ECPU_Feature_SSE, 25 );
	CHECK_FEATURE_EDX( cpu_id_sse, ECPU_Feature_SSE2, 26 );

	// ECX Data
	CHECK_FEATURE_ECX( cpu_id_sse, ECPU_Feature_F16C, 29 );
	CHECK_FEATURE_ECX( cpu_id_sse, ECPU_Feature_SSE3, 0 );
	CHECK_FEATURE_ECX( cpu_id_sse, ECPU_Feature_SSSE3, 9 );
	CHECK_FEATURE_ECX( cpu_id_sse, ECPU_Feature_SSE4_1, 19 );
	CHECK_FEATURE_ECX( cpu_id_sse, ECPU_Feature_SSE4_2, 20 );

//	CHECK_FEATURE_ECX( ECPU_Feature_SSE4a, 0 );
//	CHECK_FEATURE_ECX( ECPU_Feature_SSE5, 0 );
//	CHECK_FEATURE_ECX( ECPU_Feature_AVX, 0 );
//	CHECK_FEATURE_ECX( ECPU_Feature_AVX2, 0 );

#elif __unix__

	if ( SDL_HasSSE() )
		cpu_info.features |= ECPU_Feature_SSE;

	if ( SDL_HasSSE2() )
		cpu_info.features |= ECPU_Feature_SSE2;

	if ( SDL_HasSSE3() )
		cpu_info.features |= ECPU_Feature_SSE3;

	if ( SDL_HasSSE41() )
		cpu_info.features |= ECPU_Feature_SSE4_1;

	if ( SDL_HasSSE42() )
		cpu_info.features |= ECPU_Feature_SSE4_2;

#endif

	// just do this for now
	if ( SDL_HasAVX() )
		cpu_info.features |= ECPU_Feature_AVX;

	if ( SDL_HasAVX2() )
		cpu_info.features |= ECPU_Feature_AVX2;

	if ( SDL_HasAVX512F() )
		cpu_info.features |= ECPU_Feature_AVX512_F;

	return cpu_info;
}

