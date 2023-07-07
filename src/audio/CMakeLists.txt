message( "Current Project: Aduio" )

set( OPENAL_DIR ${CH_THIRDPARTY}/openal-soft )

message( OPENAL_DIR = ${OPENAL_DIR} )

file(
	GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
	*.cpp
	*.h
)

file(
	#GLOB_RECURSE PUBLIC_FILES CONFIGURE_DEPENDS
	GLOB PUBLIC_FILES CONFIGURE_DEPENDS
	${CH_PUBLIC}/*.cpp
	${CH_PUBLIC}/*.h
	${CH_PUBLIC}/core/*.h
)

link_libraries( Core )

add_compile_definitions(
	"ENABLE_AUDIO=1"
	"ENABLE_WAV=0"
	"ENABLE_VORBIS=1"
)

include_directories(
	"${OPENAL_DIR}/include"
	"${CH_THIRDPARTY}/libogg/include"
	"${CH_THIRDPARTY}/libvorbis/include"
	if( MSVC )
		"${CH_THIRDPARTY}/libwav/include"
	endif()
)

if ( CMAKE_BUILD_TYPE EQUAL Debug )
	link_directories(
		"${OPENAL_DIR}/build/Debug"
	)
else()
	link_directories(
		"${OPENAL_DIR}/build/Release"
	)
endif()

if( MSVC )
	link_libraries(
		"OpenAL32"
	)
	
	# stupdi
	if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
		link_libraries(
			"${CH_THIRDPARTY}/libvorbis/win32/VS2010/x64/Debug/libvorbis_static.lib"
			"${CH_THIRDPARTY}/libvorbis/win32/VS2010/x64/Debug/libvorbisfile_static.lib"
			"${CH_THIRDPARTY}/libogg/win32/VS2015/x64/Debug/libogg.lib"
		)
	else()
		link_libraries(
			"${CH_THIRDPARTY}/libvorbis/win32/VS2010/x64/Release/libvorbis_static.lib"
			"${CH_THIRDPARTY}/libvorbis/win32/VS2010/x64/Release/libvorbisfile_static.lib"
			"${CH_THIRDPARTY}/libogg/win32/VS2015/x64/Release/libogg.lib"
		)
	endif()
	
	add_link_options( "/LTCG" )
else()
	link_libraries(
		"libOpenAL32.so"
		"vorbis"
		"vorbisfile"
	)
endif()


if( MSVC )
	#set( WAV_FILES "${CH_THIRDPARTY}/libwav/src/wav.c" "${CH_THIRDPARTY}/libwav/include/wav.h" )
	set( WAV_FILES )
else()
	set( WAV_FILES )
endif()


# quite amusing
add_library(
	Aduio SHARED
	${SRC_FILES}
	${PUBLIC_FILES}
	${WAV_FILES}
)

add_dependencies( Aduio "Core" )

set_target_properties(
	Aduio PROPERTIES
	OUTPUT_NAME ch_aduio
	PREFIX ""
)

target_precompile_headers( Aduio PRIVATE "${CH_PUBLIC}/core/core.h" )

message( "Copying OpenAL32 DLL" )

if( MSVC )
	configure_file( "${OPENAL_DIR}/build/Release/OpenAL32.dll" "${CH_BUILD}/OpenAL32.dll" COPYONLY )
else()
	configure_file( "${OPENAL_DIR}/build/Release/libOpenAL32.so" "${CH_BUILD}/libOpenAL32.so" COPYONLY )
endif()

source_group(
	TREE ${CMAKE_CURRENT_LIST_DIR}
	PREFIX "Source Files"
	FILES ${SRC_FILES}
)

source_group(
	TREE ${CH_PUBLIC}
	PREFIX "Public"
	FILES ${PUBLIC_FILES}
)
