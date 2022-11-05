
set( KTX_DIR ${CH_THIRDPARTY}/KTX-Software )

if( MSVC )
	include_directories( "${KTX_DIR}/include" )
	
	if( (CMAKE_BUILD_TYPE STREQUAL Debug) )
		set( KTX_LIB_DIR ${KTX_DIR}/build/Debug )
	else()
		set( KTX_LIB_DIR ${KTX_DIR}/build/Release )
	endif()
	
	link_libraries( ${KTX_LIB_DIR}/ktx.lib ${KTX_LIB_DIR}/ktx_read.lib )
	
	configure_file( "${KTX_LIB_DIR}/ktx.dll" "${CH_BUILD}/ktx.dll" COPYONLY )
	configure_file( "${KTX_LIB_DIR}/ktx_read.dll" "${CH_BUILD}/ktx_read.dll" COPYONLY )
	
	# uhh
	if( (CMAKE_BUILD_TYPE STREQUAL Debug) )
		configure_file( "${KTX_LIB_DIR}/ktx.pdb" "${CH_BUILD}/ktx.pdb" COPYONLY )
		configure_file( "${KTX_LIB_DIR}/ktx_read.pdb" "${CH_BUILD}/ktx_read.pdb" COPYONLY )
	endif()
	
else()
	include_directories( "${KTX_DIR}/include" )
	set( KTX_LIB_DIR ${KTX_DIR}/build )
	link_directories( ${KTX_DIR}/build )
	link_libraries( ${KTX_LIB_DIR}/libktx.so ${KTX_LIB_DIR}/libktx_read.so )
	
	configure_file( "${KTX_LIB_DIR}/libktx.so" "${CH_BUILD}/libktx.so" COPYONLY )
	configure_file( "${KTX_LIB_DIR}/libktx_read.so" "${CH_BUILD}/libktx_read.so" COPYONLY )
endif()

