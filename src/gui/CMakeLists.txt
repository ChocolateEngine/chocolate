message( "Current Project: GUI" )

file(
	#GLOB_RECURSE PUBLIC_FILES CONFIGURE_DEPENDS
	GLOB PUBLIC_FILES CONFIGURE_DEPENDS
	${CH_PUBLIC}/*.cpp
	${CH_PUBLIC}/*.h
	${CH_PUBLIC}/core/*.h
	${CH_PUBLIC}/graphics/*.h
	${CH_PUBLIC}/graphics2/*.h
)

link_libraries(
	Core
	ImGui
)

# include( ${CH_ROOT}/scripts/rmlui.cmake )

set(
	SRC_FILES
	
	consoleui.cpp
	consoleui.h
	convar_list.cpp
	gui.cpp
	gui.h
	# rmlui.cpp
	# rmlui.h
	
	# NOTE: maybe use Win32 backend for windows?
	# ${RMLUI_DIR}/Backends/RmlUi_Platform_SDL.cpp
	# ${RMLUI_DIR}/Backends/RmlUi_Platform_SDL.h
)

add_library( GUI SHARED ${SRC_FILES} ${PUBLIC_FILES} )

add_dependencies( GUI "Core" "ImGui" )

set_target_properties(
	GUI PROPERTIES
	OUTPUT_NAME ch_gui
)

source_group(
	TREE ${CMAKE_CURRENT_LIST_DIR}
	PREFIX "Source Files"
	FILES ${CH_ROOT}
)

source_group(
	TREE ${CH_PUBLIC}
	PREFIX "Public"
	FILES ${PUBLIC_FILES}
)
