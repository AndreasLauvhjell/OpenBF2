@echo off
setlocal
rem [TEMP] Real collection/small-family regression; /FORCE only for uncalled Node/Tree/stream metadata and external providers.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_stream_descriptor_runtime_20260905"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /I"%TASK_ROOT%\External\zlib-1.2.3" /I"%TASK_ROOT%\External\zlib-1.2.3\contrib\minizip" %BF2_TEST_OPTFLAGS%
for %%F in (External\BFMeme\Object External\BFMeme\TypeDescriptor External\BFMeme\Data External\BFMeme\Stream External\BFMeme\OStream External\BFMeme\IStream External\BFMeme\ClassIStream External\BFMeme\ClassOStream External\BFMeme\FileOStream External\BFMeme\System External\BFMeme\NamedObjectRegistry External\BFMeme\ObjectCollection External\BFMeme\Event External\BFMeme\Action External\BFMeme\Function IO\File\FileManager Core\StringUtil Core\Debug) do (
    cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%~nF.obj"
    if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /d1reportAllClassLayout /c "%~dp0stream_descriptor_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\layout.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\layout.log" & exit /b 1)
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\stream_descriptor_runtime.exe" /PDB:"%BF2_BUILD_OUT%\stream_descriptor_runtime.pdb" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\Data.obj" "%BF2_BUILD_OUT%\Stream.obj" "%BF2_BUILD_OUT%\OStream.obj" "%BF2_BUILD_OUT%\IStream.obj" "%BF2_BUILD_OUT%\ClassIStream.obj" "%BF2_BUILD_OUT%\ClassOStream.obj" "%BF2_BUILD_OUT%\FileOStream.obj" "%BF2_BUILD_OUT%\System.obj" "%BF2_BUILD_OUT%\NamedObjectRegistry.obj" "%BF2_BUILD_OUT%\ObjectCollection.obj" "%BF2_BUILD_OUT%\Event.obj" "%BF2_BUILD_OUT%\Action.obj" "%BF2_BUILD_OUT%\Function.obj" "%BF2_BUILD_OUT%\FileManager.obj" "%BF2_BUILD_OUT%\StringUtil.obj" "%BF2_BUILD_OUT%\Debug.obj" "%BF2_BUILD_OUT%\fixture.obj" user32.lib >"%BF2_BUILD_OUT%\link_force.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\link_force.log" & exit /b 1)
"%BF2_BUILD_OUT%\stream_descriptor_runtime.exe" >"%BF2_BUILD_OUT%\runtime_force.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime_force.log"
exit /b %TASK_RESULT%
