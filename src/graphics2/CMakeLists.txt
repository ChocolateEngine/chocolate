message( "Current Project: Graphics69" )

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
	${CH_THIRDPARTY}/speedykeyv/KeyValue.cpp
	${CH_THIRDPARTY}/speedykeyv/KeyValue.h
)

include( ${CH_ROOT}/scripts/vulkan.cmake )

link_directories(
    ${CH_THIRDPARTY}/libspirv/lib
)

link_libraries(
	Core
	ImGui
	SPIRV-Tools
)


add_library( Graphics2 SHARED ${SRC_FILES} ${PUBLIC_FILES} )

add_dependencies( Graphics2 "Core" "ImGui" )

set_target_properties(
	Graphics2 PROPERTIES
	OUTPUT_NAME graphics2
)

if ( VULKAN_DEBUG )
	target_compile_definitions( Graphics2 PRIVATE
		-DVK_DEBUG=1
	)
else()
	target_compile_definitions( Graphics2 PRIVATE
		-DVK_DEBUG=0
	)
endif()

