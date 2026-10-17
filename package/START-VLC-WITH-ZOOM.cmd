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
  echo ERROR: no se ha localizado VLC.
  echo Uso: START-VLC-WITH-ZOOM.cmd "C:\ruta\a\VLC"
  exit /b 2
)
if not exist "%VLC%\vlc.exe" (
  echo ERROR: no se encuentra VLC en: %VLC%
  exit /b 2
)
start "" "%VLC%\vlc.exe" --extraintf=xmeyezoom
endlocal