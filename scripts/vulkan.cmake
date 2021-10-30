
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

