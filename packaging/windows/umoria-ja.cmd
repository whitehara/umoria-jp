@echo off
rem Launches Umoria in Japanese, preferring Windows Terminal for correct
rem display of double-width characters. Legacy consoles (the classic
rem Command Prompt window) have a known ncurses rendering bug that garbles
rem Japanese text; see README-windows.txt for details.
where wt.exe >nul 2>nul
if errorlevel 1 (
    "%~dp0umoria.exe" -l ja
) else (
    rem "%~dp0." (trailing dot), not "%~dp0": %~dp0 ends in a backslash, and
    rem a backslash immediately before a closing quote is parsed as an
    rem escaped quote, corrupting the rest of the command line.
    start "" wt.exe -d "%~dp0." "%~dp0umoria.exe" -l ja
)
