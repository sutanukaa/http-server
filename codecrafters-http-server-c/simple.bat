@echo off
echo Multi-threaded HTTP Server Builder
echo.

REM Generate random executable name to avoid conflicts
set /a RAND=%RANDOM%
set "EXEC_NAME=httpserver_%RAND%.exe"

echo Compiling with name: %EXEC_NAME%

REM Direct compilation in one step with threading support
"C:\MinGW\bin\gcc.exe" -std=c11 src\main.c -o %EXEC_NAME% -lws2_32 -lwsock32 -lz

if %errorlevel% neq 0 (
    echo.
    echo Build failed! Let me try without zlib...
    echo.
    
    REM Try without zlib
    "C:\MinGW\bin\gcc.exe" -std=c11 src\main.c -o %EXEC_NAME% -lws2_32 -lwsock32
    if %errorlevel% neq 0 (
        echo Source compilation failed!
        pause
        exit /b 1
    )
)

echo.
echo ✅ Build successful! 
echo ✅ Starting multi-threaded server on port 4221...
echo ✅ Server now supports concurrent connections!
echo ✅ Press Ctrl+C to stop
echo.

%EXEC_NAME%

REM Clean up after server stops
echo.
echo Cleaning up...
del %EXEC_NAME% >nul 2>&1
