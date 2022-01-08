
set( KTX_DIR ${CH_THIRDPARTY}/KTX-Software-4.0.0 )

if( MSVC )
	include_directories( "${KTX_DIR}/include" )
	set( KTX_LIB_DIR ${KTX_DIR}/out/build/x64-Debug )
	link_libraries( ${KTX_LIB_DIR}/ktx.lib ${KTX_LIB_DIR}/ktx_read.lib )
else()
	# idfk
	link_libraries( ktx ktx_read )
endif()

