message( "Current Project: Sidury Client" )

include ( ../shared/sidury_shared.cmake )

set(
	SRC_FILES
	
	cl_interface.cpp
	cl_interface.h
	cl_main.cpp
	cl_main.h
	inputsystem.cpp
	inputsystem.h

	${SIDURY_SHARED_SRC_FILES}
)

add_compile_definitions(
	"CH_CLIENT=1"
	"CH_SERVER=0"
)

include_directories(
	"${CMAKE_CURRENT_LIST_DIR}"
)

# ??????
add_library( SiduryClient SHARED ${SRC_FILES} ${PUBLIC_FILES} ${THIRDPARTY_FILES} ../shared/sidury_shared.cmake )

target_link_libraries(
	SiduryClient
	PRIVATE
	Core
	ImGui
	SDL2
	flatbuffers
)

if( WIN32 )
	target_link_libraries(
		SiduryClient
		PRIVATE
		# Networking
		wsock32
		ws2_32
		Iphlpapi
	)
endif()

add_dependencies( SiduryClient "Core" "ImGui" )

set_target_properties(
	SiduryClient PROPERTIES
	RUNTIME_OUTPUT_NAME ch_client
	LIBRARY_OUTPUT_NAME ch_client

	RUNTIME_OUTPUT_DIRECTORY ${CH_BUILD}/sidury/bin/${PLAT_FOLDER}
	LIBRARY_OUTPUT_DIRECTORY ${CH_BUILD}/sidury/bin/${PLAT_FOLDER}
)

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set_target_properties(
    	SiduryClient PROPERTIES
    	RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_BUILD}/sidury/bin/${PLAT_FOLDER}
    	LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_BUILD}/sidury/bin/${PLAT_FOLDER}
    )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

target_precompile_headers( SiduryClient PRIVATE "${CH_PUBLIC}/core/core.h" )

source_group(
	TREE ${CMAKE_CURRENT_LIST_DIR}/../../
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

