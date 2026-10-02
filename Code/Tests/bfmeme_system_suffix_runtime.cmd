@echo off
setlocal
rem [TEMP] Real base System suffix/size/paint; surrounding UI frontiers uncalled.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_bfmeme_system_suffix_runtime"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
for %%s in (Object TypeDescriptor Stream Data Node Pointer System EventQueue NamedObjectRegistry) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\External\BFMeme\%%s.cpp" /Fo"%BF2_BUILD_OUT%\\"
  if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /d1reportSingleClassLayoutSystem /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0bfmeme_system_suffix_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE /INCREMENTAL:NO /DEBUG /OPT:REF /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\bfmeme_system_suffix_runtime.exe" /PDB:"%BF2_BUILD_OUT%\bfmeme_system_suffix_runtime.pdb" /MAP:"%BF2_BUILD_OUT%\bfmeme_system_suffix_runtime.map" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\Stream.obj" "%BF2_BUILD_OUT%\Data.obj" "%BF2_BUILD_OUT%\Node.obj" "%BF2_BUILD_OUT%\Pointer.obj" "%BF2_BUILD_OUT%\System.obj" "%BF2_BUILD_OUT%\EventQueue.obj" "%BF2_BUILD_OUT%\NamedObjectRegistry.obj" "%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\bfmeme_system_suffix_runtime.exe"
exit /b %errorlevel%
