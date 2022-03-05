message( "Current Project: Core" )

file(
	GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
	*.cpp
	*.h
)

file(
	GLOB_RECURSE PUBLIC_FILES CONFIGURE_DEPENDS
	${CH_PUBLIC}/core/*.h
)

list( APPEND PUBLIC_FILES
	${CH_PUBLIC}/util.h
)

add_compile_definitions( "CORE_DLL" )

add_library( Core SHARED ${SRC_FILES} ${PUBLIC_FILES} )

if( MSVC )
	target_link_libraries( Core Shlwapi )
endif()

set_target_properties(
	Core PROPERTIES
	LINKER_LANGUAGE CXX
	OUTPUT_NAME core
)

target_precompile_headers( Core PUBLIC "${CH_PUBLIC}/core/core.h" )

