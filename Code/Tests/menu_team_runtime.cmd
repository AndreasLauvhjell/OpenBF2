@echo off
setlocal
rem [TEMP] Strict native x86 owner/parser/XML fixture; see cpp for collaborator seams.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\menu_team_20260909\runtime"
if not defined BF2_TEST_OPT set "BF2_TEST_OPT=/O2"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set TASK_FLAGS=/nologo %BF2_TEST_OPT% /EHsc /std:c++14 /permissive /MD /Z7 /Gy /Gw /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /D_CRT_SECURE_NO_WARNINGS /FI"%TASK_ROOT%\Core\BuildConfig.h" /I"%TASK_ROOT%\External\zlib-1.2.3" /I"%TASK_ROOT%\External\zlib-1.2.3\contrib\minizip"
set "TASK_OBJECTS="
type nul >"%BF2_BUILD_OUT%\compile.log"
for %%S in (Core\StringUtil External\TinyXML\tinyxml External\TinyXML\tinyxmlparser External\TinyXML\tinyxmlerror External\TinyXML\tinystr) do (
    call :compile "%%S"
    if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /c "%~dp0menu_team_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
link /nologo /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\menu_team_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" /MAP:"%BF2_BUILD_OUT%\fixture.map" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" kernel32.lib >"%BF2_BUILD_OUT%\link.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\link.log" & exit /b 1)
if defined BF2_MENU_DATA ("%BF2_BUILD_OUT%\menu_team_runtime.exe" "%BF2_MENU_DATA%" >"%BF2_BUILD_OUT%\runtime.log" 2>&1) else ("%BF2_BUILD_OUT%\menu_team_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1)
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
:compile
set "TASK_SOURCE=%~1"
set "TASK_NAME=%TASK_SOURCE:\=_%"
cl %TASK_FLAGS% /c "%TASK_ROOT%\%TASK_SOURCE%.cpp" /Fo"%BF2_BUILD_OUT%\%TASK_NAME%.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
set TASK_OBJECTS=%TASK_OBJECTS% "%BF2_BUILD_OUT%\%TASK_NAME%.obj"
exit /b 0
