
# yeah, i could use FindVulkan, but i don't like cmake's automated things so...

set( VK_DIR $ENV{VULKAN_SDK} )
set( VMA_DIR "${CH_THIRDPARTY}/VulkanMemoryAllocator" )

message( "VK_DIR = " ${VK_DIR} )
message( "VMA_DIR = " ${VMA_DIR} )

function( add_vk_to_target TARGET )
	target_include_directories( ${TARGET} PRIVATE ${VMA_DIR}/include )
	
	if( MSVC )
		target_include_directories( ${TARGET} PRIVATE ${VK_DIR}/Include )
		target_link_directories( ${TARGET} PRIVATE ${VK_DIR}/Lib )
		target_link_libraries( ${TARGET} PRIVATE "vulkan-1" )
		
		target_link_libraries(
			${TARGET} PRIVATE
			"$<$<CONFIG:Debug>:${VMA_DIR}/build/src/Debug/VulkanMemoryAllocatord.lib>"
			"$<$<CONFIG:Release>:${VMA_DIR}/build/src/Release/VulkanMemoryAllocator.lib>"
			"$<$<CONFIG:RelWithDebInfo>:${VMA_DIR}/build/src/Release/VulkanMemoryAllocator.lib>"
		)
		
	else()
		# are these first 2 win32 only?
		target_include_directories( ${TARGET} PRIVATE ${VK_DIR}/Include )
		target_link_directories( ${TARGET} PRIVATE ${VK_DIR}/Lib )
		target_link_libraries( ${TARGET} PRIVATE "vulkan" )
		
	endif()

	# renderdoc support
	if( DEFINED RENDER_DOC )
		message( "RENDER_DOC = " ${RENDER_DOC} )
		include_directories( "${RENDER_DOC}" )
		add_compile_definitions( RENDER_DOC=1 )
	endif()
	
endfunction()

include( ${CH_ROOT}/scripts/ktx.cmake )
