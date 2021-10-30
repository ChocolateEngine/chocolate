cmake_minimum_required(VERSION 3.5)

if(NOT DEFINED QPC_PLATFORM)
	set(QPC_PLATFORM "LINUX")
endif()

if(NOT DEFINED QPC_ARCH)
	set(QPC_ARCH "I386")
endif()

if( (CMAKE_BUILD_TYPE STREQUAL Debug) AND (QPC_PLATFORM STREQUAL LINUX) AND (QPC_ARCH STREQUAL I386) )

	set(
		CHOCOLATE_SRC_FILES
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/util.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/system.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/msgs.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/platform.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/console.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/thirdparty/speedykeyv/KeyValue.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_impl_vulkan.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_demo.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_widgets.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_draw.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_impl_sdl.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_tables.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/graphics.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/codec_wav.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/audio.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/main.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/input.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/gui.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/codec_vorbis.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/codec_libav.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/engine.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/device.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/layoutbuilder.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/descriptorcache.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/shaders.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/shadercache.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/material.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/renderer.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/pipelinebuilder.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/materialsystem.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/modelloader.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/allocator.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/types/modeldata.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/types/spritedata.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/types/transform.cpp"
	)

	set(
		CHOCOLATE_INC_FILES
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/msgs.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/platform.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/system.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/util.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/baseaudio.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/basegui.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/commandmanager.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/console.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/basegraphics.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/systemmanager.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/baseinput.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/thirdparty/speedykeyv/KeyValue.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imgui.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imstb_textedit.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imgui_internal.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imgui_impl_sdl.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imstb_truetype.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imstb_rectpack.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imconfig.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imgui_impl_vulkan.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/codec_vorbis.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/engine.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/codec_libav.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/input.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/graphics.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/debug.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/gui.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/audio.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/codec_wav.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/shadercache.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/layoutbuilder.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/shaders.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/initializers.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/modelloader.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/allocator.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/pipelinebuilder.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/materialsystem.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/device.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/renderer.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/material.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/descriptorcache.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/missingtexture.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/modeldata.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/transform.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/msg.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/spritedata.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/command.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/renderertypes.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/enums.h"
	)

	add_library( CHOCOLATE SHARED ${CHOCOLATE_SRC_FILES} ${CHOCOLATE_INC_FILES} )

	set_target_properties(
		CHOCOLATE PROPERTIES
		PREFIX ""
		OUTPUT_NAME "engine"
		CXX_COMPILER "g++"
		CXX_STANDARD 20
		RUNTIME_OUTPUT_DIRECTORY "/home/karl/Desktop/Git-Repos/sidury/sidury/bin"
		ARCHIVE_OUTPUT_DIRECTORY "/home/karl/Desktop/Git-Repos/sidury/obj/engine/Debug"
	)

	target_include_directories(
		CHOCOLATE PRIVATE
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/scripts/chocolate/inc"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/lib/io"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/SDL2"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/steamaudio"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/thirdparty/speedykeyv"
	)

	target_link_directories(
		CHOCOLATE PRIVATE
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/scripts/chocolate/lib"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/scripts/$VK_DIR$/Lib"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/lib"
	)

	target_link_libraries(
		CHOCOLATE
		SDL2
		vulkan
		dl
		pthread
		X11
		Xxf86vm
		Xrandr
		Xi
	)

	target_compile_definitions(
		CHOCOLATE PRIVATE
		-D_X86_
		-D__i386__
		-D_DEBUG=1
	)

	target_compile_options(
		CHOCOLATE PRIVATE
		-std=c++20
	)

