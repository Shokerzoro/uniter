@echo off
cd /d %~dp0
setlocal enabledelayedexpansion

REM Папка с файлами для конвертации
set SRC_DIR=.\nsis

for /r "%SRC_DIR%" %%f in (*.nsi *.txt) do (
    echo Converting %%f ...
    powershell -Command "Get-Content -Encoding UTF8 '%%f' | Set-Content -Encoding Default '%%f.tmp'"
    move /Y "%%f.tmp" "%%f" >nul
)

REM Запускаем makensis из той же папки
"D:\QT\Tools\NSIS\Bin\makensis.exe" "nsis\installer.nsi"
