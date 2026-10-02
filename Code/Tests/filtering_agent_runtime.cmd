@echo off
setlocal
rem [TEMP] Strict x86 link of only real classifier bodies and lifetime observer.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set TESTOUT=%~dp0..\Build\unresolved_wave1_filter_20260908\runtime
if not exist "%TESTOUT%" mkdir "%TESTOUT%"
cl /nologo /c /MDd /Od /Gy /std:c++14 /permissive /EHsc /Z7 /DDEDICATED=1 ^
   /FI"%~dp0..\BF2\Core\BuildConfig.h" /Fo"%TESTOUT%/" ^
   "%~dp0filtering_agent_runtime.cpp" "%~dp0..\BF2\IO\Network\PacketFilter.cpp" ^
   "%~dp0..\BF2\Core\Mutex.cpp" ^
   > "%TESTOUT%\compile.log" 2>&1
if errorlevel 1 ( type "%TESTOUT%\compile.log" & exit /b 1 )
link /nologo /DEBUG /OPT:REF /subsystem:console /OUT:"%TESTOUT%\filtering_agent_runtime.exe" ^
   /PDB:"%TESTOUT%\runtime.pdb" /MAP:"%TESTOUT%\runtime.map" ^
   "%TESTOUT%\filtering_agent_runtime.obj" "%TESTOUT%\PacketFilter.obj" "%TESTOUT%\Mutex.obj" ^
   > "%TESTOUT%\link.log" 2>&1
if errorlevel 1 ( type "%TESTOUT%\link.log" & exit /b 1 )
"%TESTOUT%\filtering_agent_runtime.exe"
exit /b %errorlevel%