elseif( (CMAKE_BUILD_TYPE STREQUAL Debug) AND (QPC_PLATFORM STREQUAL LINUX) AND (QPC_ARCH STREQUAL AMD64) )

	set(
		CHOCOLATE_SRC_FILES
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/util.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/system.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/msgs.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/platform.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/console.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/thirdparty/speedykeyv/KeyValue.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_impl_vulkan.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_demo.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_widgets.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_draw.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_impl_sdl.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_tables.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/graphics.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/codec_wav.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/audio.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/main.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/input.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/gui.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/codec_vorbis.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/codec_libav.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/engine.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/device.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/layoutbuilder.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/descriptorcache.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/shaders.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/shadercache.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/material.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/renderer.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/pipelinebuilder.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/materialsystem.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/modelloader.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/allocator.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/types/modeldata.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/types/spritedata.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/types/transform.cpp"
	)

	set(
		CHOCOLATE_INC_FILES
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/msgs.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/platform.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/system.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/util.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/baseaudio.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/basegui.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/commandmanager.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/console.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/basegraphics.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/systemmanager.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/baseinput.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/thirdparty/speedykeyv/KeyValue.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imgui.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imstb_textedit.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imgui_internal.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imgui_impl_sdl.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imstb_truetype.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imstb_rectpack.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imconfig.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imgui_impl_vulkan.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/codec_vorbis.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/engine.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/codec_libav.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/input.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/graphics.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/debug.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/gui.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/audio.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/codec_wav.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/shadercache.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/layoutbuilder.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/shaders.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/initializers.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/modelloader.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/allocator.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/pipelinebuilder.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/materialsystem.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/device.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/renderer.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/material.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/descriptorcache.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/missingtexture.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/modeldata.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/transform.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/msg.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/spritedata.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/command.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/renderertypes.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/enums.h"
	)

	add_library( CHOCOLATE SHARED ${CHOCOLATE_SRC_FILES} ${CHOCOLATE_INC_FILES} )

	set_target_properties(
		CHOCOLATE PROPERTIES
		PREFIX ""
		OUTPUT_NAME "engine"
		CXX_COMPILER "g++"
		CXX_STANDARD 20
		RUNTIME_OUTPUT_DIRECTORY "/home/karl/Desktop/Git-Repos/sidury/sidury/bin"
		ARCHIVE_OUTPUT_DIRECTORY "/home/karl/Desktop/Git-Repos/sidury/obj/engine/Debug"
	)

	target_include_directories(
		CHOCOLATE PRIVATE
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/scripts/chocolate/inc"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/lib/io"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/SDL2"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/steamaudio"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/thirdparty/speedykeyv"
	)

	target_link_directories(
		CHOCOLATE PRIVATE
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/scripts/chocolate/lib"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/scripts/$VK_DIR$/Lib"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/lib"
	)

	target_link_libraries(
		CHOCOLATE
		SDL2
		vulkan
		dl
		pthread
		X11
		Xxf86vm
		Xrandr
		Xi
	)

	target_compile_definitions(
		CHOCOLATE PRIVATE
		-D_AMD64_
		-D__x86_64
		-D__amd64
		-D_DEBUG=1
	)

	target_compile_options(
		CHOCOLATE PRIVATE
		-std=c++20
	)

elseif( (CMAKE_BUILD_TYPE STREQUAL Release) AND (QPC_PLATFORM STREQUAL LINUX) AND (QPC_ARCH STREQUAL I386) )

	set(
		CHOCOLATE_SRC_FILES
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/util.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/system.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/msgs.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/platform.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/console.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/thirdparty/speedykeyv/KeyValue.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_impl_vulkan.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_demo.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_widgets.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_draw.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_impl_sdl.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_tables.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/graphics.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/codec_wav.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/audio.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/main.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/input.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/gui.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/codec_vorbis.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/codec_libav.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/engine.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/device.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/layoutbuilder.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/descriptorcache.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/shaders.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/shadercache.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/material.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/renderer.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/pipelinebuilder.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/materialsystem.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/modelloader.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/allocator.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/types/modeldata.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/types/spritedata.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/types/transform.cpp"
	)

	set(
		CHOCOLATE_INC_FILES
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/msgs.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/platform.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/system.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/util.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/baseaudio.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/basegui.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/commandmanager.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/console.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/basegraphics.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/systemmanager.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/baseinput.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/thirdparty/speedykeyv/KeyValue.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imgui.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imstb_textedit.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imgui_internal.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imgui_impl_sdl.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imstb_truetype.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imstb_rectpack.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imconfig.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imgui_impl_vulkan.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/codec_vorbis.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/engine.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/codec_libav.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/input.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/graphics.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/debug.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/gui.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/audio.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/codec_wav.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/shadercache.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/layoutbuilder.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/shaders.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/initializers.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/modelloader.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/allocator.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/pipelinebuilder.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/materialsystem.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/device.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/renderer.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/material.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/descriptorcache.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/missingtexture.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/modeldata.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/transform.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/msg.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/spritedata.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/command.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/renderertypes.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/enums.h"
	)

	add_library( CHOCOLATE SHARED ${CHOCOLATE_SRC_FILES} ${CHOCOLATE_INC_FILES} )

	set_target_properties(
		CHOCOLATE PROPERTIES
		PREFIX ""
		OUTPUT_NAME "engine"
		CXX_COMPILER "g++"
		CXX_STANDARD 20
		RUNTIME_OUTPUT_DIRECTORY "/home/karl/Desktop/Git-Repos/sidury/sidury/bin"
		ARCHIVE_OUTPUT_DIRECTORY "/home/karl/Desktop/Git-Repos/sidury/obj/engine/Release"
	)

	target_include_directories(
		CHOCOLATE PRIVATE
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/scripts/chocolate/inc"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/lib/io"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/SDL2"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/steamaudio"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/thirdparty/speedykeyv"
	)

	target_link_directories(
		CHOCOLATE PRIVATE
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/scripts/chocolate/lib"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/scripts/$VK_DIR$/Lib"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/lib"
	)

	target_link_libraries(
		CHOCOLATE
		SDL2
		vulkan
		dl
		pthread
		X11
		Xxf86vm
		Xrandr
		Xi
	)

	target_compile_definitions(
		CHOCOLATE PRIVATE
		-D_X86_
		-D__i386__
		-DNDEBUG=1
	)

	target_compile_options(
		CHOCOLATE PRIVATE
		-std=c++20
	)

