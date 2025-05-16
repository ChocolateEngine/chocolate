message("Current Project: Chocolate Engine Toolkit")

file(
    GLOB_RECURSE SRC_FILES_GLOB CONFIGURE_DEPENDS
    *.cpp
    *.h
    ../shared/*.cpp
    ../shared/*.h

    # Sidury Shared files
    ../../shared/*.cpp
    ../../shared/*.h

    # This is to include the material editor stuff in here
    ../modules/material_editor/*.cpp

    # Map Editor
    ../modules/map_editor/*.cpp
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
    "../shared"
    "../../shared"
    "../modules"
)

add_library(Toolkit SHARED ${SRC_FILES} ${PUBLIC_FILES} ${THIRDPARTY_FILES})

target_link_libraries(
    Toolkit
    PRIVATE
    Core
    ImGui
    SDL2
)


add_dependencies(Toolkit "Core" "ImGui")

set_target_properties(
    Toolkit PROPERTIES
    RUNTIME_OUTPUT_NAME ch_toolkit
    LIBRARY_OUTPUT_NAME ch_toolkit

    RUNTIME_OUTPUT_DIRECTORY ${CH_OUTPUT}/toolkit/bin/${PLAT_FOLDER}
    LIBRARY_OUTPUT_DIRECTORY ${CH_OUTPUT}/toolkit/bin/${PLAT_FOLDER}
)

# set output directories for all builds (Debug, Release, etc.)
foreach (OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set_target_properties(
        Toolkit PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_OUTPUT}/toolkit/bin/${PLAT_FOLDER}
        LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CH_OUTPUT}/toolkit/bin/${PLAT_FOLDER}
    )
endforeach (OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

target_precompile_headers(Toolkit PRIVATE "${CH_PUBLIC}/core/core.h")

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

