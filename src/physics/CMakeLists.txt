message( "Current Project: Physics" )

include( ${CH_ROOT}/scripts/joltphysics.cmake )

file(
	GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
	*.cpp
	*.h
)

file(
	GLOB_RECURSE PUBLIC_FILES CONFIGURE_DEPENDS
	${CH_PUBLIC}/*.cpp
	${CH_PUBLIC}/*.h
)

link_libraries( Core )

# this is really stupid
if ( MSVC )
	set( COMPILE_OPTION_MT_DEBUG "/MTd" )
	set( COMPILE_OPTION_MT_RELEASE "/MT" )

	# NOTE: won't work for everything, really dumb
	add_compile_options(
	    "$<$<CONFIG:Debug>:${COMPILE_OPTION_MT_DEBUG}>"
	    "$<$<CONFIG:Release>:${COMPILE_OPTION_MT_RELEASE}>"
	    "$<$<CONFIG:RelWithDebInfo>:${COMPILE_OPTION_MT_RELEASE}>"
	    "$<$<CONFIG:Profile>:${COMPILE_OPTION_MT_RELEASE}>"
	)
endif()

add_library( JoltPhysics STATIC IMPORTED )

# Provide the full path to the library, so CMake knows where to find it.
# ${CMAKE_BUILD_TYPE}
set_target_properties( JoltPhysics PROPERTIES IMPORTED_LOCATION ${JOLTPHYSICS_DIR}/Build/build/Release/libjolt.a )

add_library(
	Physics SHARED
	${SRC_FILES}
	${PUBLIC_FILES}
)

target_link_libraries(
	Physics PRIVATE
	SDL2
)

add_dependencies( Physics "Core" )

set_target_properties(
	Physics PROPERTIES
	OUTPUT_NAME ch_physics
	PREFIX ""
)

target_precompile_headers( Physics PRIVATE "${CH_PUBLIC}/core/core.h" )
# target_link_libraries( Physics PRIVATE JoltPhysics )
include_directories( ${JOLTPHYSICS_DIR} ${JOLTPHYSICS_DIR}/Jolt )

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
