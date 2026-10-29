@echo off
chcp 65001 >nul
rem PMX(と VMD)をこのファイルへドラッグ＆ドロップすると、姿勢の画像を作って開きます。
setlocal
set "SCRIPT_DIR=%~dp0"

where py >nul 2>nul
if errorlevel 1 (set "PYTHON=python") else (set "PYTHON=py")

if "%~1"=="" (
    echo 使い方: .pmx を、必要なら .vmd も一緒に、このバッチへドラッグ＆ドロップしてください。
    echo         正面と横から見た画像を作って開きます。
    echo.
    pause
    exit /b 2
)

set "MODEL="
set "MOTION="
for %%F in (%*) do (
    if /i "%%~xF"==".pmx" set "MODEL=%%~fF"
    if /i "%%~xF"==".vmd" set "MOTION=%%~fF"
)
if not defined MODEL (
    echo .pmx ファイルが見つかりません。.pmx をドラッグ＆ドロップしてください。
    pause
    exit /b 2
)

set "OUTPUT=%MODEL:~0,-4%_preview.png"
set "FRAME=0"
if defined MOTION set /p "FRAME=何フレーム目の姿勢を描きますか(何も入れずに Enter なら 0): "

if defined MOTION (
    "%PYTHON%" "%SCRIPT_DIR%pmxpreview.py" "%MODEL%" "%MOTION%" --frame %FRAME% -o "%OUTPUT%"
) else (
    "%PYTHON%" "%SCRIPT_DIR%pmxpreview.py" "%MODEL%" -o "%OUTPUT%"
)
if errorlevel 1 (
    echo.
    echo 画像を作れませんでした。上のメッセージを確認してください。
    pause
    exit /b 1
)
if not defined PMXPREVIEW_NO_OPEN start "" "%OUTPUT%"
exit /b 0
