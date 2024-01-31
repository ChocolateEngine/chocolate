
# grab the framework global vars
get_property( CH_ROOT GLOBAL PROPERTY CH_ROOT )
get_property( CH_PUBLIC GLOBAL PROPERTY CH_PUBLIC )
get_property( CH_THIRDPARTY GLOBAL PROPERTY CH_THIRDPARTY )
get_property( CH_BUILD GLOBAL PROPERTY CH_BUILD )

set( CMAKE_POSITION_INDEPENDENT_CODE ON )
set( CMAKE_SHARED_LIBRARY_PREFIX "" )

set( CXX_STANDARD 20 )  # could do 23
set( CMAKE_CXX_STANDARD 20 )  # could do 23
set( CMAKE_CXX_STANDARD_REQUIRED ON )

if(CMAKE_CONFIGURATION_TYPES)
    message("Multi-configuration generator")
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;Profile" CACHE STRING "My multi config types" FORCE)
else()
    message("Single-configuration generator")
endif()

# set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;Profile")

if (${CMAKE_CL_64})
	set( 64_BIT 1 )
	set( 32_BIT 0 )
	
	if( MSVC )
		set( PLAT_FOLDER win64 )
	else()
		set( PLAT_FOLDER linux64 )
	endif()
else()
	set( 64_BIT 0 )
	set( 32_BIT 1 )
	
	if( MSVC )
		set( PLAT_FOLDER win32 )
	else()
		set( PLAT_FOLDER linux32 )
	endif()
endif()

set_property( GLOBAL PROPERTY CH_BIN            ${CH_BUILD}/bin/${PLAT_FOLDER} )
get_property( CH_BIN GLOBAL PROPERTY CH_BIN )

message( "CMAKE_GENERATOR: ${CMAKE_GENERATOR}" )
message( "CMAKE_SIZE_OF_VOID_P: ${CMAKE_SIZE_OF_VOID_P}" )
message( "CMAKE_GENERATOR_PLATFORM: ${CMAKE_GENERATOR_PLATFORM}" )
message( "" )
message( "CMAKE_CL_64:     ${CMAKE_CL_64}" )
message( "Is 64-bit:       ${64_BIT}" )
message( "Is 32-bit:       ${32_BIT}" )
message( "Platform Folder: ${PLAT_FOLDER}" )

message( CH_BIN ${CH_BIN} )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CH_BIN} )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CH_BIN} )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CH_ROOT}/obj )

# https://stackoverflow.com/questions/20638963/cmake-behaviour-custom-configuration-types-with-visual-studio-need-multiple-cma
# if( CMAKE_CONFIGURATION_TYPES )
#   set( CMAKE_CONFIGURATION_TYPES Release Debug )
#   set( CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
#       "Reset the configurations to what we need" FORCE )
# endif()

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    
    # set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_BUILD}/bin )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_BIN} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_BIN} )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_BUILD}/obj )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

set( CMAKE_SHARED_LINKER_FLAGS_PROFILE ${CMAKE_SHARED_LINKER_FLAGS_RELEASE} )
set( CMAKE_EXE_LINKER_FLAGS_PROFILE ${CMAKE_EXE_LINKER_FLAGS_RELEASE} )

if ( ${CMAKE_GENERATOR} MATCHES "Visual Studio .*" )
	set( IS_VISUAL_STUDIO 1 )
else()
	set( IS_VISUAL_STUDIO 0 )
endif()

set_property( GLOBAL PROPERTY PREFIX "" )
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

include_directories(
	"."
	${CH_PUBLIC}
	${CH_THIRDPARTY}
)

add_compile_definitions(
	# GLM_FORCE_DEPTH_ZERO_TO_ONE  # what does even this do internally
	# glm is SHIT and can't do this with AVX2, ugh
	GLM_FORCE_XYZW_ONLY=GLM_ENABLE
	GLM_FORCE_SWIZZLE
	GLM_DEPTH_ZERO_TO_ONE
	GLM_ENABLE_EXPERIMENTAL
	
	#GLM_CONFIG_SIMD  # defined in GLM_FORCE_AVX2

	#GLM_FORCE_INLINE
	#GLM_FORCE_SSE2 # or GLM_FORCE_SSE42 if your processor supports it
	#GLM_FORCE_SSE42
	#GLM_FORCE_ALIGNED
)


