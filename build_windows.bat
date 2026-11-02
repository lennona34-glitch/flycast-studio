@echo off
setlocal enabledelayedexpansion

echo =====================================================================
echo                   Flycast-XR Studio - Windows Build
echo =====================================================================
echo.

:: 1. Check for Git and initialize submodules
echo [1/4] Checking Git submodules...
where git >nul 2>nul
if %ERRORLEVEL% equ 0 (
    if exist ".git" (
        echo Updating git submodules recursively (SDL, glslang, libchdr, etc.)...
        git submodule update --init --recursive
        if %ERRORLEVEL% neq 0 (
            echo [WARNING] Submodule update returned a warning or non-zero code. Proceeding...
        )
    ) else (
        echo [.git not found, skipping submodule update]
    )
) else (
    echo [WARNING] Git not found in PATH. Ensure submodules are checked out if building from source.
)

:: 2. Check for CMake
echo.
echo [2/4] Checking CMake...
where cmake >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake is not installed or not in your system PATH.
    echo Please install CMake from https://cmake.org/download/ and make sure to check
    echo "Add CMake to system PATH".
    goto error
)

:: 3. Configure with CMake
echo.
echo [3/4] Configuring project with Visual Studio 2022 (x64 Release)...
cmake -B build -A x64 -DCMAKE_BUILD_TYPE=Release -DUSE_DX9=OFF
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed!
    echo Please ensure Visual Studio 2022 with "Desktop development with C++" is installed.
    goto error
)

:: 4. Build Flycast Studio
echo.
echo [4/4] Building Flycast Studio (Release x64)...
cmake --build build --config Release --parallel
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed! Check the compiler errors above.
    goto error
)

:: Verify output binary
if exist "build\Release\flycast.exe" (
    echo.
    echo =====================================================================
    echo [SUCCESS] Flycast-XR Studio built successfully!
    echo Binary located at: build\Release\flycast.exe
    echo =====================================================================
    echo.
    if not exist "dist" mkdir dist
    copy /Y "build\Release\flycast.exe" "dist\flycast.exe" >nul
    echo Copied executable to: dist\flycast.exe
    echo.
) else (
    echo [ERROR] Build completed but build\Release\flycast.exe was not found.
    goto error
)

goto end

:error
echo.
echo =====================================================================
echo [BUILD FAILED] Please address the errors above.
echo =====================================================================
echo.
pause
exit /b 1

:end
echo Press any key to exit.
pause >nul
exit /b 0
