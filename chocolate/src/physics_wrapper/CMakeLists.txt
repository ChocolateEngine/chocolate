message( "Current Project: Physics Wrapper" )

set(
	SRC_FILES
	physics_wrapper.cpp
)

file(
	GLOB_RECURSE PUBLIC_FILES CONFIGURE_DEPENDS
	${CH_PUBLIC}/*.cpp
	${CH_PUBLIC}/*.h
)

link_libraries( Core )

add_library(
	Physics_Wrapper SHARED
	${SRC_FILES}
	${PUBLIC_FILES}
)

target_link_libraries(
	Physics_Wrapper PRIVATE
	SDL2
)

add_dependencies( Physics_Wrapper "Core" )

set_target_properties(
	Physics_Wrapper PROPERTIES
	OUTPUT_NAME ch_physics
	PREFIX ""
)

target_precompile_headers( Physics_Wrapper PRIVATE "${CH_PUBLIC}/core/core.h" )
# target_link_libraries( Physics_Wrapper PRIVATE JoltPhysics )
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
