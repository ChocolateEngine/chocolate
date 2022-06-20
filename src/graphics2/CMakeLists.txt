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

if (WIN32)
	link_directories(
		# ${VK_DIR}/lib
	)
else()
	link_directories(
		${CH_THIRDPARTY}/libspirv/lib
	)
endif()

link_libraries(
	Core
	ImGui
)

if (WIN32)
	link_libraries( ${CH_THIRDPARTY}/SPIRV-Tools/build/source/Debug/SPIRV-Tools.lib )
else()
	link_libraries( SPIRV-Tools )
endif()

add_library( Graphics2 SHARED ${SRC_FILES} ${PUBLIC_FILES} )

add_dependencies( Graphics2 "Core" "ImGui" )

set_target_properties(
	Graphics2 PROPERTIES
	OUTPUT_NAME ch_graphics
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

