@echo off
setlocal
rem [TEMP] Real question storage; larger display/reply collaborators remain uncalled.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_questions_20260906"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /O2 /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /D_ITERATOR_DEBUG_LEVEL=0 %HUD_QUESTIONS_EXTRA_FLAGS%"
for %%F in (Menu\HudQuestionManager Core\System) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%~nF.obj"
  if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0hud_questions_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_questions_runtime.exe" /PDB:"%BF2_BUILD_OUT%\hud_questions_runtime.pdb" "%BF2_BUILD_OUT%\HudQuestionManager.obj" "%BF2_BUILD_OUT%\System.obj" "%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_questions_runtime.exe"
exit /b %errorlevel%
