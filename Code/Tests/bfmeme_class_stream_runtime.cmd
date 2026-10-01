@echo off
setlocal
rem [TEMP] Typed file/class transport and object reader fixture; no replacement production bodies.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_bfmeme_class_stream_runtime_20260905"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /I"%TASK_ROOT%\External\zlib-1.2.3" /I"%TASK_ROOT%\External\zlib-1.2.3\contrib\minizip"
for %%F in (External\BFMeme\Object External\BFMeme\TypeDescriptor External\BFMeme\Data External\BFMeme\Stream External\BFMeme\OStream External\BFMeme\IStream External\BFMeme\ClassIStream External\BFMeme\ClassOStream External\BFMeme\FileOStream External\BFMeme\System External\BFMeme\NamedObjectRegistry IO\File\FileManager Core\StringUtil Core\Debug) do (
    cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%~nF.obj"
    if errorlevel 1 exit /b 1
)
if /i "%~1"=="compile" exit /b 0
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0bfmeme_class_stream_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\class_stream_runtime.exe" /PDB:"%BF2_BUILD_OUT%\class_stream_runtime.pdb" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\Data.obj" "%BF2_BUILD_OUT%\Stream.obj" "%BF2_BUILD_OUT%\OStream.obj" "%BF2_BUILD_OUT%\IStream.obj" "%BF2_BUILD_OUT%\ClassIStream.obj" "%BF2_BUILD_OUT%\ClassOStream.obj" "%BF2_BUILD_OUT%\FileOStream.obj" "%BF2_BUILD_OUT%\System.obj" "%BF2_BUILD_OUT%\NamedObjectRegistry.obj" "%BF2_BUILD_OUT%\FileManager.obj" "%BF2_BUILD_OUT%\StringUtil.obj" "%BF2_BUILD_OUT%\Debug.obj" "%BF2_BUILD_OUT%\fixture.obj" user32.lib >"%BF2_BUILD_OUT%\link_force.log" 2>&1
set "TASK_LINK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\link_force.log"
if not "%TASK_LINK_RESULT%"=="0" exit /b %TASK_LINK_RESULT%
"%BF2_BUILD_OUT%\class_stream_runtime.exe" >"%BF2_BUILD_OUT%\runtime_force.log" 2>&1
set "TASK_RUNTIME_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime_force.log"
exit /b %TASK_RUNTIME_RESULT%
