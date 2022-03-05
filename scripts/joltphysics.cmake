
set( JOLTPHYSICS_DIR ${CH_THIRDPARTY}/JoltPhysics )

message( "JoltPhysics = " ${JOLTPHYSICS_DIR} )

add_compile_definitions(
	"JOLT_PHYSICS=1"
)

include_directories( ${JOLTPHYSICS_DIR}/Jolt )

if( MSVC )
	if( (CMAKE_BUILD_TYPE STREQUAL Debug) )
		link_directories( ${JOLTPHYSICS_DIR}/Build/Debug )
	else()
		link_directories( ${JOLTPHYSICS_DIR}/Build/Release )
	endif()
	link_libraries( Jolt )
else()
	message( "NO JOLTPHYSICS ON LINUX SETUP YET AAAAA" )
endif()

