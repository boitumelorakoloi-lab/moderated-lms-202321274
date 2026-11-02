@echo off
rem Starts the LMS with demo accounts (password Password1) in the "demo" folder.
cd /d "%~dp0"
chcp 65001 >nul
if not exist demo mkdir demo
cd demo
..\lms.exe --seed-demo
pause
