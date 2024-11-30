
option( STEAM "Build the steam abstraction" OFF )
# option( GAME_OLD "Build the old game code" OFF )
option( GAME "Build the game code" ON )
option( TOOLKIT "Build the toolkit" ON )
option( RENDER_TEST "Build Render 3 Test App" ON )

# Add Chocolate Framework DLLs
# add_subdirectory( ../chocolate ${CMAKE_CURRENT_LIST_DIR} )
# add_subdirectory( ${CMAKE_CURRENT_LIST_DIR}/../chocolate ${CMAKE_CURRENT_LIST_DIR}/../chocolate )

message( "" )
message( "----------------------- Apps -----------------------" )

# grab the globals and put them here
get_property( CH_ROOT GLOBAL PROPERTY CH_ROOT )
get_property( CH_PUBLIC GLOBAL PROPERTY CH_PUBLIC )
get_property( CH_THIRDPARTY GLOBAL PROPERTY CH_THIRDPARTY )

include( ${CH_ROOT}/scripts/core.cmake )

# Now Add Sidury Projects
add_subdirectory( src/launcher )

if ( GAME )
	add_subdirectory( src/game )
endif( GAME )

if( TOOLKIT )
	add_subdirectory( src/toolkit )
endif( TOOLKIT )

if ( RENDER_TEST )
 	add_subdirectory( src/render_test )
endif( RENDER_TEST )

if( STEAM )
	add_subdirectory( src/steam )
endif( STEAM )

