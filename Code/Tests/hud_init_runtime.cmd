@echo off
setlocal EnableDelayedExpansion
rem [TEMP] Actual init prefix/helpers; external font lookup intentionally stops.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hud_init_final_20260906\runtime_o2"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
if not defined BF2_TEST_OPT set "BF2_TEST_OPT=/O2"
set TASK_FLAGS=/nologo %BF2_TEST_OPT% /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /I"%TASK_ROOT%\External\zlib-1.2.3" /I"%TASK_ROOT%\External\zlib-1.2.3\contrib\minizip"
set "TASK_SOURCES=Menu\BF2HudManager Menu\BF2HudManagerInit Menu\Bf2HudBuilder Menu\BfListBoxData Menu\BfEditData Menu\HudBlinker Menu\HudInformationAdapter Menu\HudQuestionManager Menu\HudSquad Menu\HudCommander Menu\HudMessages Menu\HudScoreboard Menu\HudVoteMap Menu\HudColors Game\HudInformationLayer Core\EventManager Core\System Menu\Ref2System External\BFMeme\System External\BFMeme\Node External\BFMeme\NodeState External\BFMeme\InputNodeVisitors External\BFMeme\NamedInputStream External\BFMeme\ClassIStream External\BFMeme\OStream Menu\Font Menu\FontLoader Menu\DifFontMetadata Menu\DifFontMetadataFile External\BFMeme\Style External\BFMeme\Effect External\BFMeme\Quad External\BFMeme\FileIStream External\BFMeme\FileOStream External\BFMeme\ClassOStream External\BFMeme\IStream External\BFMeme\Stream External\BFMeme\Object External\BFMeme\Data External\BFMeme\TypeDescriptor External\BFMeme\NamedObjectRegistry External\BFMeme\EventQueue External\BFMeme\Pointer External\BFMeme\DynamicIndexPointer IO\File\File IO\File\FileManager IO\Stream\MemStream Core\StringUtil Core\ClassManager Core\Debug Math\Mat4 Math\Vec3"
type nul >"%BF2_BUILD_OUT%\compile.log"
if defined BF2_FIXTURE_ONLY goto compile_fixture
for %%F in (%TASK_SOURCES%) do (
  set "TASK_KEY=%%F"
  set "TASK_KEY=!TASK_KEY:\=_!"
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\%%F.cpp" /Fo"%BF2_BUILD_OUT%\!TASK_KEY!.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
)
:compile_fixture
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0hud_init_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
type nul >"%BF2_BUILD_OUT%\objects.rsp"
for %%F in (%TASK_SOURCES%) do (
  set "TASK_KEY=%%F"
  set "TASK_KEY=!TASK_KEY:\=_!"
  echo "%BF2_BUILD_OUT%\!TASK_KEY!.obj" >>"%BF2_BUILD_OUT%\objects.rsp"
)
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_init_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" /MAP:"%BF2_BUILD_OUT%\fixture.map" @"%BF2_BUILD_OUT%\objects.rsp" "%BF2_BUILD_OUT%\fixture.obj" user32.lib >"%BF2_BUILD_OUT%\link_force.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\link_force.log" & exit /b 1)
"%BF2_BUILD_OUT%\hud_init_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
