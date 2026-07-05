@echo off
cd /d "%~dp0"
if not exist build\NBodyProblem.exe (
    echo Build the project first:
    echo   cd build
    echo   cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
    echo   cmake --build .
    pause
    exit /b 1
)
cmake --build build --target NBodyProblem >nul 2>&1
cd /d "%~dp0dist"
if not exist NBodyProblem.exe (
    echo Build the project first:
    echo   cd build
    echo   cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
    echo   cmake --build .
    pause
    exit /b 1
)
start "" "%~dp0dist\NBodyProblem.exe"
