@echo off

IF NOT DEFINED VSCMD_VER (
	echo Please run this file in the Developer Command Prompt for Visual Studio
	pause
	EXIT /B
)

rem Check for 7za
WHERE 7za >nul 2>nul
IF %ERRORLEVEL% NEQ 0 (
	echo Failed! Could not find 7za! Please download the standalone console version of 7-Zip from https://www.7-zip.org/download.html
	pause
	EXIT /B
)

echo Downloading thirdparty libraries...

set "inc_dir=..\public"

:SDL2
set "SDL2_VER=2.0.16"
IF EXIST SDL2-%SDL2_VER% GOTO SDL2_COPY
echo Downloading SDL2
call download-unzip.bat https://www.libsdl.org/release SDL2-devel-%SDL2_VER%-VC.zip .
:SDL2_COPY
robocopy SDL2-%SDL2_VER%\include %inc_dir%\SDL2 /S
mv SDL2-%SDL2_VER% SDL2

:STEAMAUDIO
IF EXIST steamaudio_api GOTO VORBIS
echo Downloading Steam Audio
call download-unzip.bat https://github.com/ValveSoftware/steam-audio/releases/download/v2.0-beta.17 steamaudio_api_2.0-beta.17.zip .

@REM should add as submodule later
:VORBIS
IF EXIST libvorbis GOTO OGG
echo Downloading libvorbis
call download-unzip.bat https://github.com/xiph/vorbis/releases/download/v1.3.7 libvorbis-1.3.7.zip .
mv libvorbis-1.3.7 libvorbis

:OGG
@REM curl fails to download this?
@REM IF EXIST libogg GOTO GLM
@REM echo Downloading libogg
@REM call download-unzip.bat https://downloads.xiph.org/releases/ogg libogg-1.3.5.zip .
@REM mv libogg-1.3.5 libogg

@REM should add as submodule later
:GLM
IF EXIST glm GOTO GLM_COPY
echo Downloading GLM
call download-unzip.bat https://github.com/g-truc/glm/releases/download/0.9.9.8 glm-0.9.9.8.7z .
:GLM_COPY
robocopy glm\glm %inc_dir%\glm /S

:KTX
IF EXIST KTX-Software-4.0.0 GOTO KTX_COPY
echo Downloading KTX
call download-unzip.bat https://github.com/KhronosGroup/KTX-Software/archive/refs/tags v4.0.0.zip .
:KTX_COPY
@REM robocopy KTX-Software-4.0.0\include %inc_dir%\ktx /S

:DONE
