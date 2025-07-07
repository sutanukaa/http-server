@echo off
echo Testing Connection: close header functionality
echo.

REM Start the server in background (if available)
echo Starting test...

REM Test 1: Regular request (should keep connection open)
echo.
echo Test 1: Regular request without Connection header
curl -v http://localhost:4221/echo/orange 2>&1 | findstr /i "connection"

echo.
echo Test 2: Request with Connection: close header  
curl -v -H "Connection: close" http://localhost:4221/ 2>&1 | findstr /i "connection"

echo.
echo Test 3: Simulating the actual test case
curl --http1.1 -v http://localhost:4221/echo/orange --next http://localhost:4221/ -H "Connection: close" 2>&1 | findstr /i "connection"

echo.
echo Tests completed. Check server logs to verify behavior.
pause
