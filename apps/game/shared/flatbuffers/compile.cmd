@echo off

@REM set "flatc=%cd%\..\..\..\..\chocolate\thirdparty\flatbuffers\build\Release\flatc.exe"
set "flatc=D:\projects\chocolate\dev\chocolate\thirdparty\flatbuffers\build\Release\flatc.exe"
"%flatc%" -c "sidury.fbs"

pause