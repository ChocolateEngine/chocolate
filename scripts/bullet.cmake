
set( BULLET_DIR ${CH_THIRDPARTY}/bullet3 )

message( "Bullet = " ${BULLET_DIR} )

add_compile_definitions(
	"BULLET_PHYSICS=1"
)

include_directories( ${BULLET_DIR}/src )

if( MSVC )
	if( (CMAKE_BUILD_TYPE STREQUAL Debug) )
		link_directories( ${BULLET_DIR}/build/lib/Debug )
		link_libraries( LinearMath_Debug BulletCollision_Debug BulletDynamics_Debug )
	else()
		link_directories( ${BULLET_DIR}/build/lib/Release )
		link_libraries( LinearMath BulletCollision BulletDynamics )
	endif()
else()
	link_directories( /usr/local/include/bullet )
	link_libraries( LinearMath BulletCollision  BulletDynamics )
endif()

