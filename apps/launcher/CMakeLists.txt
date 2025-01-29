message( "Current Project: Sidury Launcher" )

set( BASE_SRC_FILES
	launcher_base.cpp
	launcher_base.h
)

add_executable( LauncherSidury launcher_sidury.cpp ${BASE_SRC_FILES} )

set( SIDURY_LAUNCHER_NAME sidury )

set_target_properties(
	LauncherSidury PROPERTIES
	OUTPUT_NAME ${SIDURY_LAUNCHER_NAME}_${PLAT_FOLDER}
	RUNTIME_OUTPUT_DIRECTORY ${CH_OUTPUT}
	
	VS_DEBUGGER_WORKING_DIRECTORY ${CH_OUTPUT}
)

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set_target_properties(
    	LauncherSidury PROPERTIES
    	RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_OUTPUT}
    )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

# Set as startup project in visual studio
# set_property( DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY VS_STARTUP_PROJECT LauncherSidury )

if (MSVC)
	# Copy a pre-built manifest file to the output directory for DPI Scaling
	add_custom_command(
		TARGET LauncherSidury POST_BUILD
        # COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/launcher.exe.manifest" "${CH_BIN}/${SIDURY_LAUNCHER_NAME}_${PLAT_FOLDER}.exe.manifest"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/launcher.exe.manifest" "${CH_OUTPUT}/${SIDURY_LAUNCHER_NAME}_${PLAT_FOLDER}.exe.manifest"
		COMMAND_EXPAND_LISTS
        COMMENT "Copying ${SIDURY_LAUNCHER_NAME}.exe.manifest to launcher directory"
    )
endif(MSVC)


# ======================================================================================================


message( "Current Project: Toolkit Launcher" )

add_executable( LauncherToolkit launcher_toolkit.cpp ${BASE_SRC_FILES} )

set( TOOLKIT_LAUNCHER_NAME toolkit )

set_target_properties(
	LauncherToolkit PROPERTIES
	OUTPUT_NAME ${TOOLKIT_LAUNCHER_NAME}_${PLAT_FOLDER}
	RUNTIME_OUTPUT_DIRECTORY ${CH_OUTPUT}
	
	VS_DEBUGGER_WORKING_DIRECTORY ${CH_OUTPUT}
)

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set_target_properties(
    	LauncherToolkit PROPERTIES
    	RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_OUTPUT}
    )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

# Set as startup project in visual studio
# set_property( DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY VS_STARTUP_PROJECT LauncherToolkit )

if (MSVC)
	# Copy a pre-built manifest file to the output directory for DPI Scaling
	add_custom_command(
		TARGET LauncherToolkit POST_BUILD
        # COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/launcher.exe.manifest" "${CH_BIN}/${TOOLKIT_LAUNCHER_NAME}_${PLAT_FOLDER}.exe.manifest"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/launcher.exe.manifest" "${CH_OUTPUT}/${TOOLKIT_LAUNCHER_NAME}_${PLAT_FOLDER}.exe.manifest"
		COMMAND_EXPAND_LISTS
        COMMENT "Copying ${TOOLKIT_LAUNCHER_NAME}.exe.manifest to launcher directory"
    )
endif(MSVC)


# ======================================================================================================


message( "Current Project: Render Test Launcher" )

add_executable( ch_render3_test_launch launcher_render_test.cpp ${BASE_SRC_FILES} )

set( RENDER_TEST_LAUNCHER_NAME render3_test_launch )

set_target_properties(
	ch_render3_test_launch PROPERTIES
	OUTPUT_NAME ${RENDER_TEST_LAUNCHER_NAME}_${PLAT_FOLDER}
	RUNTIME_OUTPUT_DIRECTORY ${CH_OUTPUT}
	
	VS_DEBUGGER_WORKING_DIRECTORY ${CH_OUTPUT}
)

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set_target_properties(
    	ch_render3_test_launch PROPERTIES
    	RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_OUTPUT}
    )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

# Set as startup project in visual studio
# set_property( DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY VS_STARTUP_PROJECT ch_render3_test_launch )

if (MSVC)
	# Copy a pre-built manifest file to the output directory for DPI Scaling
	add_custom_command(
		TARGET ch_render3_test_launch POST_BUILD
        # COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/launcher.exe.manifest" "${CH_BIN}/${RENDER_TEST_LAUNCHER_NAME}_${PLAT_FOLDER}.exe.manifest"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/launcher.exe.manifest" "${CH_OUTPUT}/${RENDER_TEST_LAUNCHER_NAME}_${PLAT_FOLDER}.exe.manifest"
		COMMAND_EXPAND_LISTS
        COMMENT "Copying ${RENDER_TEST_LAUNCHER_NAME}.exe.manifest to launcher directory"
    )
endif(MSVC)