elseif( (CMAKE_BUILD_TYPE STREQUAL Release) AND (QPC_PLATFORM STREQUAL LINUX) AND (QPC_ARCH STREQUAL AMD64) )

	set(
		CHOCOLATE_SRC_FILES
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/util.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/system.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/msgs.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/platform.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/shared/console.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/thirdparty/speedykeyv/KeyValue.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_impl_vulkan.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_demo.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_widgets.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_draw.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_impl_sdl.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/imgui/imgui_tables.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/graphics.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/codec_wav.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/audio.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/main.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/input.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/gui.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/codec_vorbis.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/codec_libav.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/engine.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/device.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/layoutbuilder.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/descriptorcache.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/shaders.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/shadercache.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/material.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/renderer.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/pipelinebuilder.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/materialsystem.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/modelloader.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/core/renderer/allocator.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/types/modeldata.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/types/spritedata.cpp"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/src/types/transform.cpp"
	)

	set(
		CHOCOLATE_INC_FILES
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/msgs.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/platform.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/system.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/util.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/baseaudio.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/basegui.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/commandmanager.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/console.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/basegraphics.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/systemmanager.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared/baseinput.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/thirdparty/speedykeyv/KeyValue.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imgui.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imstb_textedit.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imgui_internal.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imgui_impl_sdl.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imstb_truetype.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imstb_rectpack.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imconfig.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui/imgui_impl_vulkan.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/codec_vorbis.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/engine.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/codec_libav.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/input.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/graphics.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/debug.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/gui.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/audio.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/codec_wav.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/shadercache.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/layoutbuilder.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/shaders.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/initializers.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/modelloader.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/allocator.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/pipelinebuilder.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/materialsystem.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/device.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/renderer.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/material.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core/renderer/descriptorcache.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/missingtexture.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/modeldata.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/transform.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/msg.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/spritedata.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/command.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/renderertypes.h"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types/enums.h"
	)

	add_library( CHOCOLATE SHARED ${CHOCOLATE_SRC_FILES} ${CHOCOLATE_INC_FILES} )

	set_target_properties(
		CHOCOLATE PROPERTIES
		PREFIX ""
		OUTPUT_NAME "engine"
		CXX_COMPILER "g++"
		CXX_STANDARD 20
		RUNTIME_OUTPUT_DIRECTORY "/home/karl/Desktop/Git-Repos/sidury/sidury/bin"
		ARCHIVE_OUTPUT_DIRECTORY "/home/karl/Desktop/Git-Repos/sidury/obj/engine/Release"
	)

	target_include_directories(
		CHOCOLATE PRIVATE
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/scripts/chocolate/inc"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/lib/io"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/core"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/imgui"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/shared"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/types"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/SDL2"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/inc/steamaudio"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/thirdparty/speedykeyv"
	)

	target_link_directories(
		CHOCOLATE PRIVATE
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/scripts/chocolate/lib"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/scripts/$VK_DIR$/Lib"
		"/home/karl/Desktop/Git-Repos/sidury/chocolate/lib"
	)

	target_link_libraries(
		CHOCOLATE
		SDL2
		vulkan
		dl
		pthread
		X11
		Xxf86vm
		Xrandr
		Xi
	)

	target_compile_definitions(
		CHOCOLATE PRIVATE
		-D_AMD64_
		-D__x86_64
		-D__amd64
		-DNDEBUG=1
	)

	target_compile_options(
		CHOCOLATE PRIVATE
		-std=c++20
	)

endif()

