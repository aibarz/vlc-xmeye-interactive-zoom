@echo off
setlocal
set "VLC=%~1"
if "%VLC%"=="" set "VLC=C:\Program Files\VideoLAN\VLC"

if not exist "%VLC%\vlc.exe" (
  echo ERROR: no se encuentra VLC en: %VLC%
  exit /b 2
)

tasklist /FI "IMAGENAME eq vlc.exe" | find /I "vlc.exe" >nul
if not errorlevel 1 (
  echo ERROR: VLC esta abierto. Cierralo antes de retirar el plugin.
  exit /b 3
)

if exist "%VLC%\plugins\control\libxmeyezoom_plugin.dll" del /Q "%VLC%\plugins\control\libxmeyezoom_plugin.dll"
if exist "%VLC%\plugins\control\libxmeyezoom_plugin.dll.previous.bak" (
  move /Y "%VLC%\plugins\control\libxmeyezoom_plugin.dll.previous.bak" "%VLC%\plugins\control\libxmeyezoom_plugin.dll" >nul
)

if exist "%VLC%\vlc-cache-gen.exe" "%VLC%\vlc-cache-gen.exe" "%VLC%\plugins"
echo OK: XMEye Interactive Zoom retirado.
endlocal
