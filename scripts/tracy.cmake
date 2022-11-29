
set( TRACY_DIR "${CH_THIRDPARTY}/Tracy/public" )

include_directories( ${TRACY_DIR} ${TRACY_DIR}/tracy )

if( NOT DEFINED USE_TRACY )
	set( USE_TRACY 0 )
endif()

if( USE_TRACY )
	message( "Compiling With Tracy Support" )
	# add_compile_definitions( TRACY_ENABLE=1 TRACY_IMPORTS TRACY_CALLSTACK=24 )
	add_compile_definitions( TRACY_ENABLE=1 TRACY_IMPORTS )
	
	set(
		TRACY_FILES
		${TRACY_DIR}/TracyClient.cpp
		${TRACY_DIR}/tracy/Tracy.hpp
	)
endif()

