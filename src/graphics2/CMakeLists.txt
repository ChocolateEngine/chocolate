message( "Current Project: RenderVK" )

set(
	RENDER_VK_FILES
	
	render_vk.cpp
	render_vk.h
	instance.cpp
	descriptor_set.cpp
	present.cpp
	render_pass.cpp
	shaders.cpp
	swapchain.cpp
	texture.cpp
	texture_ktx.cpp
	conversions.cpp
	
	# imgui files
	imgui_impl_vulkan.cpp
	imgui_impl_vulkan.h
)

file(
	#GLOB_RECURSE PUBLIC_FILES CONFIGURE_DEPENDS
	GLOB PUBLIC_FILES CONFIGURE_DEPENDS
	${CH_PUBLIC}/*.cpp
	${CH_PUBLIC}/*.h
	${CH_PUBLIC}/core/*.h
	${CH_PUBLIC}/graphics/*.h
	${CH_PUBLIC}/graphics2/*.h
)

file(
	GLOB THIRDPARTY_FILES CONFIGURE_DEPENDS
	${CH_THIRDPARTY}/io/*.h
	${CH_THIRDPARTY}/speedykeyv/KeyValue.cpp
	${CH_THIRDPARTY}/speedykeyv/KeyValue.h
)

include( ${CH_ROOT}/scripts/vulkan.cmake )

add_library( RenderVK SHARED ${RENDER_VK_FILES} ${PUBLIC_FILES} ${THIRDPARTY_FILES} )

add_dependencies( RenderVK "Core" "ImGui" )

target_precompile_headers( RenderVK PRIVATE "${CH_PUBLIC}/core/core.h" )

set_target_properties(
	RenderVK PROPERTIES
	OUTPUT_NAME ch_render_vk
)

target_include_directories(
	RenderVK PRIVATE
	${CH_PUBLIC}/imgui
)

target_link_libraries(
	RenderVK PRIVATE
	Core
	ImGui
	SDL2
)

add_vk_to_target( RenderVK )

if ( VULKAN_DEBUG )
	target_compile_definitions( RenderVK PRIVATE
		-DVK_DEBUG=1
	)
else()
	target_compile_definitions( RenderVK PRIVATE
		-DVK_DEBUG=0
	)
endif()

source_group(
	TREE ${CMAKE_CURRENT_LIST_DIR}
	PREFIX "Source Files"
	FILES ${RENDER_VK_FILES}
)

source_group(
	TREE ${CH_PUBLIC}
	PREFIX "Public"
	FILES ${PUBLIC_FILES}
)

source_group(
	TREE ${CH_THIRDPARTY}
	PREFIX "Thirdparty"
	FILES ${THIRDPARTY_FILES}
)