set( COMPILE_DEF_DEBUG "DEBUG=1" )
set( COMPILE_DEF_RELEASE "NDEBUG=1" )

add_compile_definitions(
	"$<$<CONFIG:Debug>:${COMPILE_DEF_DEBUG}>"
	"$<$<CONFIG:Release>:${COMPILE_DEF_RELEASE}>"
	"$<$<CONFIG:RelWithDebInfo>:${COMPILE_DEF_RELEASE}>"
	"$<$<CONFIG:Profile>:${COMPILE_DEF_RELEASE}>"
	
	CH_PLAT_FOLDER="${PLAT_FOLDER}"
	"CH_64_BIT=${64_BIT}"
	"CH_32_BIT=${32_BIT}"
)


set( MIMALLOC_DIR "${CH_THIRDPARTY}/mimalloc" )

include_directories( ${MIMALLOC_DIR}/include )

# Compiler/Platform specifc options
if( MSVC )

	set( MIMALLOC_BUILD "${MIMALLOC_DIR}/out/msvc-x64" )
	# set( MIMALLOC_BUILD "${MIMALLOC_DIR}/build" )

	#link_libraries(
	#	"$<$<CONFIG:Debug>:${MIMALLOC_DIR}/build/Debug/mimalloc-secure-debug.lib>"
	#	"$<$<CONFIG:Release>:${MIMALLOC_BUILD}/Release/mimalloc-override.lib>"
	#	"$<$<CONFIG:RelWithDebInfo>:${MIMALLOC_BUILD}/Release/mimalloc-override.lib>"
	#	"$<$<CONFIG:Profile>:${MIMALLOC_BUILD}/Release/mimalloc-override.lib>"
	#	${MIMALLOC_DIR}/bin/mimalloc-redirect.lib
	#)

	if ( ${64_BIT} )
		link_directories(
			${CH_THIRDPARTY}/SDL2/lib/x64
		)
	else()
		link_directories(
			${CH_THIRDPARTY}/SDL2/lib/x86
		)
	endif()
	
	include_directories(
		${CH_THIRDPARTY}/glm
		${CH_THIRDPARTY}/SDL2/include
	)

	add_compile_definitions(
		NOMINMAX
		_CRT_SECURE_NO_WARNINGS
		_ALLOW_RUNTIME_LIBRARY_MISMATCH _ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH _ALLOW_MSC_VER_MISMATCH
		CH_USE_MIMALLOC=0
	)

	if ( ${64_BIT} )
		add_compile_definitions(
			_AMD64_ __x86_64 __amd64
		)
	else()
		add_compile_definitions(
			_X86_ __x86_32
		)
	endif()
	
	# Use these runtime libraries for both so it doesn't crash smh my head
	# actually this is useless right now because of core.dll, god dammit
	# set_property( GLOBAL PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL" )
	
	set( CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" )
	set_property( GLOBAL PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" )

	set( MSVC_VERSION 1939 )

	add_compile_options(
		# "/Wall"             # Show ALL Warnings
		"/W4"               # Warning Level 4
		"/MP"               # multi-threaded compilation
		"/permissive-"      # make MSVC very compliant
		"/Zc:__cplusplus"   # set the __cplusplus macro to be the correct version
		# "/fp:fast"          # fast floating point model
		"/GF"               # string pooling: allows compiler to create a single copy of identical strings in the program image and in memory during execution
		
		# disable stupid warnings
		"/wd4244"  # conversion from 'X' to 'Y', possible loss of data
		"/wd4267"  # conversion from 'X' to 'Y', possible loss of data (yes there's 2 idk why)
		"/wd4305"  # truncation from 'double' to 'float'
		"/wd4464"  # relative include path contains ".."
		"/wd4668"  # 'X' is not defined as a preprocessor macro, replacing '0' for '#if/#elif' (floods with win32 stuff)
		"/wd4514"  # 'X' unreferenced inline function has been removed
		"/wd4505"  # 'X' unreferenced function with internal linkage has been removed
		"/wd4100"  # 'X' unreferenced formal paramter
		"/wd4201"  # nonstandard extension used: nameless struct/union (used in glm)
		"/wd5045"  # Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified

		"/wd4706"  # assignment within conditional expression
		
		# probably temporary
		"/wd4365"  # signed/unsigned mismatch
		"/wd4061"  # enumerator 'X' in switch of enum 'Y' is not explicitly handled by a case label
		
		# padding stuff
		"/wd4324"  # 'X' structure was padded due to alignment specifier
		"/wd4820"  # 'X' bytes padding added after data member 'Y'

		# Treat these warnings as errors

		# MSVC doesn't care about this one, but this errors on gcc and can help catch a mistake i made unknowingly
		"/we4002"  # Too many arguments for function-like macro invocation

		# I accidently get this a lot with printing std::string, so I want this to error so I know i made a mistake
		"/we4840"  # non-portable use of class 'type' as an argument to a variadic function

		# This is self-explanitory
		"/we4129"  # 'X' unrecognized character escape sequence
	)

	set( COMPILE_OPTIONS_DEBUG
		"/Od"        # no optimizations
		"/ZI"        # edit and continue
		"/fp:except" # raise floating point exceptions
		#"/fsanitize=address" # use address sanitizing in Debug (incompatible with /INCREMENTAL)
	)
	
	set( COMPILE_OPTIONS_RELEASE
		"/fp:fast"
	)
	
	set( COMPILE_OPTIONS_PROFILE
		"/fp:fast"
		"/Od"        # no optimizations
		"/Zi"        # debug information
	)
	
	add_compile_options(
		"$<$<CONFIG:Debug>:${COMPILE_OPTIONS_DEBUG}>"
		"$<$<CONFIG:Release>:${COMPILE_OPTIONS_RELEASE}>"
		"$<$<CONFIG:RelWithDebInfo>:${COMPILE_OPTIONS_RELEASE}>"
		"$<$<CONFIG:Profile>:${COMPILE_OPTIONS_PROFILE}>"
	)
	
	set( LINK_OPTIONS_DEBUG
		""
	)
	
	set( LINK_OPTIONS_RELEASE
		""
	)
	
	set( LINK_OPTIONS_PROFILE
		"/DEBUG"
	)
	
	add_link_options(
		"$<$<CONFIG:Debug>:${LINK_OPTIONS_DEBUG}>"
		"$<$<CONFIG:Release>:${LINK_OPTIONS_RELEASE}>"
		"$<$<CONFIG:RelWithDebInfo>:${LINK_OPTIONS_RELEASE}>"
		"$<$<CONFIG:Profile>:${LINK_OPTIONS_PROFILE}>"
	)
	
	add_link_options(
		"/INCREMENTAL"
	)
	
else()  # linux

	# set( MIMALLOC_BUILD "${MIMALLOC_DIR}/build" )
	# 
	# link_libraries(
	# 	"$<$<CONFIG:Debug>:${MIMALLOC_BUILD}/Debug/mimalloc-override.lib>"
	# 	"$<$<CONFIG:Release>:${MIMALLOC_BUILD}/Release/mimalloc-override.lib>"
	# 	"$<$<CONFIG:RelWithDebInfo>:${MIMALLOC_BUILD}/Release/mimalloc-override.lib>"
	# 	"$<$<CONFIG:Profile>:${MIMALLOC_BUILD}/Release/mimalloc-override.lib>"
	# )

	set( CMAKE_CXX_COMPILER g++ )
	
	include_directories(
		"/usr/include/SDL2"
	)

	# idk what's needed here or not
	link_libraries( dl pthread )
	
	if ( CMAKE_BUILD_TYPE STREQUAL Debug )
		add_compile_options( -g -fpermissive )
	else()
		add_compile_options( -O2 -fpermissive )
	endif()
	
	# From Thermite:
	# Added -Wno-unused-parameter because this warning as an error is %*$*ing dumb
	# Added -Wno-missing-field-initializers because designated initializers initialize to 0 anyway, so screw you
	# Added -Wno-deprecated-enum-enum-conversion because ImGui
	add_compile_options(
		-pedantic
		-Wno-unused-parameter
		-Wno-missing-field-initializers
		-Wno-deprecated-enum-enum-conversion
	)

	set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" ) 
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20" )
	
endif()

include( ${CH_ROOT}/scripts/tracy.cmake )






# set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}"     CACHE STRING "List of supported configurations.")
# 
# mark_as_advanced(CMAKE_CONFIGURATION_TYPES)
# 
# if(NOT CMAKE_BUILD_TYPE)
# 	message("Defaulting to release build.")
# 	set(CMAKE_BUILD_TYPE Release         CACHE STRING         "Choose the type of build, options are: ${CMAKE_CONFIGURATION_TYPES}."         FORCE)
# endif()

