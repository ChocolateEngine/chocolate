message( "Current Project: ch_render_3" )

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

# include( ${CH_ROOT}/scripts/rmlui.cmake )
include( ${CH_ROOT}/scripts/vulkan.cmake )

include_directories( ${CMAKE_CURRENT_LIST_DIR} )

add_library( ch_render_3 SHARED ${SRC_FILES} ${PUBLIC_FILES} )

target_link_libraries(
	ch_render_3
	PRIVATE
	Core
	ImGui
	SDL2
)

target_include_directories(
	ch_render_3 PRIVATE
	${CH_PUBLIC}/imgui
)

add_vk_to_target( ch_render_3 )

add_dependencies( ch_render_3 "Core" "ImGui" )

set_target_properties(
	ch_render_3 PROPERTIES
	RUNTIME_OUTPUT_NAME ch_render_3
	LIBRARY_OUTPUT_NAME ch_render_3
)

# render.h can't be a precompiled header, as in one file we need to enable some win32 define for vulkan,
# and that includes windows.h, and i don't want that polluting the entire project
# target_precompile_headers( ch_render_3 PRIVATE "render.h" "${CH_PUBLIC}/core/core.h" )
target_precompile_headers( ch_render_3 PRIVATE "${CH_PUBLIC}/core/core.h" )

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

