@echo off
rem Starts the training lists builder (Agent/artemis/web, LS.3) with the python of the venv and opens the page in the browser.
rem It listens on 127.0.0.1 only. Close the window (or Ctrl+C) to stop it. Extra arguments go to the server (--port N, --no-browser).
title Artemis lists builder
cd /d "%~dp0..\Agent"
if not exist ".venv\Scripts\python.exe" (
    echo There is no Agent\.venv: open Tools\game_watcher.bat once so it creates it.
    pause
    exit /b 1
)
".venv\Scripts\python.exe" -m artemis.web %*
pause