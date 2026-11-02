@echo off
rem Starts the LMS with the school database (lms.db) in this folder.
cd /d "%~dp0"
chcp 65001 >nul
lms.exe
pause
