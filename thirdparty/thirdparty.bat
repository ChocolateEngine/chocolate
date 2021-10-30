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
IF EXIST SDL2-%SDL2_VER% GOTO STEAMAUDIO
echo Downloading SDL2
call download-unzip.bat https://www.libsdl.org/release SDL2-devel-%SDL2_VER%-VC.zip .

:STEAMAUDIO
IF EXIST steamaudio_api GOTO GLM
echo Downloading Steam Audio
call download-unzip.bat https://github.com/ValveSoftware/steam-audio/releases/download/v2.0-beta.17 steamaudio_api_2.0-beta.17.zip .

@REM should add as submodule later
:GLM
IF EXIST glm GOTO GLM_COPY
echo Downloading GLM
call download-unzip.bat https://github.com/g-truc/glm/releases/download/0.9.9.8 glm-0.9.9.8.7z .
:GLM_COPY
robocopy glm\glm %inc_dir%\glm /S

:OGG
IF EXIST libogg GOTO DONE
echo Downloading OGG
call download-unzip.bat https://github.com/g-truc/glm/releases/download/0.9.9.8 glm-0.9.9.8.7z .


:DONE
