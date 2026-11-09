@echo off
title Flycast VR - Meta Quest 6DoF Instant Launcher
color 0B
echo ================================================================
echo        FLYCAST VR - META QUEST 6DOF LAUNCHER
echo ================================================================
echo.

echo [1/3] Checking ADB Wireless connection to Meta Quest...
adb devices
echo.

echo [2/3] Setting up ADB Reverse Tunnel (Port 8765)...
echo       This grants Quest Browser instant Secure Context (6DoF WebXR enabled)!
adb reverse tcp:8765 tcp:8765
if %ERRORLEVEL% NEQ 0 (
    echo [!] ADB reverse returned a warning or Quest is connecting.
) else (
    echo [*] Port 8765 reversed successfully!
)
echo.

echo [3/3] Opening Meta Quest Browser to http://localhost:8765 ...
adb shell am start -a android.intent.action.VIEW -d "http://localhost:8765" com.oculus.browser >nul 2>&1
echo [*] Browser launch signal sent to Quest headset!
echo.

echo Launching Flycast with VR Broadcast active...
if exist "%~dp0flycast.exe" (
    start "" "%~dp0flycast.exe" --vr-broadcast
) else if exist "%~dp0build\Release\flycast.exe" (
    start "" "%~dp0build\Release\flycast.exe" --vr-broadcast
) else (
    start "" "C:\Users\adria\.gemini\antigravity\scratch\flycast-2.7\build\Release\flycast.exe" --vr-broadcast
)

echo.
echo ================================================================
echo  Flycast is running!
echo  Put on your Quest headset - the browser is loading localhost:8765.
echo  Click "Enter Mixed Reality" or "Enter VR Theater" to play!
echo ================================================================
timeout /t 5
