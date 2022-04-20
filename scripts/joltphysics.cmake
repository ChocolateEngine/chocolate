
set( JOLTPHYSICS_DIR ${CH_THIRDPARTY}/JoltPhysics )

message( "JoltPhysics = " ${JOLTPHYSICS_DIR} )

add_compile_definitions(
	"JOLT_PHYSICS=1"
)

include_directories( ${JOLTPHYSICS_DIR}/Jolt )
link_directories( ${JOLTPHYSICS_DIR}/Build/build/${CMAKE_BUILD_TYPE} )
link_libraries( Jolt )

