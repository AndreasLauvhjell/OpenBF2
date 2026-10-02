@echo off
setlocal
rem [TEMP] Actual NodeState/System/queue path; uncalled family frontiers use /FORCE.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_node_state_runtime_20260905"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 %BF2_TEST_OPTFLAGS%"
for %%F in (Object TypeDescriptor Stream OStream System Node Pointer EventQueue InputNodeVisitors NodeState) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\External\BFMeme\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%F.obj"
  if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0node_state_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\node_state_runtime.exe" /PDB:"%BF2_BUILD_OUT%\node_state_runtime.pdb" "%BF2_BUILD_OUT%\fixture.obj" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\Stream.obj" "%BF2_BUILD_OUT%\OStream.obj" "%BF2_BUILD_OUT%\System.obj" "%BF2_BUILD_OUT%\Node.obj" "%BF2_BUILD_OUT%\Pointer.obj" "%BF2_BUILD_OUT%\EventQueue.obj" "%BF2_BUILD_OUT%\InputNodeVisitors.obj" "%BF2_BUILD_OUT%\NodeState.obj"
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\node_state_runtime.exe"
exit /b %errorlevel%
