message( "Current Project: Renderer" )

file(
	GLOB_RECURSE SRC_FILES_GLOB CONFIGURE_DEPENDS
	*.cpp
	*.h
)

set(
	SRC_FILES
	${SRC_FILES_GLOB}
)

file(
	GLOB_RECURSE PUBLIC_FILES CONFIGURE_DEPENDS
	${CH_PUBLIC}/*.cpp
	${CH_PUBLIC}/*.h
	${CH_PUBLIC}/core/*.h
)

set(
	THIRDPARTY_FILES
	${CH_THIRDPARTY}/speedykeyv/KeyValue.cpp
	${CH_THIRDPARTY}/speedykeyv/KeyValue.h
)

# include( ${CH_ROOT}/scripts/rmlui.cmake )

include_directories( ${CMAKE_CURRENT_LIST_DIR} )

add_library( Graphics SHARED ${SRC_FILES} ${PUBLIC_FILES} ${THIRDPARTY_FILES} )

target_link_libraries(
	Graphics
	PRIVATE
	Core
	ImGui
	SDL2
)

if( WIN32 )
	target_link_libraries(
		Graphics
		PRIVATE
		# Networking
		wsock32
		ws2_32
		Iphlpapi
	)
endif()

add_dependencies( Graphics "Core" "ImGui" )

set_target_properties(
	Graphics PROPERTIES
	RUNTIME_OUTPUT_NAME ch_graphics
	LIBRARY_OUTPUT_NAME ch_graphics

	RUNTIME_OUTPUT_DIRECTORY ${CH_BUILD}/sidury/bin/${PLAT_FOLDER}
	LIBRARY_OUTPUT_DIRECTORY ${CH_BUILD}/sidury/bin/${PLAT_FOLDER}
)

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set_target_properties(
    	Graphics PROPERTIES
    	RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_BUILD}/sidury/bin/${PLAT_FOLDER}
    	LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_BUILD}/sidury/bin/${PLAT_FOLDER}
    )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

target_precompile_headers( Graphics PRIVATE "${CH_PUBLIC}/core/core.h" )

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

source_group(
	TREE ${CH_THIRDPARTY}
	PREFIX "Thirdparty"
	FILES ${THIRDPARTY_FILES}
)

