@echo off
rem Renders BF2's dedicated-server status screen through the recovered StatusMonitorImpl interface.
rem   statusscreen.cmd          text backend, prints the composed grid (capturable)
rem   statusscreen.cmd --live   real Win32 console backend from StatusMonitorImpl_win32.cpp
rem   statusscreen.cmd --scrolltest   scroll first, then paint - the regression test for the
rem                                   buffer-vs-window origin bug (exit 1 on failure)
call "%~dp0..\vsdevcmd.cmd" x86
pushd "%~dp0"
cl /nologo /std:c++14 /permissive /EHsc /wd4996 /DDEDICATED=1 /FI"%~dp0..\..\BF2\Core\BuildConfig.h" /I"%~dp0..\..\BF2\Game\Main" ^
   statusscreen.cpp "%~dp0..\..\BF2\Game\Main\StatusMonitorImpl_win32.cpp" /Fe:statusscreen.exe
if errorlevel 1 ( popd & exit /b 1 )
"%~dp0statusscreen.exe" %1
set RC=%errorlevel%
popd
exit /b %RC%
