message( "Current Project: Steam Abstraction" )

set( STEAM_API_DIR "${CMAKE_CURRENT_LIST_DIR}/steamworks_sdk_157/sdk" )

if( WIN32 )
	if( ${64_BIT} )
		set( STEAM_API_BIN_DIR "${STEAM_API_DIR}/redistributable_bin/win64" )
	else()
		set( STEAM_API_BIN_DIR "${STEAM_API_DIR}/redistributable_bin/win32" )
	endif()
else()
	set( STEAM_API_BIN_DIR "${STEAM_API_DIR}/redistributable_bin/linux64" )
endif()

set(
	SRC_FILES
	
	steam.cpp
	steam.h
	
	ch_isteam.h
)

file(
	GLOB_RECURSE PUBLIC_FILES CONFIGURE_DEPENDS
	${CH_PUBLIC}/*.cpp
	${CH_PUBLIC}/*.h
)

include_directories( "${STEAM_API_DIR}/public/steam" )
link_directories( "${STEAM_API_BIN_DIR}" )

if( WIN32 AND ${64_BIT} )
	link_libraries(
		steam_api64
	)
else()
	link_libraries(
		steam_api
	)
endif()

add_library( Steam SHARED ${SRC_FILES} ${PUBLIC_FILES} )

target_link_libraries(
	Steam
	PRIVATE
	Core
	SDL2
)

add_dependencies( Steam "Core" )

set_target_properties(
	Steam PROPERTIES
	RUNTIME_OUTPUT_NAME ch_steam
	LIBRARY_OUTPUT_NAME ch_steam

	RUNTIME_OUTPUT_DIRECTORY ${CH_OUTPUT}/sidury/bin/${PLAT_FOLDER}
	LIBRARY_OUTPUT_DIRECTORY ${CH_OUTPUT}/sidury/bin/${PLAT_FOLDER}
)

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set_target_properties(
    	Steam PROPERTIES
    	RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_OUTPUT}/sidury/bin/${PLAT_FOLDER}
    	LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_OUTPUT}/sidury/bin/${PLAT_FOLDER}
    )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

target_precompile_headers( Steam PRIVATE "${CH_PUBLIC}/core/core.h" )

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

if( WIN32 AND ${64_BIT} )
	add_custom_command(
		TARGET Steam POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy "${STEAM_API_BIN_DIR}/steam_api64.dll" "${CH_BIN}/steam_api64.dll"
		COMMAND_EXPAND_LISTS
        COMMENT "Copying Steam API DLL to launcher directory"
    )
endif()

if( WIN32 AND ${32_BIT} )
	add_custom_command(
		TARGET Steam POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy "${STEAM_API_BIN_DIR}/steam_api.dll" "${CH_BIN}/steam_api.dll"
		COMMAND_EXPAND_LISTS
        COMMENT "Copying Steam API 32-bit DLL to launcher directory"
    )
endif()

