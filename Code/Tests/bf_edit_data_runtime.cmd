@echo off
setlocal
rem [TEMP] Real data family and bounded node bodies. Uncalled virtuals remain /FORCE diagnostics.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\bf_edit_data_20260906\runtime"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /c /EHsc /std:c++14 /permissive /MD /Od /fp:precise /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 %BF_EDIT_EXTRA_FLAGS%"
type nul >"%BF2_BUILD_OUT%\compile.log"
for %%s in (Data Object TypeDescriptor Node System Stream OStream) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" "%TASK_ROOT%\External\BFMeme\%%s.cpp" /Fo"%BF2_BUILD_OUT%\%%s.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
)
for %%s in (BfEditData BfEditNode) do (
  cl %TASK_FLAGS% /d1reportSingleClassLayout%%s /FI"%TASK_ROOT%\Core\BuildConfig.h" "%TASK_ROOT%\Menu\%%s.cpp" /Fo"%BF2_BUILD_OUT%\%%s.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" "%~dp0bf_edit_data_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\bf_edit_data_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" "%BF2_BUILD_OUT%\Data.obj" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\Node.obj" "%BF2_BUILD_OUT%\System.obj" "%BF2_BUILD_OUT%\Stream.obj" "%BF2_BUILD_OUT%\OStream.obj" "%BF2_BUILD_OUT%\BfEditData.obj" "%BF2_BUILD_OUT%\BfEditNode.obj" "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link_force.log" 2>&1
set "TASK_LINK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\link_force.log"
if not "%TASK_LINK_RESULT%"=="0" exit /b %TASK_LINK_RESULT%
"%BF2_BUILD_OUT%\bf_edit_data_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RUNTIME_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RUNTIME_RESULT%
