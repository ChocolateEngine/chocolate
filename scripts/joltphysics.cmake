
set( JOLTPHYSICS_DIR ${CH_THIRDPARTY}/JoltPhysics )

message( "JoltPhysics = " ${JOLTPHYSICS_DIR} )

add_compile_definitions(
	"JOLT_PHYSICS=1"
)

set( JOLT_PATH_DEBUG "${JOLTPHYSICS_DIR}/Build/build/Debug" )
set( JOLT_PATH_RELEASE "${JOLTPHYSICS_DIR}/Build/build/Release" )

link_directories(
	"$<$<CONFIG:Debug>:${JOLT_PATH_DEBUG}>"
	"$<$<CONFIG:Release>:${JOLT_PATH_RELEASE}>"
	"$<$<CONFIG:RelWithDebInfo>:${JOLT_PATH_RELEASE}>"
)

include_directories( ${JOLTPHYSICS_DIR} ${JOLTPHYSICS_DIR}/Jolt )
link_libraries( Jolt )

