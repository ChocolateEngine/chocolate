message( "Current Project: Renderer" )

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

include_directories( ${CMAKE_CURRENT_LIST_DIR} )

add_library( Render SHARED ${SRC_FILES} ${PUBLIC_FILES} )

target_link_libraries(
	Render
	PRIVATE
	Core
	ImGui
	SDL2
)

if( WIN32 )
	target_link_libraries(
		Render
		PRIVATE
		# Networking
		wsock32
		ws2_32
		Iphlpapi
	)
endif()

add_dependencies( Render "Core" "ImGui" )

set_target_properties(
	Render PROPERTIES
	RUNTIME_OUTPUT_NAME ch_render
	LIBRARY_OUTPUT_NAME ch_render
)

target_precompile_headers( Render PRIVATE "${CH_PUBLIC}/core/core.h" )

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

