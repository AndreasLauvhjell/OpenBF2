@echo off
setlocal
rem [TEMP] Native x86 client font fixture; force link's unrelated symbols remain uncalled.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_font_runtime_20260905"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Od /fp:precise /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /I"%TASK_ROOT%\External\zlib-1.2.3" /I"%TASK_ROOT%\External\zlib-1.2.3\contrib\minizip"
for %%F in (Menu\Font Menu\FontLoader Menu\DifFontMetadata Menu\DifFontMetadataFile External\BFMeme\Style External\BFMeme\Quad External\BFMeme\FileIStream External\BFMeme\IStream External\BFMeme\Stream External\BFMeme\Object External\BFMeme\TypeDescriptor IO\File\File IO\File\FileManager IO\Stream\MemStream Core\StringUtil Core\ClassManager) do (
    cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%~nF.obj"
    if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0font_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\font_runtime.exe" /PDB:"%BF2_BUILD_OUT%\font_runtime.pdb" "%BF2_BUILD_OUT%\Font.obj" "%BF2_BUILD_OUT%\FontLoader.obj" "%BF2_BUILD_OUT%\DifFontMetadata.obj" "%BF2_BUILD_OUT%\DifFontMetadataFile.obj" "%BF2_BUILD_OUT%\Style.obj" "%BF2_BUILD_OUT%\Quad.obj" "%BF2_BUILD_OUT%\FileIStream.obj" "%BF2_BUILD_OUT%\IStream.obj" "%BF2_BUILD_OUT%\Stream.obj" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\File.obj" "%BF2_BUILD_OUT%\FileManager.obj" "%BF2_BUILD_OUT%\MemStream.obj" "%BF2_BUILD_OUT%\StringUtil.obj" "%BF2_BUILD_OUT%\ClassManager.obj" "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link_force.log" 2>&1
set "TASK_LINK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\link_force.log"
if not "%TASK_LINK_RESULT%"=="0" exit /b %TASK_LINK_RESULT%
"%BF2_BUILD_OUT%\font_runtime.exe" >"%BF2_BUILD_OUT%\runtime_force.log" 2>&1
set "TASK_RUNTIME_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime_force.log"
exit /b %TASK_RUNTIME_RESULT%
