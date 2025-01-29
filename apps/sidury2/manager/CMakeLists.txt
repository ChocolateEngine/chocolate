message( "Current Project: Sidury2 Manager" )

set(
	SRC_FILES
	main.cpp
)

include_directories(
	"../../"
	"../../graphics"
)

add_library( Sidury2Manager SHARED ${SRC_FILES} )

target_link_libraries(
	Sidury2Manager
	PRIVATE
	Core
	ImGui
	SDL2
)

add_dependencies( Sidury2Manager "Core" "ImGui" )

set_target_properties(
	Sidury2Manager PROPERTIES
	RUNTIME_OUTPUT_NAME ch_manager
	LIBRARY_OUTPUT_NAME ch_manager

	RUNTIME_OUTPUT_DIRECTORY ${CH_OUTPUT}/sidury/bin/${PLAT_FOLDER}
	LIBRARY_OUTPUT_DIRECTORY ${CH_OUTPUT}/sidury/bin/${PLAT_FOLDER}
)

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set_target_properties(
    	Sidury2Manager PROPERTIES
    	RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_OUTPUT}/sidury/bin/${PLAT_FOLDER}
    	LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_OUTPUT}/sidury/bin/${PLAT_FOLDER}
    )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

target_precompile_headers( Sidury2Manager PRIVATE "${CH_PUBLIC}/core/core.h" )

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

