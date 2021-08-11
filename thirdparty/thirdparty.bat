@echo off
rem I'm not a batch master so feel free to improve this file

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

if not exist "../bin" mkdir "../bin"

echo Building thirdparty libraries...

set "bin_dir=..\bin\win"
set "lib_dir=..\lib"
set "inc_dir=..\inc"

:SDL2
set "SDL2_VER=2.0.16"
IF EXIST SDL2-%SDL2_VER% GOTO SDL2_COPY
echo Downloading SDL2
call download-unzip.bat https://www.libsdl.org/release SDL2-devel-%SDL2_VER%-VC.zip .
:SDL2_COPY
robocopy SDL2-%SDL2_VER%\lib\x64 %bin_dir% *.dll
robocopy SDL2-%SDL2_VER%\lib\x64 %lib_dir% *.lib
robocopy SDL2-%SDL2_VER%\include %inc_dir%\SDL2

:STEAMAUDIO
IF EXIST steamaudio_api GOTO STEAMAUDIO_COPY
echo Downloading Steam Audio
call download-unzip.bat https://github.com/ValveSoftware/steam-audio/releases/download/v2.0-beta.17 steamaudio_api_2.0-beta.17.zip .
:STEAMAUDIO_COPY
robocopy steamaudio_api\bin\Windows\x64 %bin_dir% /S
robocopy steamaudio_api\lib\Windows\x64 %lib_dir% /S
robocopy steamaudio_api\include %inc_dir%\steamaudio /S

@REM should add as submodule later
:GLM
IF EXIST glm GOTO GLM_COPY
echo Downloading GLM
call download-unzip.bat https://github.com/g-truc/glm/releases/download/0.9.9.8 glm-0.9.9.8.7z .
:GLM_COPY
robocopy glm\glm %inc_dir%\glm /S


:DONE
