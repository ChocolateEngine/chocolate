cmake_minimum_required( VERSION 3.18 FATAL_ERROR )

# make these global for all projects, though they need to use get_property to get these
set_property( GLOBAL PROPERTY CH_ROOT           ${CMAKE_CURRENT_LIST_DIR} )
set_property( GLOBAL PROPERTY CH_PUBLIC         ${CMAKE_CURRENT_LIST_DIR}/public )
set_property( GLOBAL PROPERTY CH_THIRDPARTY     ${CMAKE_CURRENT_LIST_DIR}/thirdparty )

set( CH_SRC ${CMAKE_CURRENT_LIST_DIR}/src )

# grab the globals and put them here for chocolate's projects
get_property( CH_ROOT GLOBAL PROPERTY CH_ROOT )
get_property( CH_PUBLIC GLOBAL PROPERTY CH_PUBLIC )
get_property( CH_THIRDPARTY GLOBAL PROPERTY CH_THIRDPARTY )

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )
set( BUILD_CHOCOLATE ON )


if( NOT DEFINED GAME_DIR )
  message( FATAL_ERROR "GAME_DIR var is not defined or set!!" )
endif()

include( scripts/core.cmake )

# Chocolate Shared Objects
if ( BUILD_CHOCOLATE STREQUAL ON ) 
  add_subdirectory( ${CH_SRC}/core )
  add_subdirectory( ${CH_SRC}/imgui )
  add_subdirectory( ${CH_SRC}/engine )
  add_subdirectory( ${CH_SRC}/audio )
  add_subdirectory( ${CH_SRC}/graphics )
  endif()
