message( "Current Project: Physics" )

include( ${CH_ROOT}/scripts/joltphysics.cmake )

file(
	GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
	*.cpp
	*.h
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
	)
endif()

add_library(
	Physics SHARED
	${SRC_FILES}
)

add_dependencies( Physics "Core" )

set_target_properties(
	Physics PROPERTIES
	OUTPUT_NAME ch_physics
	PREFIX ""
)

target_precompile_headers( Physics PRIVATE "${CH_PUBLIC}/core/core.h" )

