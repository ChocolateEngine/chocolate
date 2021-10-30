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
	${CH_PUBLIC}/util.cpp
)

add_compile_definitions( "CORE_DLL" )

add_library( Core SHARED ${SRC_FILES} ${PUBLIC_FILES} )

set_target_properties(
	Core PROPERTIES
	LINKER_LANGUAGE CXX
	OUTPUT_NAME core
)

