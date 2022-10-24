message( "Current Project: GUI" )

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
	${CH_PUBLIC}/graphics/*.h
	${CH_PUBLIC}/graphics2/*.h
	${CH_THIRDPARTY}/io/*.h
)

link_libraries(
	Core
	ImGui
)

add_library( GUI SHARED ${SRC_FILES} ${PUBLIC_FILES} )

add_dependencies( Graphics2 "Core" "ImGui" )

set_target_properties(
	GUI PROPERTIES
	OUTPUT_NAME ch_gui
)
