@echo off
setlocal
rem [TEMP] Actual native GeneralSettings owner/defaults/script x86 fixture.
rem The link forces ONE unresolved external: HudInformationAdapter::setMapStatic, which only
rem apply's non-null HUD branch reaches (the fixture keeps hudManager null). Core\StringUtil.cpp is
rem linked since 2026-09-28: the published table pulls compareIgnoreCase in through the history and
rem played-help slots, and the fixture now runs the played-help pair.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\general_settings_20260907\runtime_general"
if not defined BF2_TEST_OPT set "BF2_TEST_OPT=/O2"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set TASK_FLAGS=/nologo %BF2_TEST_OPT% /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\Game\GeneralSettings.cpp" /Fo"%BF2_BUILD_OUT%\GeneralSettings.obj" >"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0general_settings_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\Core\StringUtil.cpp" /Fo"%BF2_BUILD_OUT%\StringUtil.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\general_settings_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" /MAP:"%BF2_BUILD_OUT%\fixture.map" "%BF2_BUILD_OUT%\GeneralSettings.obj" "%BF2_BUILD_OUT%\StringUtil.obj" "%BF2_BUILD_OUT%\fixture.obj" kernel32.lib >"%BF2_BUILD_OUT%\link.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\link.log" & exit /b 1)
findstr /C:"unresolved external symbol" "%BF2_BUILD_OUT%\link.log" >"%BF2_BUILD_OUT%\unresolved.log"
"%BF2_BUILD_OUT%\general_settings_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
echo forced unresolved externals:
type "%BF2_BUILD_OUT%\unresolved.log"
exit /b %TASK_RESULT%
