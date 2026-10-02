@echo off
setlocal
rem [TEMP] Strict x86 exact binding block + whole actual console/manager/template TUs.
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_KIT_CONSOLE_OUT set "BF2_KIT_CONSOLE_OUT=%TASK_CODE%\Build\ai_kit_20260927\console\validation"
if not exist "%BF2_KIT_CONSOLE_OUT%" mkdir "%BF2_KIT_CONSOLE_OUT%"
python -I "%~dp0kit_console_runtime.py" generate "%BF2_KIT_CONSOLE_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1"
type nul >"%BF2_KIT_CONSOLE_OUT%\compile.log"
type nul >"%BF2_KIT_CONSOLE_OUT%\objects.rsp"
rem 2026-09-28: the extracted block now also holds the AI template bindings, so their whole TUs link too.
for %%F in (Core\ConsoleObjectBase Core\ConsoleObjects Core\StringUtil Core\Debug Math\Vec3 Math\Mat4 IO\Input\InputMapStream AI\AIInterface\BFKnowledge\KitTemplate AI\AIInterface\BFKnowledge\KitTemplateManager AI\AIInterface\BFKnowledge\InformationType AI\AIInterface\BFKnowledge\AITemplate AI\AIInterface\BFKnowledge\AITemplateManager AI\AIInterface\BFKnowledge\AITemplatePlugInManager AI\AIInterface\BFKnowledge\AITemplatePlugIn AI\AIInterface\BFKnowledge\AITemplatePhysical AI\AIInterface\BFKnowledge\AITemplateArmament AI\AIInterface\BFKnowledge\AITemplateCover AI\AIInterface\BFKnowledge\AITemplateMobile AI\AIInterface\BFKnowledge\AITemplateUnit AI\AIInterface\BFKnowledge\AITemplateControlInfo AI\AIInterface\BFKnowledge\SoldierPose AI\AIInterface\BFKnowledge\WeaponTemplate AI\AIInterface\BFKnowledge\WeaponTemplateManager) do (
  cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\%%F.cpp" /Fo"%BF2_KIT_CONSOLE_OUT%\%%~nF.obj" >>"%BF2_KIT_CONSOLE_OUT%\compile.log" 2>&1
  if errorlevel 1 goto failed
  echo "%BF2_KIT_CONSOLE_OUT%\%%~nF.obj" >>"%BF2_KIT_CONSOLE_OUT%\objects.rsp"
)
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_KIT_CONSOLE_OUT%\kit_console_actual.cpp" /Fo"%BF2_KIT_CONSOLE_OUT%\actual.obj" >>"%BF2_KIT_CONSOLE_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%BF2_KIT_CONSOLE_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0kit_console_runtime.cpp" /Fo"%BF2_KIT_CONSOLE_OUT%\fixture.obj" >>"%BF2_KIT_CONSOLE_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_KIT_CONSOLE_OUT%\kit_console_runtime.exe" @"%BF2_KIT_CONSOLE_OUT%\objects.rsp" "%BF2_KIT_CONSOLE_OUT%\actual.obj" "%BF2_KIT_CONSOLE_OUT%\fixture.obj" >"%BF2_KIT_CONSOLE_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0kit_console_runtime.py" run "%BF2_KIT_CONSOLE_OUT%\kit_console_runtime.exe" >"%BF2_KIT_CONSOLE_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_KIT_CONSOLE_OUT%\runtime.log"
exit /b %TASK_RESULT%
:failed
type "%BF2_KIT_CONSOLE_OUT%\compile.log"
if exist "%BF2_KIT_CONSOLE_OUT%\link.log" type "%BF2_KIT_CONSOLE_OUT%\link.log"
exit /b 1
