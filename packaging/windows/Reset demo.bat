@echo off
rem Deletes the demo database so the next demo starts fresh. School data is not touched.
cd /d "%~dp0"
if exist demo rmdir /s /q demo
echo Demo data deleted.
pause
