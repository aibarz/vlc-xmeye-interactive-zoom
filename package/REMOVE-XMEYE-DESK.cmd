@echo off
setlocal EnableExtensions
set "VLC=%~1"
if "%VLC%"=="" set "VLC=C:\Program Files\VideoLAN\VLC"

if not exist "%VLC%\vlc.exe" exit /b 2

tasklist /FI "IMAGENAME eq vlc.exe" | find /I "vlc.exe" >nul
if not errorlevel 1 (
  echo ERROR: VLC esta abierto. Cierralo antes de retirar los plugins.
  exit /b 3
)

call :remove control libxmeyezoom_plugin.dll
call :remove control libxmeyecontrol_plugin.dll
call :remove video_filter libxmeyegate_plugin.dll
call :remove video_filter libxmeyedeblock_plugin.dll
call :remove video_filter libxmeyedeconv_plugin.dll
call :remove video_filter libxmeyegrayguard_plugin.dll
call :remove video_filter libxmeyefocus_plugin.dll

if exist "%VLC%\vlc-cache-gen.exe" "%VLC%\vlc-cache-gen.exe" "%VLC%\plugins"
echo OK: XMEye Desk retirado/restaurado.
exit /b 0

:remove
set "SUB=%~1"
set "DLL=%~2"
set "DST=%VLC%\plugins\%SUB%\%DLL%"
if exist "%DST%" del /Q "%DST%"
if exist "%DST%.previous.bak" move /Y "%DST%.previous.bak" "%DST%" >nul
exit /b 0
