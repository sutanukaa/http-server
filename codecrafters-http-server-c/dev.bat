@echo off
echo Compiling HTTP server...

REM Kill any running server processes
taskkill /f /im server.exe >nul 2>&1
taskkill /f /im http-server.exe >nul 2>&1

REM Wait a moment for process cleanup
timeout /t 1 >nul 2>&1

REM Try multiple times to delete old files
for /l %%i in (1,1,5) do (
    del server.exe >nul 2>&1
    if not exist server.exe goto :compile
    timeout /t 1 >nul 2>&1
)

:compile
del main.o >nul 2>&1

REM Compile to object file
echo Compiling source...
"C:\MinGW\bin\gcc.exe" -std=c11 -c src\main.c -o main.o

if %errorlevel% neq 0 (
    echo Compilation failed!
    pause
    exit /b 1
)

REM Try different executable names if needed
echo Linking executable...
"C:\MinGW\bin\gcc.exe" main.o -o server.exe -lws2_32 -lwsock32

if %errorlevel% neq 0 (
    echo Trying alternative name...
    "C:\MinGW\bin\gcc.exe" main.o -o myserver.exe -lws2_32 -lwsock32
    if %errorlevel% neq 0 (
        echo Linking failed!
        pause
        exit /b 1
    )
    set "EXEC_NAME=myserver.exe"
) else (
    set "EXEC_NAME=server.exe"
)

REM Clean up object file
del main.o >nul 2>&1

echo Build successful! Starting server on port 4221...
echo Press Ctrl+C to stop the server
echo Running: %EXEC_NAME%
%EXEC_NAME%
