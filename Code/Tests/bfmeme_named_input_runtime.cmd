@echo off
setlocal
rem [TEMP] Production named-input/cache bodies with typed byte providers.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_named_input_20260905"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0
for %%F in (Object TypeDescriptor Data Stream OStream IStream ClassOStream ClassIStream NamedInputStream System NamedObjectRegistry Node NodeState InputNodeVisitors EventQueue Pointer) do (
    cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\External\BFMeme\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%F.obj"
    if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0bfmeme_named_input_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\named_input_runtime.exe" /PDB:"%BF2_BUILD_OUT%\named_input_runtime.pdb" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\Data.obj" "%BF2_BUILD_OUT%\Stream.obj" "%BF2_BUILD_OUT%\OStream.obj" "%BF2_BUILD_OUT%\IStream.obj" "%BF2_BUILD_OUT%\ClassOStream.obj" "%BF2_BUILD_OUT%\ClassIStream.obj" "%BF2_BUILD_OUT%\NamedInputStream.obj" "%BF2_BUILD_OUT%\System.obj" "%BF2_BUILD_OUT%\NamedObjectRegistry.obj" "%BF2_BUILD_OUT%\Node.obj" "%BF2_BUILD_OUT%\NodeState.obj" "%BF2_BUILD_OUT%\InputNodeVisitors.obj" "%BF2_BUILD_OUT%\EventQueue.obj" "%BF2_BUILD_OUT%\Pointer.obj" "%BF2_BUILD_OUT%\fixture.obj" user32.lib >"%BF2_BUILD_OUT%\link_force.log" 2>&1
set "TASK_LINK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\link_force.log"
if not "%TASK_LINK_RESULT%"=="0" exit /b %TASK_LINK_RESULT%
"%BF2_BUILD_OUT%\named_input_runtime.exe" >"%BF2_BUILD_OUT%\runtime_force.log" 2>&1
set "TASK_RUNTIME_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime_force.log"
exit /b %TASK_RUNTIME_RESULT%
