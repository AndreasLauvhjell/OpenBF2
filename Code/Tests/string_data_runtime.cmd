@echo off
setlocal
rem [TEMP] Native client StringData fixture. Uncalled Stream dependencies remain diagnostic.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_string_data_20260906"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /c /MD /std:c++14 /permissive /EHsc /Od /Z7 /Gy /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DDEDICATED=0 %BF2_RUNTIME_OPT%"
for %%s in (Data Object TypeDescriptor Stream) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\BF2\Core\BuildConfig.h" /Fa"%BF2_BUILD_OUT%\%%s.asm" /d1reportSingleClassLayoutStringData "%TASK_ROOT%\BF2\External\BFMeme\%%s.cpp" /Fo"%BF2_BUILD_OUT%\\" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 (
    type "%BF2_BUILD_OUT%\compile.log"
    exit /b 1
  )
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\BF2\Core\BuildConfig.h" /Fa"%BF2_BUILD_OUT%\fixture.asm" "%~dp0string_data_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\fixture_compile.log" 2>&1
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\fixture_compile.log"
  exit /b 1
)
link /nologo /FORCE /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\string_data_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" /MAP:"%BF2_BUILD_OUT%\fixture.map" "%BF2_BUILD_OUT%\Data.obj" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\Stream.obj" "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link_force.log" 2>&1
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\link_force.log"
  exit /b 1
)
"%BF2_BUILD_OUT%\string_data_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
