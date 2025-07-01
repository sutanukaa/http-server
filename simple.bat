@echo off
echo Simple HTTP Server Builder
echo.

REM Generate random executable name to avoid conflicts
set /a RAND=%RANDOM%
set "EXEC_NAME=httpserver_%RAND%.exe"

echo Compiling with name: %EXEC_NAME%

REM Direct compilation in one step
"C:\MinGW\bin\gcc.exe" -std=c11 src\main.c -o %EXEC_NAME% -lws2_32 -lwsock32

if %errorlevel% neq 0 (
    echo.
    echo Build failed! Let me try a different approach...
    echo.
    
    REM Try two-step compilation
    "C:\MinGW\bin\gcc.exe" -std=c11 -c src\main.c -o temp.o
    if %errorlevel% neq 0 (
        echo Source compilation failed!
        pause
        exit /b 1
    )
    
    "C:\MinGW\bin\gcc.exe" temp.o -o %EXEC_NAME% -lws2_32 -lwsock32
    if %errorlevel% neq 0 (
        echo Linking failed!
        del temp.o >nul 2>&1
        pause
        exit /b 1
    )
    
    del temp.o >nul 2>&1
)

echo.
echo ✅ Build successful! 
echo ✅ Starting server on port 4221...
echo ✅ Press Ctrl+C to stop
echo.

%EXEC_NAME%

REM Clean up after server stops
echo.
echo Cleaning up...
del %EXEC_NAME% >nul 2>&1
