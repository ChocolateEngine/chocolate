message( "Current Project: ImGui" )

file(
	GLOB IMGUI_FILES CONFIGURE_DEPENDS
	${CH_PUBLIC}/imgui/*.h
	*.cpp
)

include_directories( ${CH_PUBLIC}/imgui )

if( MSVC )
	include_directories( ${CH_PUBLIC}/SDL2 )
endif()

include( ${CH_ROOT}/scripts/vulkan.cmake )

add_library(
	ImGui SHARED
	${IMGUI_FILES} )

set_target_properties(
	ImGui PROPERTIES
	LINKER_LANGUAGE CXX
	OUTPUT_NAME imgui
)

