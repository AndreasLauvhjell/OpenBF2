@echo off
setlocal
rem [TEMP] Bounded BFMeme codec/lifetime fixture; unexercised deep frontiers remain explicit.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_bfmeme_stream_runtime_20260905"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
for %%F in (Object TypeDescriptor Data Stream OStream IStream FileIStream NamedObjectRegistry) do (
    cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\External\BFMeme\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%F.obj"
    if errorlevel 1 exit /b 1
)
if /i "%~1"=="compile" exit /b 0
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0bfmeme_stream_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\streams.exe" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\Data.obj" "%BF2_BUILD_OUT%\Stream.obj" "%BF2_BUILD_OUT%\OStream.obj" "%BF2_BUILD_OUT%\IStream.obj" "%BF2_BUILD_OUT%\NamedObjectRegistry.obj" "%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\streams.exe"
exit /b %errorlevel%
