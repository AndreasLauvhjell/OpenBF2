@echo off
setlocal
rem [TEMP] Actual native ControlSettings owner/lifetime/backup/restore/capture/forwarding x86 fixture.
rem The control-map manager, the device manager, the mouse and the frame event manager are typed
rem observers; the maps derive from the tree's real io::ControlMap. Strict link by default.
rem Pass `console` to also link the real console (setDefaults runs Settings/Controls.con through it);
rem only that mode permits uncalled /FORCE dependencies, recorded in unresolved.log.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
set "TASK_MODE=strict"
set "TASK_MODE_FLAGS="
set "TASK_EXTERNAL_TUS="
set "TASK_LINK_MODE="
if /i "%~1"=="console" (
  set "TASK_MODE=console"
  set "TASK_MODE_FLAGS=/DBF2_CONTROL_CONSOLE_FIXTURE=1"
  set "TASK_EXTERNAL_TUS=IO\Console\Console IO\File\FileManager"
  set "TASK_LINK_MODE=/FORCE:UNRESOLVED"
)
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\control_settings_20260908\runtime_%TASK_MODE%"
if not defined BF2_TEST_OPT set "BF2_TEST_OPT=/O2"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set TASK_FLAGS=/nologo %BF2_TEST_OPT% /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 %TASK_MODE_FLAGS% /I"%TASK_ROOT%\External\zlib-1.2.3" /I"%TASK_ROOT%\External\zlib-1.2.3\contrib\minizip"
type nul >"%BF2_BUILD_OUT%\compile.log"
type nul >"%BF2_BUILD_OUT%\objects.rsp"
for %%F in (Game\ControlSettings IO\Input\ControlMap IO\Input\InputEnums IO\Input\InputMapStream Core\ClassManager Core\EventManager Core\Debug Core\StringUtil %TASK_EXTERNAL_TUS%) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%~nF.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
  echo "%BF2_BUILD_OUT%\%%~nF.obj" >>"%BF2_BUILD_OUT%\objects.rsp"
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0control_settings_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
link /nologo %TASK_LINK_MODE% /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\control_settings_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" /MAP:"%BF2_BUILD_OUT%\fixture.map" @"%BF2_BUILD_OUT%\objects.rsp" "%BF2_BUILD_OUT%\fixture.obj" kernel32.lib user32.lib >"%BF2_BUILD_OUT%\link.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\link.log" & exit /b 1)
findstr /C:"unresolved external symbol" "%BF2_BUILD_OUT%\link.log" >"%BF2_BUILD_OUT%\unresolved.log"
"%BF2_BUILD_OUT%\control_settings_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
if /i "%TASK_MODE%"=="console" (
  echo forced unresolved externals:
  type "%BF2_BUILD_OUT%\unresolved.log"
)
exit /b %TASK_RESULT%
