message( "Current Project: Core" )

file(
	GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
	*.cpp
	*.h
)

file(
	GLOB_RECURSE PUBLIC_FILES CONFIGURE_DEPENDS
	${CH_PUBLIC}/core/*.h
	${CH_PUBLIC}/core/*.hpp
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
	OUTPUT_NAME core
)

target_precompile_headers( Core PUBLIC "${CH_PUBLIC}/core/core.h" )

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

