@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem [TEMP] Native x86 /Od + /O2 fixture for BfNewListBoxNode::updateSelf.
rem /FORCE:UNRESOLVED covers uncalled BFMeme streaming/paint families only;
rem every dependency reached by updateSelf is linked and exercised normally.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_new_list_box_update_runtime_20260916"

call :build_and_run od /Od
if errorlevel 1 exit /b 1
call :build_and_run o2 /O2
if errorlevel 1 exit /b 1
echo HUD new list-box update passed at /Od and /O2
exit /b 0

:build_and_run
set "TASK_TAG=%~1"
set "TASK_OPT=%~2"
set "TASK_OUT=%BF2_BUILD_OUT%\%TASK_TAG%"
if not exist "%TASK_OUT%" mkdir "%TASK_OUT%"
set "TASK_FLAGS=/nologo /c /MD /std:c++14 /permissive /EHsc !TASK_OPT! /Z7 /Gy /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DDEDICATED=0"
set "TASK_OBJECTS="
for %%S in (Object TypeDescriptor Node System Data Action) do (
  cl !TASK_FLAGS! /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\External\BFMeme\%%S.cpp" /Fo"%TASK_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  set "TASK_OBJECTS=!TASK_OBJECTS! "%TASK_OUT%\%%S.obj""
)
for %%S in (BfListBoxData BfNewListBoxNode) do (
  cl !TASK_FLAGS! /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Menu\%%S.cpp" /Fo"%TASK_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  set "TASK_OBJECTS=!TASK_OBJECTS! "%TASK_OUT%\%%S.obj""
)
cl !TASK_FLAGS! /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%~dp0hud_new_list_box_update_runtime.cpp" /Fo"%TASK_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
set "TASK_OBJECTS=!TASK_OBJECTS! "%TASK_OUT%\fixture.obj""
rem Avoid Windows' legacy installer-name heuristic: an unmanifested executable
rem containing "update" can be refused with ERROR_ELEVATION_REQUIRED before main.
if exist "%TASK_OUT%\hud_new_list_box_fixture.exe" del /q "%TASK_OUT%\hud_new_list_box_fixture.exe"
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%TASK_OUT%\hud_new_list_box_fixture.exe" /PDB:"%TASK_OUT%\hud_new_list_box_fixture.pdb" /MAP:"%TASK_OUT%\hud_new_list_box_fixture.map" !TASK_OBJECTS! >"%TASK_OUT%\link.log" 2>&1
if not exist "%TASK_OUT%\hud_new_list_box_fixture.exe" (
  type "%TASK_OUT%\link.log"
  exit /b 1
)
"%TASK_OUT%\hud_new_list_box_fixture.exe" >"%TASK_OUT%\runtime.log" 2>&1
set "TASK_RESULT=!errorlevel!"
type "%TASK_OUT%\runtime.log"
if not "!TASK_RESULT!"=="0" exit /b !TASK_RESULT!

"%TASK_OUT%\hud_new_list_box_fixture.exe" null-manager >"%TASK_OUT%\null-manager.log" 2>&1
set "TASK_NULL_RESULT=!errorlevel!"
if not "!TASK_NULL_RESULT!"=="-1073741819" (
  echo !TASK_TAG!: null input manager returned unexpected code !TASK_NULL_RESULT!, expected C0000005
  type "%TASK_OUT%\null-manager.log"
  exit /b 1
)
findstr /c:"FIXTURE_NULL_MANAGER_RETURNED" "%TASK_OUT%\null-manager.log" >nul
if not errorlevel 1 (
  echo !TASK_TAG!: null input manager returned past retail's missing guard
  exit /b 1
)
echo !TASK_TAG!: null input-manager stop confirmed
exit /b 0
