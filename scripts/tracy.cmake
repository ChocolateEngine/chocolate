
set( TRACY_DIR "${CH_THIRDPARTY}/tracy" )

include_directories( ${TRACY_DIR} )

# tracy is enabled by default for debug, at least for now
# if ( CMAKE_BUILD_TYPE STREQUAL Debug AND NOT DEFINED NO_TRACY )
# 	set( USE_TRACY 1 )
# endif()

# if( DEFINED USE_TRACY )
# 	message( "Compiling With Tracy Support" )
# 	add_compile_definitions( TRACY_ENABLE=1 TRACY_IMPORTS TRACY_CALLSTACK=24 )
# endif()

# set(
# 	TRACY_FILES
# 	${TRACY_DIR}/TracyClient.cpp
# 	${TRACY_DIR}/Tracy.hpp
# )
