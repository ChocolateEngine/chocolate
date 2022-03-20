
# yeah, i could use FindVulkan, but i don't like cmake's automated things so...

set( VK_DIR $ENV{VULKAN_SDK} )

message( "VK_DIR = " ${VK_DIR} )

if( MSVC )
	include_directories( ${VK_DIR}/Include )
	link_directories( ${VK_DIR}/Lib )
	link_libraries( vulkan-1 )
	
else()
	# are these first 2 win32 only?
	include_directories( ${VK_DIR}/Include )
	link_directories( ${VK_DIR}/Lib )
	link_libraries( vulkan )
	
endif()

# renderdoc support
if( DEFINED RENDER_DOC )
	message( "RENDER_DOC = " ${RENDER_DOC} )
	include_directories( "${RENDER_DOC}" )
	add_compile_definitions( RENDER_DOC=1 )
endif()


include( ${CH_ROOT}/scripts/ktx.cmake )

