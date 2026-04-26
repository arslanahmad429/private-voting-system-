@echo off
echo ===========================================
echo   VOTING SYSTEM - ALL-IN-ONE BUILD SCRIPT
echo ===========================================
echo.
echo Compiling the consolidated server...

gcc -std=c99 -O2 -o server.exe server.c mongoose.c sqlite3.c -lws2_32 -Wno-unused-result -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-variable -Wno-implicit-function-declaration

if %ERRORLEVEL% neq 0 (
    echo.
    echo Compilation failed! Please check your compiler setup.
    pause
    exit /b %ERRORLEVEL%
)

echo Compilation successful! Starting the server...
echo.
echo Point your browser to: http://localhost:5000/
echo Default admin credentials:
echo   Email: admin@votingsystem.com
echo   Password: Admin@123
echo.
echo Excel Reports Auto-generated in: data/System_Data.xls
echo.

server.exe
pause