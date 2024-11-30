message( "Current Project: Aduio" )

set( STEAMAUDIO_DIR ${CH_THIRDPARTY}/steamaudio )

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
	"${STEAMAUDIO_DIR}/include"
	"${CH_THIRDPARTY}/libogg/include"
	"${CH_THIRDPARTY}/libvorbis/include"
	if( MSVC )
		"${CH_THIRDPARTY}/libwav/include"
	endif()
)

if( MSVC )
	link_libraries(
		"${STEAMAUDIO_DIR}/lib/windows-x64/phonon.lib"
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
	link_directories( "${STEAMAUDIO_DIR}/lib/linux-x64" )
	link_libraries(
		"libphonon.so"  # uh how does this work on cmake on linux
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

target_link_libraries(
	Aduio PRIVATE
	SDL2
)

target_precompile_headers( Aduio PRIVATE "${CH_PUBLIC}/core/core.h" )

# copy phonon over
message( "Copying Steam Audio DLL (phonon)" )

if( MSVC )
	configure_file( "${STEAMAUDIO_DIR}/lib/windows-x64/phonon.dll" "${CH_BIN}/phonon.dll" COPYONLY )
	configure_file( "${STEAMAUDIO_DIR}/lib/windows-x64/GPUUtilities.dll" "${CH_BIN}/GPUUtilities.dll" COPYONLY )
	configure_file( "${STEAMAUDIO_DIR}/lib/windows-x64/TrueAudioNext.dll" "${CH_BIN}/TrueAudioNext.dll" COPYONLY )
else()
	configure_file( "${STEAMAUDIO_DIR}/lib/linux-x64/libphonon.so" "${CH_BIN}/libphonon.so" COPYONLY )
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
