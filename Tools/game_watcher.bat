@echo off
rem Starts the game watcher (Tools/game_watcher.ps1). Leave the window open while Claude works with the game.
title Artemis game watcher
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0game_watcher.ps1"
pause