@echo off
setlocal
rem [TEMP] [CHOSEN, not recovered] Diagnostic tool; no engine code is compiled here.
call "%~dp0vsdevcmd.cmd" x86
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0obj_client_startup_trace"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
cl /nologo /EHsc /W4 /Zi "%~dp0client_startup_trace.cpp" /Fo"%BF2_BUILD_OUT%\client_startup_trace.obj" /Fd"%BF2_BUILD_OUT%\client_startup_trace.pdb" /Fe"%BF2_BUILD_OUT%\client_startup_trace.exe" /link user32.lib
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\client_startup_trace.exe" %*
exit /b %errorlevel%
