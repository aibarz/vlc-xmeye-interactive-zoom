@echo off
setlocal EnableExtensions
set "VLC=%~1"
if "%VLC%"=="" set "VLC=C:\Program Files\VideoLAN\VLC"

if not exist "%VLC%\vlc.exe" (
  echo ERROR: no se encuentra VLC en: %VLC%
  echo Uso: INSTALL-XMEYE-DESK.cmd "C:\ruta\a\VLC"
  exit /b 2
)
if not exist "%VLC%\plugins\control" exit /b 3
if not exist "%VLC%\plugins\video_filter" exit /b 3

tasklist /FI "IMAGENAME eq vlc.exe" | find /I "vlc.exe" >nul
if not errorlevel 1 (
  echo ERROR: VLC esta abierto. Cierralo antes de instalar.
  exit /b 4
)

call :install control libxmeyezoom_plugin.dll || exit /b 5
call :install control libxmeyecontrol_plugin.dll || exit /b 5
call :install video_filter libxmeyegate_plugin.dll || exit /b 5
call :install video_filter libxmeyedeblock_plugin.dll || exit /b 5
call :install video_filter libxmeyedeconv_plugin.dll || exit /b 5
call :install video_filter libxmeyegrayguard_plugin.dll || exit /b 5
call :install video_filter libxmeyefocus_plugin.dll || exit /b 5

if exist "%VLC%\vlc-cache-gen.exe" (
  "%VLC%\vlc-cache-gen.exe" "%VLC%\plugins"
  if errorlevel 1 exit /b 7
)

echo.
echo OK: XMEye Desk instalado en %VLC%
echo Usa START-VLC-XMEYE-DESK.cmd para abrir Zoom + XMEye Control.
exit /b 0

:install
set "SUB=%~1"
set "DLL=%~2"
set "SRC=%~dp0plugins\%SUB%\%DLL%"
set "DST=%VLC%\plugins\%SUB%\%DLL%"
if not exist "%SRC%" (
  echo ERROR: falta %SRC%
  exit /b 1
)
if exist "%DST%" if not exist "%DST%.previous.bak" copy /Y "%DST%" "%DST%.previous.bak" >nul
copy /Y "%SRC%" "%DST%" >nul || exit /b 1
exit /b 0
