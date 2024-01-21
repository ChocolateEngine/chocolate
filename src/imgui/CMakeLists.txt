message( "Current Project: ImGui" )

file(
	GLOB_RECURSE IMGUI_FILES CONFIGURE_DEPENDS
	*.cpp
)

file(
	GLOB PUBLIC_FILES CONFIGURE_DEPENDS
	${CH_PUBLIC}/imgui/*.h
)

include_directories( ${CH_PUBLIC}/imgui )

if( MSVC )
	include_directories( ${CH_PUBLIC}/SDL2 )
endif()

add_library(
	ImGui SHARED
	${IMGUI_FILES}
	${PUBLIC_FILES} )

set_target_properties(
	ImGui PROPERTIES
	LINKER_LANGUAGE CXX
	OUTPUT_NAME imgui
)

target_link_libraries(
	ImGui PRIVATE
	SDL2
)

source_group(
	TREE ${CMAKE_CURRENT_LIST_DIR}
	PREFIX "Source Files"
	FILES ${IMGUI_FILES}
)

source_group(
	TREE ${CH_PUBLIC}
	PREFIX "Public"
	FILES ${PUBLIC_FILES}
)
