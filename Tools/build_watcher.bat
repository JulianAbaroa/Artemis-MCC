@echo off
rem Starts the build watcher (Tools/build_watcher.ps1). Leave the window open while Claude works.
title Artemis build watcher
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_watcher.ps1"
pause