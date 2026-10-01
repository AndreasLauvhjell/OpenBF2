@echo off
setlocal
rem [TEMP] handleCommand's run/include keyword tokenisation against the stock quoted HUD script
rem spelling. Links the real OldConsole (IO\Console\Console) with run()/include() overridden by the
rem fixture to record their arguments; uncalled dependencies of that TU are /FORCE'd and listed.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\console_run_quotes_20260914\runtime_o2"
if not defined BF2_TEST_OPT set "BF2_TEST_OPT=/O2"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set TASK_FLAGS=/nologo %BF2_TEST_OPT% /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=1 /D_ITERATOR_DEBUG_LEVEL=0 /I"%TASK_ROOT%\External\zlib-1.2.3" /I"%TASK_ROOT%\External\zlib-1.2.3\contrib\minizip"
type nul >"%BF2_BUILD_OUT%\compile.log"
type nul >"%BF2_BUILD_OUT%\objects.rsp"
for %%F in (IO\Console\Console Core\StringUtil Core\Debug) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%~nF.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
  echo "%BF2_BUILD_OUT%\%%~nF.obj" >>"%BF2_BUILD_OUT%\objects.rsp"
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0console_run_quotes_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\console_run_quotes_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" /MAP:"%BF2_BUILD_OUT%\fixture.map" @"%BF2_BUILD_OUT%\objects.rsp" "%BF2_BUILD_OUT%\fixture.obj" kernel32.lib user32.lib >"%BF2_BUILD_OUT%\link.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\link.log" & exit /b 1)
findstr /C:"unresolved external symbol" "%BF2_BUILD_OUT%\link.log" >"%BF2_BUILD_OUT%\unresolved.log"
"%BF2_BUILD_OUT%\console_run_quotes_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
echo forced unresolved externals:
type "%BF2_BUILD_OUT%\unresolved.log"
exit /b %TASK_RESULT%
