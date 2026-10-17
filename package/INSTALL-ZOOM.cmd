@echo off
setlocal
set "VLC=%~1"
if not "%VLC%"=="" goto :found
if exist "C:\Program Files\VideoLAN\VLC\vlc.exe" set "VLC=C:\Program Files\VideoLAN\VLC"
if not "%VLC%"=="" goto :found
if exist "D:\Program Files\VideoLAN.VLC\vlc.exe" set "VLC=D:\Program Files\VideoLAN.VLC"
if not "%VLC%"=="" goto :found
if exist "D:\Program Files\VideoLAN\VLC\vlc.exe" set "VLC=D:\Program Files\VideoLAN\VLC"
:found
if "%VLC%"=="" (
  echo ERROR: no se ha localizado VLC 3.0.24 x64.
  echo Uso: INSTALL-ZOOM.cmd "C:\ruta\a\VLC"
  exit /b 2
)
if not exist "%VLC%\vlc.exe" (
  echo ERROR: no se encuentra VLC en: %VLC%
  exit /b 2
)
if not exist "%VLC%\plugins\control" (
  echo ERROR: no existe %VLC%\plugins\control
  exit /b 3
)
tasklist /FI "IMAGENAME eq vlc.exe" | find /I "vlc.exe" >nul
if not errorlevel 1 (
  echo ERROR: VLC esta abierto. Cierralo antes de instalar.
  exit /b 4
)
if exist "%VLC%\plugins\control\libxmeyezoom_plugin.dll" if not exist "%VLC%\plugins\control\libxmeyezoom_plugin.dll.previous.bak" (
  copy /Y "%VLC%\plugins\control\libxmeyezoom_plugin.dll" "%VLC%\plugins\control\libxmeyezoom_plugin.dll.previous.bak" >nul || exit /b 5
)
copy /Y "%~dp0plugins\control\libxmeyezoom_plugin.dll" "%VLC%\plugins\control\libxmeyezoom_plugin.dll" >nul || exit /b 6
if exist "%VLC%\vlc-cache-gen.exe" (
  "%VLC%\vlc-cache-gen.exe" "%VLC%\plugins"
  if errorlevel 1 exit /b 7
)
echo OK: XMEye Interactive Zoom instalado en %VLC%
echo Lanzalo con START-VLC-WITH-ZOOM.cmd
endlocal