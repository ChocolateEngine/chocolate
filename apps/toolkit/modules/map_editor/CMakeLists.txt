message( "Current Project: Tool - Map Editor" )

file(
	GLOB_RECURSE SRC_FILES_GLOB CONFIGURE_DEPENDS
	*.cpp
	*.h
)

file(
	GLOB_RECURSE SHARED_FILES_GLOB CONFIGURE_DEPENDS
	../../shared/*.cpp
	../../shared/*.h

	# Sidury Shared files
	../../../shared/*.cpp
	../../../shared/*.h
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

include_directories(
	"../../shared"
	"../../../shared"
)

add_library( Tool_MapEditor SHARED ${SRC_FILES} ${SHARED_FILES_GLOB} ${PUBLIC_FILES} )

target_link_libraries(
	Tool_MapEditor
	PRIVATE
	Core
	ImGui
	SDL2
)

add_dependencies( Tool_MapEditor "Core" "ImGui" )

set_target_properties(
	Tool_MapEditor PROPERTIES
	RUNTIME_OUTPUT_NAME ch_map_editor
	LIBRARY_OUTPUT_NAME ch_map_editor

	RUNTIME_OUTPUT_DIRECTORY ${CH_BUILD}/toolkit/bin/${PLAT_FOLDER}/modules
	LIBRARY_OUTPUT_DIRECTORY ${CH_BUILD}/toolkit/bin/${PLAT_FOLDER}/modules
)

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set_target_properties(
    	Tool_MapEditor PROPERTIES
    	RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_BUILD}/toolkit/bin/${PLAT_FOLDER}/modules
    	LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_BUILD}/toolkit/bin/${PLAT_FOLDER}/modules
    )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

target_precompile_headers( Tool_MapEditor PRIVATE "${CH_PUBLIC}/core/core.h" )

source_group(
	TREE ${CMAKE_CURRENT_LIST_DIR}/../
	PREFIX "Source Files"
	FILES ${SRC_FILES}
)

source_group(
	TREE ${CMAKE_CURRENT_LIST_DIR}/../../../
	PREFIX "Shared"
	FILES ${SHARED_FILES_GLOB}
)

source_group(
	TREE ${CH_PUBLIC}
	PREFIX "Public"
	FILES ${PUBLIC_FILES}
)
