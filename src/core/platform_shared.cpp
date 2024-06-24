#include "core/core.h"


#ifdef _MSC_VER
	#include <intrin.h>
#endif


#ifdef __GNUC__

void __cpuid( int* cpuinfo, int info )
{
	__asm__ __volatile__(
	  "xchg %%ebx, %%edi;"
	  "cpuid;"
	  "xchg %%ebx, %%edi;"
	  : "=a"( cpuinfo[0] ), "=D"( cpuinfo[1] ), "=c"( cpuinfo[2] ), "=d"( cpuinfo[3] )
	  : "0"( info ) );
}

unsigned long long _xgetbv( unsigned int index )
{
	unsigned int eax, edx;
	__asm__ __volatile__(
	  "xgetbv;"
	  : "=a"( eax ), "=d"( edx )
	  : "c"( index ) );
	return ( (unsigned long long)edx << 32 ) | eax;
}

#endif


// Source: https://gist.github.com/hi2p-perim/7855506
// Source: https://www.intel.com/content/dam/develop/external/us/en/documents/how-to-detect-new-instruction-support-in-the-4th-generation-intel-core-processor-family.pdf

#if 0
ECPU_Features Sys_GetCpuFeatures()
{
	ECPU_Features features = ECPU_Feature_None;

	int           cpuinfo[ 4 ];
	__cpuid( cpuinfo, 1 );

	// Check SSE, SSE2, SSE3, SSSE3, SSE4.1, and SSE4.2 support
	if ( cpuinfo[ 3 ] & ( 1 << 25 ) || false )
		features |= ECPU_Feature_SSE;

	if ( cpuinfo[ 3 ] & ( 1 << 26 ) || false )
		features |= ECPU_Feature_SSE2;

	if ( cpuinfo[ 2 ] & ( 1 << 0 ) || false )
		features |= ECPU_Feature_SSE3;

	if ( cpuinfo[ 2 ] & ( 1 << 9 ) || false )
		features |= ECPU_Feature_SSSE3;

	if ( cpuinfo[ 2 ] & ( 1 << 19 ) || false )
		features |= ECPU_Feature_SSE4_1;

	if ( cpuinfo[ 2 ] & ( 1 << 20 ) || false )
		features |= ECPU_Feature_SSE4_2;

	// ----------------------------------------------------------------------
	// Check AVX support
	
	// References
	// http://software.intel.com/en-us/blogs/2011/04/14/is-avx-enabled/
	// http://insufficientlycomplicated.wordpress.com/2011/11/07/detecting-intel-advanced-vector-extensions-avx-in-visual-studio/

	bool avxSupported     = cpuinfo[ 2 ] & ( 1 << 28 ) || false;
	bool osxsaveSupported = cpuinfo[ 2 ] & ( 1 << 27 ) || false;
	if ( osxsaveSupported && avxSupported )
	{
		// _XCR_XFEATURE_ENABLED_MASK = 0
		unsigned long long xcrFeatureMask = _xgetbv( 0 );
		avxSupported                      = ( xcrFeatureMask & 0x6 ) == 0x6;
	}

	if ( avxSupported )
		features |= ECPU_Feature_AVX;

	// ----------------------------------------------------------------------
	// Check SSE4a and SSE5 support

	// Get the number of valid extended IDs
	__cpuid( cpuinfo, 0x80000000 );
	int numExtendedIds = cpuinfo[ 0 ];
	if ( numExtendedIds >= 0x80000001 )
	{
		__cpuid( cpuinfo, 0x80000001 );

		if ( cpuinfo[ 2 ] & ( 1 << 6 ) || false )
			features |= ECPU_Feature_SSE4a;

		if ( cpuinfo[ 2 ] & ( 1 << 11 ) || false )
			features |= ECPU_Feature_SSE5;
	}

	// ----------------------------------------------------------------------
	// Check AVX2 Support

	return features;
}
#endif


#include <stdint.h>
#if defined( _MSC_VER )
	#include <intrin.h>
