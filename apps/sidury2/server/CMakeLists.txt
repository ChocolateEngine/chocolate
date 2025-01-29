message( "Current Project: Sidury2 Server" )

include ( ../shared/sidury_shared.cmake )

set(
	SRC_FILES
	
	sv_interface.cpp
	sv_interface.h
	sv_main.cpp
	sv_main.h

	${SIDURY_SHARED_SRC_FILES}
)

add_compile_definitions(
	"CH_CLIENT=0"
	"CH_SERVER=1"
)

# ??????
add_library( Sidury2Server SHARED ../../graphics/mesh_builder.cpp ${SRC_FILES} ${PUBLIC_FILES} ${THIRDPARTY_FILES} )

include_directories(
	"${CMAKE_CURRENT_LIST_DIR}"
)

target_link_libraries(
	Sidury2Server
	PRIVATE
	Core
	ImGui
	SDL2
)

if( WIN32 )
	target_link_libraries(
		Sidury2Server
		PRIVATE
		# Networking
		wsock32
		ws2_32
		Iphlpapi
	)
endif()

add_dependencies( Sidury2Server "Core" "ImGui" )

set_target_properties(
	Sidury2Server PROPERTIES
	RUNTIME_OUTPUT_NAME ch_server
	LIBRARY_OUTPUT_NAME ch_server

	RUNTIME_OUTPUT_DIRECTORY ${CH_OUTPUT}/sidury/bin/${PLAT_FOLDER}
	LIBRARY_OUTPUT_DIRECTORY ${CH_OUTPUT}/sidury/bin/${PLAT_FOLDER}
)

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set_target_properties(
    	Sidury2Server PROPERTIES
    	RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_OUTPUT}/sidury/bin/${PLAT_FOLDER}
    	LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_OUTPUT}/sidury/bin/${PLAT_FOLDER}
    )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

target_precompile_headers( Sidury2Server PRIVATE "${CH_PUBLIC}/core/core.h" )

source_group(
	TREE ${CMAKE_CURRENT_LIST_DIR}/../
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

