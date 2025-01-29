message( "Current Project: ch_graphics_data" )

file(
	GLOB_RECURSE SRC_FILES_GLOB CONFIGURE_DEPENDS
	*.cpp
	*.h
)

set(
	SRC_FILES
	${SRC_FILES_GLOB}
)

file(
	GLOB_RECURSE PUBLIC_FILES CONFIGURE_DEPENDS
	${CH_PUBLIC}/*.cpp
	${CH_PUBLIC}/*.h
	${CH_PUBLIC}/core/*.h
)

include_directories( ${CMAKE_CURRENT_LIST_DIR} )

add_library( ch_graphics_data SHARED ${SRC_FILES} ${PUBLIC_FILES} )

target_link_libraries(
	ch_graphics_data
	PRIVATE
	Core
	SDL2
)

target_compile_definitions(
	ch_graphics_data PRIVATE
	USING_RENDER3=1
)

add_dependencies( ch_graphics_data "Core" "ImGui" )

set_target_properties(
	ch_graphics_data PROPERTIES
	RUNTIME_OUTPUT_NAME ch_graphics_data
	LIBRARY_OUTPUT_NAME ch_graphics_data
)

target_precompile_headers( ch_graphics_data PRIVATE "${CH_PUBLIC}/core/core.h" )

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