#endif


void run_cpuid( uint32_t eax, uint32_t ecx, int32_t* abcd )
{
	#if defined( _MSC_VER )
	__cpuidex( abcd, eax, ecx );
	#else
	uint32_t ebx, edx;
		#if defined( __i386__ ) && defined( __PIC__ )
	/* in case of PIC under 32-bit EBX cannot be clobbered */
	__asm__( "movl %%ebx, %%edi \n\t cpuid \n\t xchgl %%ebx, %%edi" : "=D"( ebx ),
		#else
	__asm__( "cpuid" : "+b"( ebx ),
		#endif
	                                                                  "+a"( eax ), "+c"( ecx ), "=d"( edx ) );
	abcd[ 0 ] = eax;
	abcd[ 1 ] = ebx;
	abcd[ 2 ] = ecx;
	abcd[ 3 ] = edx;
	#endif
}


int check_xcr0_ymm()
{
	uint32_t xcr0;
	#if defined( _MSC_VER )
	xcr0 = (uint32_t)_xgetbv( 0 ); /* min VS2010 SP1 compiler is required */
	#else
	__asm__( "xgetbv" : "=a"( xcr0 ) : "c"( 0 ) : "%edx" );
	#endif
	return ( ( xcr0 & 6 ) == 6 ); /* checking if xmm and ymm state are enabled in XCR0 */
}


int check_4th_gen_intel_core_features()
{
	s32 abcd[ 4 ];
	uint32_t fma_movbe_osxsave_mask = ( ( 1 << 12 ) | ( 1 << 22 ) | ( 1 << 27 ) );
	uint32_t avx2_bmi12_mask        = ( 1 << 5 ) | ( 1 << 3 ) | ( 1 << 8 );
	/* CPUID.(EAX=01H, ECX=0H):ECX.FMA[bit 12]==1 &&
CPUID.(EAX=01H, ECX=0H):ECX.MOVBE[bit 22]==1 &&
CPUID.(EAX=01H, ECX=0H):ECX.OSXSAVE[bit 27]==1 */
	run_cpuid( 1, 0, abcd );
	if ( ( abcd[ 2 ] & fma_movbe_osxsave_mask ) != fma_movbe_osxsave_mask )
		return 0;
	if ( !check_xcr0_ymm() )
		return 0;

	/* CPUID.(EAX=07H, ECX=0H):EBX.AVX2[bit 5]==1 &&
CPUID.(EAX=07H, ECX=0H):EBX.BMI1[bit 3]==1 &&
CPUID.(EAX=07H, ECX=0H):EBX.BMI2[bit 8]==1 */

	run_cpuid( 7, 0, abcd );
	if ( ( abcd[ 1 ] & avx2_bmi12_mask ) != avx2_bmi12_mask )
		return 0;
	/* CPUID.(EAX=80000001H):ECX.LZCNT[bit 5]==1 */
	run_cpuid( 0x80000001, 0, abcd );
	if ( ( abcd[ 2 ] & ( 1 << 5 ) ) == 0 )
		return 0;
	return 1;
}


// static int can_use_intel_core_4th_gen_features()
// {
// 	static int the_4th_gen_features_available = -1;
// 	/* test is performed once */
// 	if ( the_4th_gen_features_available < 0 )
// 		the_4th_gen_features_available = check_4th_gen_intel_core_features();
// }




#include <iostream>
#include <string>
#include <cinttypes>
#include <cstring>
#include <memory>
#include <regex>

void cpuidex( int* reg, const int eax, const int ecx = 0 )
{
#ifdef _WIN64
	#include <intrin.h>
	__cpuidex( reg, eax, ecx );
#else
	__asm__ volatile(
	  "cpuid\n\t"
	  : "=a"( reg[0] ), "=b"( reg[1] ), "=c"( reg[2] ), "=d"( reg[3] )
	  : "a"( eax ), "c"( ecx )
	  : );
#endif  // _WIN64
}

/*
register order
EAX
EBX
ECX
EDX
*/


struct ExtendedFeatures
{
	uint32_t fsgsbase : 1;
	uint32_t : 1;
	uint32_t sgx : 1;
	uint32_t bmi : 1;
	uint32_t hle : 1;
	uint32_t avx2 : 1;
	uint32_t : 1;
	uint32_t smep : 1;
	uint32_t bmi2 : 1;
	uint32_t erms : 1;
	uint32_t invpcid : 1;
	uint32_t rtm : 1;
	uint32_t pqm : 1;
	uint32_t : 1;
	uint32_t mpx : 1;
	uint32_t pqe : 1;
	uint32_t avx512f : 1;
	uint32_t avx512dq : 1;
	uint32_t rdseed : 1;
	uint32_t adx : 1;
	uint32_t smap : 1;
	uint32_t avx512ifma : 1;
	uint32_t pcommit : 1;
	uint32_t clflushopt : 1;
	uint32_t clwb : 1;
	uint32_t intel_pt : 1;
	uint32_t avx512pf : 1;
	uint32_t avx512er : 1;
	uint32_t avx512cd : 1;
	uint32_t sha : 1;
	uint32_t avx512bw : 1;
	uint32_t avx512vl : 1;
	uint32_t prefetchwt1 : 1;
	uint32_t avx512vbmi : 1;
	uint32_t pku : 1;
	uint32_t ospke : 1;

	ExtendedFeatures()
	{
		int b[ 4 ];
		cpuidex( b, 7, 0 );
		std::memcpy( this, &b[ 1 ], sizeof( uint32_t ) );
		std::memcpy( (int8_t*)this + sizeof( uint32_t ), &b[ 2 ], sizeof( uint32_t ) );
	}

	bool isAvx2()
	{
		return this->avx2;
	}

	bool isAvx512f()
	{
		return this->avx512f;
	}

	bool isAvx512vbmi()
	{
		return this->avx512vbmi;
	}
};

struct CpuCore
{
	uint32_t steppingID : 4;
	uint32_t model : 4;
	uint32_t familyId : 4;
	uint32_t processorType : 2;
	uint32_t : 2;
	uint32_t extendedModelId : 4;
	uint32_t extendedFamillyId : 8;
	uint32_t : 4;

	uint32_t brandIndex : 8;
	uint32_t clflushLineSize : 8;
	uint32_t NbLogicalProcessor : 8;
	uint32_t localApicId : 8;

	uint32_t : 32;

	uint32_t : 32;

	uint32_t cacheType : 5;
	uint32_t cachelevel : 3;
	uint32_t selfInitialiazingCacheLevel : 1;
	uint32_t fullyAssociateCache : 1;
	uint32_t : 4;
	uint32_t nbThreadSharingCache : 12;
	uint32_t nbProcessorCoreInThisDie : 6;

	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;

	CpuCore()
	{
		int b[ 4 ];
		cpuidex( b, 1 );
		std::memcpy( (int8_t*)this + 0 * sizeof( uint32_t ), &b[ 0 ], sizeof( uint32_t ) );
		std::memcpy( (int8_t*)this + 1 * sizeof( uint32_t ), &b[ 1 ], sizeof( uint32_t ) );
		std::memcpy( (int8_t*)this + 2 * sizeof( uint32_t ), &b[ 2 ], sizeof( uint32_t ) );
		std::memcpy( (int8_t*)this + 3 * sizeof( uint32_t ), &b[ 3 ], sizeof( uint32_t ) );
		cpuidex( b, 4 );
		std::memcpy( (int8_t*)this + 4 * sizeof( uint32_t ), &b[ 0 ], sizeof( uint32_t ) );
		std::memcpy( (int8_t*)this + 5 * sizeof( uint32_t ), &b[ 1 ], sizeof( uint32_t ) );
		std::memcpy( (int8_t*)this + 6 * sizeof( uint32_t ), &b[ 2 ], sizeof( uint32_t ) );
		std::memcpy( (int8_t*)this + 7 * sizeof( uint32_t ), &b[ 3 ], sizeof( uint32_t ) );
	}
};


class Basic
{
	std::string vendorID;
	uint32_t    maxLeaf;
	uint32_t    maxLeafExtended;

   public:
	Basic()
	{
		int b[ 4 ];
		cpuidex( b, 0 );
		vendorID.append( (char*)( b + 1 ), 4 );
		vendorID.append( (char*)( b + 3 ), 4 );
		vendorID.append( (char*)( b + 2 ), 4 );
		maxLeaf = b[ 0 ];

		cpuidex( b, 0x80000000 );
		maxLeafExtended = b[ 0 ];
	}

	uint32_t getMaxLeaf()
	{
		return maxLeaf;
	}

	uint32_t getMaxLeafExtended()
	{
		return maxLeafExtended;
	}

	std::string getVendorID()
	{
		return vendorID;
	}
};

class CpuInfo
{
	Basic                               basic;
	std::unique_ptr< ExtendedFeatures > features;
	std::unique_ptr< CpuCore >          cpuCore;
	std::string                         brand;

   public:
	CpuInfo() :
		features( nullptr )
	{
		if ( basic.getMaxLeaf() >= 7 )
		{
			features.reset( new ExtendedFeatures() );
		}

		if ( basic.getMaxLeafExtended() >= 0x80000004 )
		{
			int reg[ 4 ];

			for ( int leaf : { 0x80000002, 0x80000003, 0x80000004 } )
			{
				cpuidex( reg, leaf );

				for ( int regNumber : { 0, 1, 2, 3 } )
				{
					brand.append( (char*)( reg + regNumber ), 4 );
				}
			}
		}

		if ( basic.getMaxLeaf() >= 4 )
		{
			cpuCore.reset( new CpuCore() );
		}
	}

	std::string getBrand()
	{
		return brand;
	}

	std::string getVendorID()
	{
		return basic.getVendorID();
	}

	bool isAvx2()
	{
		if ( !features )
		{
			return false;
		}
		return features->avx2;
	}

	bool isAvx512f()
	{
		if ( !features )
		{
			return false;
		}
		return features->avx512f;
	}

	int getNumberCore()
	{
		if ( !cpuCore || basic.getVendorID() != "GenuineIntel" )
		{
			return -1;
		}
		return this->cpuCore->nbProcessorCoreInThisDie + 1;
	}

	int getNumberLogicalProccesor()
	{
		if ( !cpuCore )
		{
			return -1;
		}
		return this->cpuCore->NbLogicalProcessor;
	}

	float getFrequency()
	{
		using namespace std::literals::string_literals;
		std::regex  regex( R"(([0-9]{1,}[\.]{0,1}[0-9]{1,})(GHz|MHz|THz))"s );
		std::smatch match;

		if ( std::regex_search( brand, match, regex ) )
		{
			return std::stof( match[ 1 ].str() );
		}
		else
		{
			return 0;
		}
	}
};

int test_other_cpu_info()
{
	//CpuInfo cpuInfo;
	//
	//std::cout << cpuInfo.getVendorID() << std::endl;
	//std::cout << cpuInfo.getBrand() << std::endl;
	//std::cout << "Avx2 : " << ( cpuInfo.isAvx2() ? "True" : "False" ) << std::endl;
	//std::cout << "Avx512f : " << ( cpuInfo.isAvx512f() ? "True" : "False" ) << std::endl;
	//
	//
	//std::cout << "Number Logical Proccesor : " << cpuInfo.getNumberLogicalProccesor() << std::endl;
	//std::cout << "Number Core : " << cpuInfo.getNumberCore() << std::endl;
	//
	//std::cout << "Frequency : " << cpuInfo.getFrequency() << std::endl;

	return 0;
}







struct cpu_id_t
{
	u32 ebx;
	u32 eax;
	u32 ecx;
	u32 edx;
};


cpu_id_t Sys_GetCpuID( u32 funcId, u32 subFuncId )
{
	cpu_id_t cpu_id{};
	u32      regs[ 4 ];

#ifdef _WIN32
	::__cpuidex( (int*)regs, (int)funcId, (int)subFuncId );
#else
	asm volatile( "cpuid" : "=a"( regs[0] ), "=b"( regs[1] ), "=c"( regs[2] ), "=d"( regs[3] )
	              : "a"( funcId ), "c"( subFuncId ) );
	// ECX is set to zero for CPUID function 4
#endif

	cpu_id.ebx = regs[ 1 ];
	cpu_id.eax = regs[ 0 ];
	cpu_id.ecx = regs[ 2 ];
	cpu_id.edx = regs[ 3 ];

	return cpu_id;
}


// Bit positions for data extractions
constexpr uint32_t SSE_POS     = 0x02000000;
constexpr uint32_t SSE2_POS    = 0x04000000;
constexpr uint32_t SSE3_POS    = 0x00000001;
constexpr uint32_t SSE41_POS   = 0x00080000;
constexpr uint32_t SSE42_POS   = 0x00100000;
constexpr uint32_t AVX_POS     = 0x10000000;
constexpr uint32_t AVX2_POS    = 0x00000020;
constexpr uint32_t AVX512F_POS = 1u << 15;  // Bit 16
constexpr uint32_t LVL_NUM     = 0x000000FF;
constexpr uint32_t LVL_TYPE    = 0x0000FF00;
constexpr uint32_t LVL_CORES   = 0x0000FFFF;


// https://github.com/VioletGiraffe/cpuid-parser/blob/main/cpuinfo.hpp
cpu_info_t Sys_GetCpuInfo()
{
	test_other_cpu_info();

	static cpu_info_t cpu_info{};

	// did we already calculate this?
	if ( cpu_info.model_name )
		return cpu_info;

	// no, we need to get all the cpu info we need
	// Get vendor name EAX=0

	cpu_id_t       cpu_id_0 = Sys_GetCpuID( 0, 0 );

	const uint32_t HFS      = cpu_id_0.eax;

	// Reinterpret bytes as ASCII characters
	const char*    vendor_id_name[ 3 ] = { (char*)&cpu_id_0.ebx, (char*)&cpu_id_0.edx, (char*)&cpu_id_0.ecx };
	size_t         vendor_id_len[ 3 ]  = { 4, 4, 4 };
	cpu_info.vendor_id                 = ch_str_concat( 3, vendor_id_name, vendor_id_len ).data;

	// Get SSE instructions availability
	cpu_id_t cpu_id_1                  = Sys_GetCpuID( 1, 0 );
	bool     is_htt                     = ( cpu_id_1.edx & AVX_POS );

	if ( cpu_id_1.edx & SSE_POS )
		cpu_info.features |= ECPU_Feature_SSE;

	if ( cpu_id_1.edx & SSE2_POS )
		cpu_info.features |= ECPU_Feature_SSE2;

	if ( cpu_id_1.ecx & SSE3_POS )
		cpu_info.features |= ECPU_Feature_SSE3;

	if ( cpu_id_1.ecx & SSE41_POS )
		cpu_info.features |= ECPU_Feature_SSE4_1;

	// same as above????
	if ( cpu_id_1.ecx & SSE41_POS )
		cpu_info.features |= ECPU_Feature_SSE4_2;

	if ( cpu_id_1.ecx & AVX_POS )
		cpu_info.features |= ECPU_Feature_AVX;

	// Get AVX2 instructions availability
	cpu_id_t cpu_id_7             = Sys_GetCpuID( 7, 0 );

	if ( cpu_id_7.ebx & AVX2_POS )
		cpu_info.features |= ECPU_Feature_AVX2;

	if ( cpu_id_7.ebx & AVX512F_POS )
		cpu_info.features |= ECPU_Feature_AVX512_F;

	std::string vendorIdUppercase = cpu_info.vendor_id;
	str_upper( vendorIdUppercase );

	// Get num of cores
	if ( vendorIdUppercase.find( "INTEL" ) != std::string::npos )
	{
		if ( HFS >= 11 )
		{
			static constexpr int MAX_INTEL_TOP_LVL = 4;
			for ( int lvl = 0; lvl < MAX_INTEL_TOP_LVL; ++lvl )
			{
				cpu_id_t cpu_id_4  = Sys_GetCpuID( 0x0B, lvl );

				uint32_t currLevel = ( LVL_TYPE & cpu_id_4.ecx ) >> 8;
				switch ( currLevel )
				{
					case 0x01: cpu_info.num_smt = LVL_CORES & cpu_id_4.ebx; break;           //  EAX=0xB, ECX=0 - EBX is the number of logical processors (threads) per core
					case 0x02: cpu_info.num_logical_cpus = LVL_CORES & cpu_id_4.ebx; break;  // EAX=0xB, ECX=1 - EBX is the number of logical processors per processor package
					default: break;
				}
			}
			cpu_info.num_cores = cpu_info.num_logical_cpus / cpu_info.num_smt;
			is_htt             = cpu_info.num_smt > 1;
		}
		else
		{
			if ( HFS >= 1 )
			{
				cpu_info.num_logical_cpus = ( cpu_id_1.ebx >> 16 ) & 0xFF;
				if ( HFS >= 4 )
				{
					cpu_info.num_cores = 1 + ( Sys_GetCpuID( 4, 0 ).eax >> 26 ) & 0x3F;
				}
			}
			if ( is_htt )
			{
				if ( !( cpu_info.num_cores > 1 ) )
				{
					cpu_info.num_cores        = 1;
					cpu_info.num_logical_cpus = ( cpu_info.num_logical_cpus >= 2 ? cpu_info.num_logical_cpus : 2 );
				}
			}
			else
			{
				cpu_info.num_cores = cpu_info.num_logical_cpus = 1;
			}
		}
	}
	else if ( vendorIdUppercase.find( "AMD" ) != std::string::npos )
	{
		// From https://github.com/time-killer-games/ween/blob/db69cafca2222c634a1d3a9e58262b5a2dc8d508/system.cpp#L1469-L1528
		cpu_info.num_smt = 1 + ( ( Sys_GetCpuID( 0x8000001e, 0 ).ebx >> 8 ) & 0xff );
		if ( cpu_info.num_logical_cpus > 0 && cpu_info.num_smt > 0 )
		{
			cpu_info.num_cores = cpu_info.num_logical_cpus / cpu_info.num_smt;
		}
		else
		{
			if ( HFS >= 1 )
			{
				if ( Sys_GetCpuID( 0x80000000, 0 ).eax >= 8 )
				{
					cpu_info.num_cores = 1 + ( Sys_GetCpuID( 0x80000008, 0 ).ecx & 0xFF );
				}
			}
			if ( is_htt )
			{
				if ( cpu_info.num_cores < 1 )
				{
					cpu_info.num_cores = 1;
				}
			}
			else
			{
				cpu_info.num_cores = 1;
			}
		}
	}
	else
	{
		Log_ErrorF( "Unknown CPU vendor - Reported vendor name is: %s", cpu_info.vendor_id );
	}

	if ( is_htt )
	{
		cpu_info.features |= ECPU_Feature_HTT;
	}

	// Get processor brand string
	// This seems to be working for both Intel & AMD vendors

	std::string modelName;
	for ( int i = 0x80000002; i < 0x80000005; ++i )
	{
		cpu_id_t cpu_id = Sys_GetCpuID( i, 0 );

		modelName += std::string( (const char*)&cpu_id.eax, 4 );
		modelName += std::string( (const char*)&cpu_id.ebx, 4 );
		modelName += std::string( (const char*)&cpu_id.ecx, 4 );
		modelName += std::string( (const char*)&cpu_id.edx, 4 );
	}

	if ( modelName.size() )
		cpu_info.model_name = ch_str_copy( modelName.data(), modelName.size() ).data;

	return cpu_info;



}
