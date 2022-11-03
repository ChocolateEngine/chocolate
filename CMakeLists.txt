cmake_minimum_required( VERSION 3.18 FATAL_ERROR )

# make these global for all projects, though they need to use get_property to get these
set_property( GLOBAL PROPERTY CH_ROOT           ${CMAKE_CURRENT_LIST_DIR} )
set_property( GLOBAL PROPERTY CH_PUBLIC         ${CMAKE_CURRENT_LIST_DIR}/public )
set_property( GLOBAL PROPERTY CH_THIRDPARTY     ${CMAKE_CURRENT_LIST_DIR}/thirdparty )
set_property( GLOBAL PROPERTY CH_BUILD          ${CMAKE_CURRENT_LIST_DIR}/../output )

set( CH_SRC ${CMAKE_CURRENT_LIST_DIR}/src )

# grab the globals and put them here for chocolate's projects
get_property( CH_ROOT GLOBAL PROPERTY CH_ROOT )
get_property( CH_PUBLIC GLOBAL PROPERTY CH_PUBLIC )
get_property( CH_THIRDPARTY GLOBAL PROPERTY CH_THIRDPARTY )
get_property( CH_BUILD GLOBAL PROPERTY CH_BUILD )

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

include( scripts/core.cmake )

# Chocolate Shared Objects
add_subdirectory( ${CH_SRC}/core )
add_subdirectory( ${CH_SRC}/imgui )
add_subdirectory( ${CH_SRC}/audio )
add_subdirectory( ${CH_SRC}/graphics )
add_subdirectory( ${CH_SRC}/graphics2 )
add_subdirectory( ${CH_SRC}/gui )
add_subdirectory( ${CH_SRC}/input )
add_subdirectory( ${CH_SRC}/physics )

# copy SDL2 over on windows

if( MSVC )
	message( "Copying SDL2" )
	configure_file( "${CH_THIRDPARTY}/SDL2/lib/x64/SDL2.dll" "${CH_BUILD}/bin/SDL2.dll" COPYONLY )
	configure_file( "${CH_THIRDPARTY}/SDL2/lib/x64/SDL2.dll" "${CH_BUILD}/SDL2.dll" COPYONLY )  # dumb launcher thing, blech
endif()
