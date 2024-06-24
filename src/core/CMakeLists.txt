message( "Current Project: Core" )

add_custom_target(
	BuildNumberUpdater
    COMMAND ${CMAKE_COMMAND} -E touch_nocreate "${CMAKE_CURRENT_SOURCE_DIR}/build_number.cpp"
)

set(
	SRC_FILES
	"app_info.cpp"
	"asserts.cpp"
	"build_number.cpp"
	"commandline.cpp"
	"console.cpp"
	"convar.cpp"
	"filesystem.cpp"
	"json5.cpp"
	"log.cpp"
	"mempool.cpp"
	"platform_shared.cpp"
	"platform.cpp"
	"platform_linux.cpp"
	"profiler.cpp"
	"resource.cpp"
	"systemmanager.cpp"
	"string.cpp"
	"util.cpp"
)

file(
	GLOB_RECURSE PUBLIC_FILES CONFIGURE_DEPENDS
	${CH_PUBLIC}/core/*.h
	${CH_PUBLIC}/core/*.hpp
)

list( APPEND PUBLIC_FILES
	${CH_PUBLIC}/util.h
)

add_compile_definitions( "CORE_DLL" "TRACY_EXPORTS" )

link_libraries( SDL2 )

add_library( Core SHARED ${SRC_FILES} ${PUBLIC_FILES} ${TRACY_FILES} )

add_dependencies( Core BuildNumberUpdater )

if( MSVC )
	target_link_libraries( Core PRIVATE Shlwapi )
endif()

set_target_properties(
	Core PROPERTIES
	OUTPUT_NAME ch_core
)

target_link_libraries(
	Core PRIVATE
	SDL2
)

target_precompile_headers( Core PUBLIC "${CH_PUBLIC}/core/core.h" )

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

# Setup Copying mimalloc dll's
# if( MSVC )
# 	
# 	# cmake is dumb
# 	set( MIMALLOC_COPY_CMD_DEBUG   ${CMAKE_COMMAND} -E copy "${MIMALLOC_BUILD}/Debug/mimalloc-override.dll" "${CH_BIN}/mimalloc-override.dll" )
# 	set( MIMALLOC_COPY_CMD_RELEASE ${CMAKE_COMMAND} -E copy "${MIMALLOC_BUILD}/Release/mimalloc-override.dll" "${CH_BIN}/mimalloc-override.dll" )
# 	
# 	# set( MIMALLOC_COPY_CMD_DEBUG   xcopy /D /Y $<1:"${MIMALLOC_BUILD}/Debug/mimalloc-override.dll"> $<1:"${CH_BUILD}/mimalloc-override.dll"> )
# 	# set( MIMALLOC_COPY_CMD_RELEASE xcopy /D /Y $<1:"${MIMALLOC_BUILD}/Release/mimalloc-override.dll"> $<1:"${CH_BUILD}/mimalloc-override.dll"> )
# 	
# 	add_custom_command(
# 		TARGET Core POST_BUILD
#         COMMAND "$<$<CONFIG:Debug>:${MIMALLOC_COPY_CMD_DEBUG}>"
# 		COMMAND_EXPAND_LISTS
#         COMMENT "Copying mimalloc-override.dll to launcher directory"
#     )
# 	
# 	add_custom_command(
# 		TARGET Core POST_BUILD
#         COMMAND "$<$<CONFIG:Release>:${MIMALLOC_COPY_CMD_RELEASE}>"
# 		COMMAND_EXPAND_LISTS
#         COMMENT "Copying mimalloc-override.dll to launcher directory"
#     )
# 	
# 	add_custom_command(
# 		TARGET Core POST_BUILD
#         COMMAND "$<$<CONFIG:RelWithDebInfo>:${MIMALLOC_COPY_CMD_RELEASE}>"
# 		COMMAND_EXPAND_LISTS
#         COMMENT "Copying mimalloc-override.dll to launcher directory"
#     )
# 	
# 	add_custom_command(
# 		TARGET Core POST_BUILD
#         COMMAND "$<$<CONFIG:Profile>:${MIMALLOC_COPY_CMD_RELEASE}>"
# 		COMMAND_EXPAND_LISTS
#         COMMENT "Copying mimalloc-override.dll to launcher directory"
#     )
# 	
#     add_custom_command(
# 		TARGET Core POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy "${MIMALLOC_DIR}/bin/mimalloc-redirect.dll" "${CH_BIN}/mimalloc-redirect.dll"
#         COMMENT "Copying mimalloc-redirect.dll to launcher directory"
#     )
# endif()

