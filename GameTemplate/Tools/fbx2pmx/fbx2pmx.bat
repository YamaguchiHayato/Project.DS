@echo off
chcp 65001 >nul
rem FBX を PMX へ変換します。.fbx をこのファイルへドラッグ＆ドロップしても使えます。
setlocal
set "SCRIPT_DIR=%~dp0"

where py >nul 2>nul
if errorlevel 1 (set "PYTHON=python") else (set "PYTHON=py")

if "%~1"=="" (
    echo 使い方: fbx2pmx.bat model.fbx [オプション]
    echo         .fbx をこのバッチへドラッグ ^& ドロップしても変換できます。
    echo.
    "%PYTHON%" "%SCRIPT_DIR%fbx2pmx.py" --help
    pause
    exit /b 2
)

"%PYTHON%" "%SCRIPT_DIR%fbx2pmx.py" %*
set "RESULT=%ERRORLEVEL%"
echo.
pause
exit /b %RESULT%
