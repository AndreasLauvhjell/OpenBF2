@echo off
setlocal EnableDelayedExpansion
rem [TEMP] Real builder createMapNode + real Bf2MenuMapNode construction/destruction/event
rem face against a real System registry, information layer and EventManager. The renderer's
rem loadTexture is the only recorder. /FORCE:UNRESOLVED covers uncalled map/builder helpers.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hud_map_create_20260914\runtime_o2"
if not defined BF2_TEST_OPT set "BF2_TEST_OPT=/O2"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set TASK_FLAGS=/nologo %BF2_TEST_OPT% /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DPy_NO_ENABLE_SHARED /I"%TASK_ROOT%\External\python-embedded\py-src-2.3.4\Include" /I"%TASK_ROOT%\External\python-embedded\py-src-2.3.4\PC" /I"%TASK_ROOT%\External\zlib-1.2.3" /I"%TASK_ROOT%\External\zlib-1.2.3\contrib\minizip"
set "TASK_SOURCES=Menu\Bf2MenuMapNode Menu\Bf2MiniMenuMapNode Menu\BF2HudManager Menu\BF2HudManagerConfiguration Menu\BF2HudManagerState Menu\Bf2TransformNode Menu\Bf2SplitNode Menu\Bf2HudBuilder Menu\HudColors Game\HudInformationLayer Menu\HudBlinker Scene\Object\PlayerManager Game\Objects\Fx\PersistentExplosionEffectManager Core\EventManager Core\StringUtil Core\Debug Core\System Math\Vec3 External\BFMeme\Object External\BFMeme\TypeDescriptor External\BFMeme\Node External\BFMeme\TransformNode External\BFMeme\SplitNode External\BFMeme\System External\BFMeme\NamedObjectRegistry External\BFMeme\Stream External\BFMeme\Data External\BFMeme\Quad External\BFMeme\Effect"
type nul >"%BF2_BUILD_OUT%\compile.log"
type nul >"%BF2_BUILD_OUT%\objects.rsp"
for %%F in (%TASK_SOURCES%) do (
  set "TASK_KEY=%%F"
  set "TASK_KEY=!TASK_KEY:\=_!"
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\%%F.cpp" /Fo"%BF2_BUILD_OUT%\!TASK_KEY!.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
  echo "%BF2_BUILD_OUT%\!TASK_KEY!.obj" >>"%BF2_BUILD_OUT%\objects.rsp"
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0hud_map_private_access.cpp" /Fo"%BF2_BUILD_OUT%\private_access.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
echo "%BF2_BUILD_OUT%\private_access.obj" >>"%BF2_BUILD_OUT%\objects.rsp"
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0hud_map_create_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_map_create_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" /MAP:"%BF2_BUILD_OUT%\fixture.map" @"%BF2_BUILD_OUT%\objects.rsp" "%BF2_BUILD_OUT%\fixture.obj" kernel32.lib user32.lib >"%BF2_BUILD_OUT%\link.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\link.log" & exit /b 1)
findstr /C:"unresolved external symbol" "%BF2_BUILD_OUT%\link.log" >"%BF2_BUILD_OUT%\unresolved.log"
"%BF2_BUILD_OUT%\hud_map_create_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
if not "%TASK_RESULT%"=="0" exit /b %TASK_RESULT%
rem 2026-09-26 (D2): no death runs remain - the last two (mini-paint-death, mini-event-death) are
rem positive checks in the main suite now.
echo forced unresolved externals:
type "%BF2_BUILD_OUT%\unresolved.log" | find /c "unresolved"
exit /b 0
