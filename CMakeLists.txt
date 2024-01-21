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

get_property( CH_BIN GLOBAL PROPERTY CH_BIN )

# Chocolate Shared Objects
add_subdirectory( ${CH_SRC}/core )
add_subdirectory( ${CH_SRC}/imgui )
add_subdirectory( ${CH_SRC}/audio )
# add_subdirectory( ${CH_SRC}/graphics )
add_subdirectory( ${CH_SRC}/graphics2 )
add_subdirectory( ${CH_SRC}/gui )
add_subdirectory( ${CH_SRC}/input )
add_subdirectory( ${CH_SRC}/physics )

# Public Files
# message( "Setting Up Source Groups for Public Folder" )
# file(GLOB_RECURSE ALL_PUBLIC_FILES CONFIGURE_DEPENDS "${CH_PUBLIC}/*")
# source_group(
# 	TREE ${CH_PUBLIC}
# 	PREFIX "Public"
# 	FILES ${ALL_PUBLIC_FILES}
# )

# Thirdparty Files
# message( "Setting Up Source Groups for Thirdparty Folder" )
# file(GLOB_RECURSE THIRDPARTY_FILES CONFIGURE_DEPENDS "${CH_THIRDPARTY}/*")
# source_group(
# 	TREE ${CH_PUBLIC}
# 	PREFIX "Thirdparty"
# 	FILES ${CH_THIRDPARTY}
# )

# copy SDL2 over on windows

if( MSVC )
	message( "Copying SDL2" )
	if ( ${64_BIT} )
		configure_file( "${CH_THIRDPARTY}/SDL2/lib/x64/SDL2.dll" "${CH_BIN}/SDL2.dll" COPYONLY )
	else()
		configure_file( "${CH_THIRDPARTY}/SDL2/lib/x86/SDL2.dll" "${CH_BIN}/SDL2.dll" COPYONLY )
	endif()
endif()
