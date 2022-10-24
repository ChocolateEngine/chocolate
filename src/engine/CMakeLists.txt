message( "Current Project: Engine" )

file(
	GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
	*.cpp
	*.h
)

file(
	#GLOB_RECURSE PUBLIC_FILES CONFIGURE_DEPENDS
	GLOB PUBLIC_FILES CONFIGURE_DEPENDS
	${CH_PUBLIC}/*.cpp
	${CH_PUBLIC}/*.h
	${CH_PUBLIC}/core/*.h
)

link_libraries( Core ImGui )

add_library( Engine SHARED ${SRC_FILES} ${PUBLIC_FILES} )

add_dependencies( Engine "Core" "ImGui" )

set_target_properties(
	Engine PROPERTIES
	OUTPUT_NAME engine
)

target_precompile_headers( Engine PRIVATE "${CH_PUBLIC}/core/core.h" )

