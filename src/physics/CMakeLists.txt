message( "Current Project: Physics" )

set( JOLTPHYSICS_DIR ${CH_THIRDPARTY}/JoltPhysics )
message( "JoltPhysics = " ${JOLTPHYSICS_DIR} )

file(
	GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
	*.cpp
	*.h
)

file(
	GLOB_RECURSE PUBLIC_FILES CONFIGURE_DEPENDS
	${CH_PUBLIC}/*.cpp
	${CH_PUBLIC}/*.h
)

link_libraries( Core )

# this is really stupid
if ( MSVC )
	set( COMPILE_OPTION_MT_DEBUG "/MTd" )
	set( COMPILE_OPTION_MT_RELEASE "/MT" )

	# NOTE: won't work for everything, really dumb
	add_compile_options(
	    "$<$<CONFIG:Debug>:${COMPILE_OPTION_MT_DEBUG}>"
	    "$<$<CONFIG:Release>:${COMPILE_OPTION_MT_RELEASE}>"
	    "$<$<CONFIG:RelWithDebInfo>:${COMPILE_OPTION_MT_RELEASE}>"
	    "$<$<CONFIG:Profile>:${COMPILE_OPTION_MT_RELEASE}>"
	)
endif()

include_directories(
	"${CH_THIRDPARTY}/"
)

# MIN is jolt physics compiled with the least cpu features we want
# add_library( JoltPhysics_MIN STATIC IMPORTED )

# AVX2 is compiled with all cpu features
# add_library( JoltPhysics_AVX2 STATIC IMPORTED )

# Provide the full path to the library, so CMake knows where to find it.
# ${CMAKE_BUILD_TYPE}
# set_target_properties( JoltPhysics_MIN PROPERTIES IMPORTED_LOCATION ${JOLTPHYSICS_DIR}/Build/min/Release/libjolt.a )
# set_target_properties( JoltPhysics_AVX2 PROPERTIES IMPORTED_LOCATION ${JOLTPHYSICS_DIR}/Build/avx2/Release/libjolt.a )

add_library(
	Physics_MIN SHARED
	${SRC_FILES}
	${PUBLIC_FILES}
)

add_library(
	Physics_AVX2 SHARED
	${SRC_FILES}
	${PUBLIC_FILES}
)

set( JOLT_MIN_PATH_DEBUG "${JOLTPHYSICS_DIR}/Build/min/Debug" )
set( JOLT_MIN_PATH_RELEASE "${JOLTPHYSICS_DIR}/Build/min/Release" )

set( JOLT_AVX2_PATH_DEBUG "${JOLTPHYSICS_DIR}/Build/avx2/Debug" )
set( JOLT_AVX2_PATH_RELEASE "${JOLTPHYSICS_DIR}/Build/avx2/Release" )

target_link_libraries(
	Physics_MIN PRIVATE
	SDL2
	Jolt
)

target_link_libraries(
	Physics_AVX2 PRIVATE
	SDL2
	Jolt
)

target_link_directories(
	Physics_MIN PRIVATE
	"$<$<CONFIG:Debug>:${JOLT_MIN_PATH_DEBUG}>"
	"$<$<CONFIG:Release>:${JOLT_MIN_PATH_RELEASE}>"
	"$<$<CONFIG:RelWithDebInfo>:${JOLT_MIN_PATH_RELEASE}>"
	"$<$<CONFIG:Profile>:${JOLT_MIN_PATH_RELEASE}>"
)

target_link_directories(
	Physics_AVX2 PRIVATE
	"$<$<CONFIG:Debug>:${JOLT_AVX2_PATH_DEBUG}>"
	"$<$<CONFIG:Release>:${JOLT_AVX2_PATH_RELEASE}>"
	"$<$<CONFIG:RelWithDebInfo>:${JOLT_AVX2_PATH_RELEASE}>"
	"$<$<CONFIG:Profile>:${JOLT_AVX2_PATH_RELEASE}>"
)



add_dependencies( Physics_MIN "Core" )
add_dependencies( Physics_AVX2 "Core" )

set_target_properties(
	Physics_MIN PROPERTIES
	OUTPUT_NAME ch_physics_min
	PREFIX ""
)

set_target_properties(
	Physics_AVX2 PROPERTIES
	OUTPUT_NAME ch_physics_avx2
	PREFIX ""
)

target_precompile_headers( Physics_MIN PRIVATE "${CH_PUBLIC}/core/core.h" )
target_precompile_headers( Physics_AVX2 PRIVATE "${CH_PUBLIC}/core/core.h" )

# target_link_libraries( Physics PRIVATE JoltPhysics )
include_directories( ${JOLTPHYSICS_DIR} ${JOLTPHYSICS_DIR}/Jolt )

source_group(
	TREE ${CMAKE_CURRENT_LIST_DIR}
	PREFIX "Source Files"
	FILES ${SRC_FILES}
)

source_group(
	TREE ${CH_PUBLIC}
	PREFIX "Public"
	FILES ${PUBLIC_FILES}
)
