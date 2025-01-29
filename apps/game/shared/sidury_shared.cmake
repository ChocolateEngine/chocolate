set( SIDURY_SHARED_DIR ${CMAKE_CURRENT_LIST_DIR} )

set(
	SIDURY_SHARED_SRC_FILES

    ${SIDURY_SHARED_DIR}/main.cpp
    ${SIDURY_SHARED_DIR}/main.h
    ${SIDURY_SHARED_DIR}/game_shared.cpp
    ${SIDURY_SHARED_DIR}/game_shared.h

    ${SIDURY_SHARED_DIR}/mapmanager.cpp
    ${SIDURY_SHARED_DIR}/mapmanager.h
    ${SIDURY_SHARED_DIR}/skybox.cpp
    ${SIDURY_SHARED_DIR}/skybox.h

    ${SIDURY_SHARED_DIR}/game_physics.cpp
    ${SIDURY_SHARED_DIR}/game_physics.h
    ${SIDURY_SHARED_DIR}/game_rules.cpp
    ${SIDURY_SHARED_DIR}/game_rules.h
    ${SIDURY_SHARED_DIR}/sound.cpp
    ${SIDURY_SHARED_DIR}/sound.h
    ${SIDURY_SHARED_DIR}/steam.cpp
    ${SIDURY_SHARED_DIR}/steam.h
    ${SIDURY_SHARED_DIR}/testing.cpp
    ${SIDURY_SHARED_DIR}/testing.h
    ${SIDURY_SHARED_DIR}/player.cpp
    ${SIDURY_SHARED_DIR}/player.h
    ${SIDURY_SHARED_DIR}/button_inputs.h

    # entity system
    ${SIDURY_SHARED_DIR}/entity/entity.cpp
    ${SIDURY_SHARED_DIR}/entity/entity.h
    ${SIDURY_SHARED_DIR}/entity/entity_component_pool.cpp
    ${SIDURY_SHARED_DIR}/entity/entity_components_base.cpp
    ${SIDURY_SHARED_DIR}/entity/entity_serialization.cpp
    ${SIDURY_SHARED_DIR}/entity/entity_systems.cpp
    ${SIDURY_SHARED_DIR}/entity/entity_systems.h
	
	# networking
	${SIDURY_SHARED_DIR}/network/net_main.cpp
	${SIDURY_SHARED_DIR}/network/net_main.h

	../../shared/map_system.cpp
	../../shared/map_system.h
	# ../../shared/skybox.cpp
	# ../../shared/skybox.h
	
	# tools
	# ${SIDURY_SHARED_DIR}/tools/light_editor.cpp
	# ${SIDURY_SHARED_DIR}/tools/light_editor.h
)

if( WIN32 )
	set(
		SIDURY_SHARED_SRC_FILES
		${SIDURY_SHARED_SRC_FILES}
		
		# networking
		${SIDURY_SHARED_DIR}/network/net_windows.cpp
	)
else()
    set(
        SIDURY_SHARED_SRC_FILES
        ${SIDURY_SHARED_SRC_FILES}
        
        # networking
        ${SIDURY_SHARED_DIR}/network/net_linux.cpp
    )
endif()

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

include_directories(
	"${CH_THIRDPARTY}/flatbuffers/include"
	"${SIDURY_SHARED_DIR}"
	"../../"
    "../../shared/"
)

set( FLATBUFFERS_DIR_DEBUG "${CH_THIRDPARTY}/flatbuffers/build/Debug" )
set( FLATBUFFERS_DIR_RELEASE "${CH_THIRDPARTY}/flatbuffers/build/Release" )

link_directories(
	"$<$<CONFIG:Debug>:${FLATBUFFERS_DIR_DEBUG}>"
	"$<$<CONFIG:Release>:${FLATBUFFERS_DIR_RELEASE}>"
	"$<$<CONFIG:RelWithDebInfo>:${FLATBUFFERS_DIR_RELEASE}>"
	"$<$<CONFIG:Profile>:${FLATBUFFERS_DIR_RELEASE}>"
)

