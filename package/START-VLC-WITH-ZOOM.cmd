@echo off
setlocal
set "VLC=%~1"
if "%VLC%"=="" set "VLC=C:\Program Files\VideoLAN\VLC"

if not exist "%VLC%\vlc.exe" (
  echo ERROR: no se encuentra VLC en: %VLC%
  echo Uso: START-VLC-WITH-ZOOM.cmd "C:\ruta\a\VLC"
  exit /b 2
)

start "" "%VLC%\vlc.exe" --extraintf=xmeyezoom
endlocal
