@echo off
setlocal
rem [TEMP] Native x86 client prefix checks, with untouched suffix dependencies uncalled.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_bf2_split_node_runtime"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
if not defined BF2_FIXTURE_OPT set "BF2_FIXTURE_OPT=/Od"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD %BF2_FIXTURE_OPT% /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
for %%s in (Object TypeDescriptor Stream Node SplitNode) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\External\BFMeme\%%s.cpp" /Fo"%BF2_BUILD_OUT%\\"
  if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\Menu\Bf2SplitNode.cpp" /Fo"%BF2_BUILD_OUT%\Bf2SplitNode.obj"
if errorlevel 1 exit /b 1
cl %TASK_FLAGS% /I"%TASK_ROOT%\Code\BF2" /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0bf2_split_node_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE /INCREMENTAL:NO /DEBUG /OPT:REF /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\bf2_split_node_runtime.exe" /PDB:"%BF2_BUILD_OUT%\bf2_split_node_runtime.pdb" /MAP:"%BF2_BUILD_OUT%\bf2_split_node_runtime.map" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\Stream.obj" "%BF2_BUILD_OUT%\Node.obj" "%BF2_BUILD_OUT%\SplitNode.obj" "%BF2_BUILD_OUT%\Bf2SplitNode.obj" "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link.log" 2>&1
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\bf2_split_node_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
