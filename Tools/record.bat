@echo off
rem Records an episode with OBS (Tools/record.py): plays the takes of the plan and leaves one video plus marks. Argument: the plan,
rem e.g. Tools\record.bat "Ignore\Content\Episode 1\grabaciones.json" [--only R3,R8] [--dry-run]. The game watcher and OBS must be open.
title Artemis record
cd /d "%~dp0.."
if not exist "Agent\.venv\Scripts\python.exe" (
    echo There is no Agent\.venv: open Tools\game_watcher.bat once so it creates it.
    pause
    exit /b 1
)
"Agent\.venv\Scripts\python.exe" Tools\record.py %*
pause
