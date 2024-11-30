message( "Current Project: Render Test" )

set(
	SRC_FILES
	main.cpp
)

set(
	PUBLIC_FILES
	${CH_PUBLIC}/types/transform.cpp
)

include_directories(
	"../../"
	"../../graphics"
)

add_library( ch_render_3_test SHARED ${SRC_FILES} ${PUBLIC_FILES} )

target_link_libraries(
	ch_render_3_test
	PRIVATE
	Core
	ImGui
	SDL2
)

target_compile_definitions(
	ch_render_3_test PRIVATE
	USING_RENDER3=1
)

add_dependencies( ch_render_3_test "Core" "ImGui" )

set_target_properties(
	ch_render_3_test PROPERTIES
	RUNTIME_OUTPUT_NAME ch_render_3_test
	LIBRARY_OUTPUT_NAME ch_render_3_test

	RUNTIME_OUTPUT_DIRECTORY ${CH_BUILD}/render_test/bin/${PLAT_FOLDER}
	LIBRARY_OUTPUT_DIRECTORY ${CH_BUILD}/render_test/bin/${PLAT_FOLDER}
)

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set_target_properties(
    	ch_render_3_test PROPERTIES
    	RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_BUILD}/render_test/bin/${PLAT_FOLDER}
    	LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_BUILD}/render_test/bin/${PLAT_FOLDER}
    )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

target_precompile_headers( ch_render_3_test PRIVATE "${CH_PUBLIC}/core/core.h" )

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

