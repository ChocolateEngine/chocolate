
set( KTX_DIR ${CH_THIRDPARTY}/KTX-Software )

if( MSVC )
	include_directories( "${KTX_DIR}/include" )
	set( KTX_LIB_DIR ${KTX_DIR}/out/build/x64-Debug )
	link_libraries( ${KTX_LIB_DIR}/ktx.lib ${KTX_LIB_DIR}/ktx_read.lib )
else()
  # idfk
	include_directories( "${KTX_DIR}/include" )
	set( KTX_LIB_DIR ${KTX_DIR}/build )
	link_directories( ${KTX_DIR}/build )
	link_libraries( ktx ktx_read )
	configure_file( "${KTX_LIB_DIR}/libktx.so" "${GAME_DIR}/libktx.so" COPYONLY )
	configure_file( "${KTX_LIB_DIR}/libktx_read.so" "${GAME_DIR}/libktx_read.so" COPYONLY )
endif()

