@echo off
rem baseUrl file outPath

WHERE 7za >nul 2>nul
IF %ERRORLEVEL% NEQ 0 (
	echo Failed! Could not find 7za! Please download the standalone console version of 7-Zip from https://www.7-zip.org/download.html
	pause
	EXIT /B
)

curl -L "%1/%2" --output "%2"
7za x -y "%2" -o"%3" 
del %2