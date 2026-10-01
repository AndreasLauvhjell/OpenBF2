@echo off
setlocal
rem [TEMP] Roads step 2 fixture (2026-10-01): the real RoadTemplate, RoadEditableRenderer, PerlinNoise,
rem EditorSpline and SplineControlPoint translation units linked against recorded host seams. No game,
rem editor or DLL is launched. BF2_TEST_ABI_NATIVE=0 builds the retail-compatible flavour, 1 the native.
rem Roads step 3 (2026-10-01): RoadEditable (and the native flavour's HeightmapEventNode) join the
rem list; road_objects_runtime.cpp models the seams it adds.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "ROOT=%~dp0..\BF2"
set "ABI=%~dp0..\RendDX9Abi"
if not defined BF2_TEST_ABI_NATIVE set "BF2_TEST_ABI_NATIVE=0"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\roads_rd1_20260930\fixture_%BF2_TEST_ABI_NATIVE%"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "FLAGS=/nologo /c /MD /std:c++14 /permissive /EHsc /O2 /W4 /wd4100 /wd4127 /DBF2_IMAGE_RENDDX9 /DBF2_PLUGIN_ABI_NATIVE=%BF2_TEST_ABI_NATIVE% /D_ITERATOR_DEBUG_LEVEL=0 /DDEDICATED=0 /FI%ROOT%\Core\BuildConfig.h /DWIN32 /D_WINDOWS /D_CRT_SECURE_NO_WARNINGS"
type nul >"%BF2_BUILD_OUT%\build.log"
set "OBJS="
set SOURCES="%~dp0road_objects_runtime.cpp" "%ROOT%\Geom\RoadTemplate.cpp" "%ROOT%\Geom\RoadEditable.cpp" "%ROOT%\Geom\RoadEditableRenderer.cpp" "%ROOT%\GeomMisc\PerlinNoise.cpp" "%ROOT%\GeomMisc\SplineControlPoint.cpp" "%ROOT%\GeomMisc\EditorSpline.cpp" "%ROOT%\Math\Mat4.cpp" "%ROOT%\Math\Mat4Inverse.cpp" "%ROOT%\Math\Vec3.cpp" "%ABI%\Vc71String.cpp"
if "%BF2_TEST_ABI_NATIVE%"=="1" set SOURCES=%SOURCES% "%ROOT%\Geom\HeightmapEventNode.cpp"
for %%F in (%SOURCES%) do (
    cl %FLAGS% %%F /Fo"%BF2_BUILD_OUT%\%%~nF.obj" >>"%BF2_BUILD_OUT%\build.log" 2>&1
    if errorlevel 1 goto failed
)
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\road_objects_runtime.exe" "%BF2_BUILD_OUT%\*.obj" >>"%BF2_BUILD_OUT%\build.log" 2>&1
if errorlevel 1 goto failed
"%BF2_BUILD_OUT%\road_objects_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %RESULT%
:failed
type "%BF2_BUILD_OUT%\build.log"
exit /b 1
