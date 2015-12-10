@echo off 
set MODE=%1
set FILE=%2
set VOLUME=%3
set REPEATS=%4

if not "p" EQU "%MODE%" goto next
rem start "" /B "c:\program files\Windows Media Player\wmplayer" /play /close "%FILE%" 
start "" /B "c:\program files\SMPlayer\mplayer\mplayer" -loop "%REPEATS%" -volume "%VOLUME%" "%FILE%" 
:next

if not "s" EQU "%MODE%" goto next2
rem TaskKill.exe /IM WMPlayer.exe
TaskKill.exe /IM mplayer.exe
:next2

