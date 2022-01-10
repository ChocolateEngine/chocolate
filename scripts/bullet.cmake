
# yeah, i could use FindVulkan, but i don't like cmake's automated things so...

set( BULLET_DIR ${CH_THIRDPARTY}/bullet3 )

message( "Bullet = " ${BULLET_DIR} )

include_directories( ${BULLET_DIR}/src )

if( MSVC )
	if( (CMAKE_BUILD_TYPE STREQUAL Debug) )
		link_directories( ${BULLET_DIR}/build/lib/Release )
		link_libraries( LinearMath BulletCollision BulletDynamics )
	else()
		link_directories( ${BULLET_DIR}/build/lib/Debug )
		link_libraries( LinearMath_Debug BulletCollision_Debug BulletDynamics_Debug )
	endif()
else()
		message( "NO BULLET ON LINUX SETUP YET AAAAA" )
endif()

