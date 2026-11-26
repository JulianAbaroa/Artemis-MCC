@echo off
rem Starts a long training run by hand (Tools/start_training.py): queues the scenario, waits for the match and opens the training
rem and hw monitor windows. No arguments = the night run (hitoa_noche, 2500 updates, from scratch). The game watcher must be open.
title Artemis start training
cd /d "%~dp0.."
if not exist "Agent\.venv\Scripts\python.exe" (
    echo There is no Agent\.venv: open Tools\game_watcher.bat once so it creates it.
    pause
    exit /b 1
)
"Agent\.venv\Scripts\python.exe" Tools\start_training.py %*
pause
