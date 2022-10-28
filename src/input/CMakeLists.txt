message( "Current Project: Input" )

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

add_library( Input SHARED ${SRC_FILES} ${PUBLIC_FILES} )

add_dependencies( Input "Core" "ImGui" )

set_target_properties(
	Input PROPERTIES
	OUTPUT_NAME input
)
