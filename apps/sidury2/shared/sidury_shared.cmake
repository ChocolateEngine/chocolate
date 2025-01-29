set( SIDURY_SHARED_DIR ${CMAKE_CURRENT_LIST_DIR} )

set(
	SIDURY_SHARED_SRC_FILES

    ${SIDURY_SHARED_DIR}/main.cpp
    ${SIDURY_SHARED_DIR}/main.h
    ${SIDURY_SHARED_DIR}/game_shared.cpp
    ${SIDURY_SHARED_DIR}/game_shared.h
    ${SIDURY_SHARED_DIR}/game_interface.h
    ${SIDURY_SHARED_DIR}/map_manager.cpp
    ${SIDURY_SHARED_DIR}/map_manager.h
    ${SIDURY_SHARED_DIR}/game_physics.cpp
    ${SIDURY_SHARED_DIR}/game_physics.h
    ${SIDURY_SHARED_DIR}/button_inputs.h
	
	# networking
	${SIDURY_SHARED_DIR}/network/net_main.cpp
	${SIDURY_SHARED_DIR}/network/net_main.h
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
	"${SIDURY_SHARED_DIR}"
	"../../"
	"../../graphics"
)

