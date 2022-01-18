message( "Current Project: Graphics" )

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
	${CH_THIRDPARTY}/io/*.h
	${CH_THIRDPARTY}/speedykeyv/KeyValue.cpp
	${CH_THIRDPARTY}/speedykeyv/KeyValue.h
)

include( ${CH_ROOT}/scripts/vulkan.cmake )

link_libraries(
	Core
	ImGui
)


add_library( Graphics SHARED ${SRC_FILES} ${PUBLIC_FILES} )

add_dependencies( Graphics "Core" "ImGui" )

set_target_properties(
	Graphics PROPERTIES
	OUTPUT_NAME graphics
)

if ( VULKAN_DEBUG )
	target_compile_definitions( Graphics PRIVATE
		-DVK_DEBUG=1
	)
else()
	target_compile_definitions( Graphics PRIVATE
		-DVK_DEBUG=0
	)
endif()

