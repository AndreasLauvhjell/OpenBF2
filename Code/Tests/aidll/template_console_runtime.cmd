@echo off
setlocal
rem [TEMP] Strict x86: the exact BFConsole binding block + whole console/template/plug-in/parser TUs (2026-09-28).
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_TEMPLATE_CONSOLE_OUT set "BF2_TEMPLATE_CONSOLE_OUT=%TASK_CODE%\Build\claude_ai_plan_20260928\template_console"
if not exist "%BF2_TEMPLATE_CONSOLE_OUT%" mkdir "%BF2_TEMPLATE_CONSOLE_OUT%"
python -I "%~dp0template_console_runtime.py" generate "%BF2_TEMPLATE_CONSOLE_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1"
set "TASK_FI=/FI"%TASK_CODE%\BF2\Core\BuildConfig.h""
type nul >"%BF2_TEMPLATE_CONSOLE_OUT%\compile.log"
type nul >"%BF2_TEMPLATE_CONSOLE_OUT%\objects.rsp"
for %%F in (Core\ConsoleObjectBase Core\ConsoleObjects Core\StringUtil Core\Debug Math\Vec3 Math\Mat4 IO\Input\InputMapStream AI\AIInterface\BFKnowledge\KitTemplate AI\AIInterface\BFKnowledge\KitTemplateManager AI\AIInterface\BFKnowledge\InformationType AI\AIInterface\BFKnowledge\AITemplate AI\AIInterface\BFKnowledge\AITemplateManager AI\AIInterface\BFKnowledge\AITemplatePlugInManager AI\AIInterface\BFKnowledge\AITemplatePlugIn AI\AIInterface\BFKnowledge\AITemplatePhysical AI\AIInterface\BFKnowledge\AITemplateArmament AI\AIInterface\BFKnowledge\AITemplateCover AI\AIInterface\BFKnowledge\AITemplateMobile AI\AIInterface\BFKnowledge\AITemplateUnit AI\AIInterface\BFKnowledge\AITemplateControlInfo AI\AIInterface\BFKnowledge\SoldierPose AI\AIInterface\BFKnowledge\WeaponTemplate AI\AIInterface\BFKnowledge\WeaponTemplateManager) do (
  cl %TASK_FLAGS% %TASK_FI% "%TASK_CODE%\BF2\%%F.cpp" /Fo"%BF2_TEMPLATE_CONSOLE_OUT%\%%~nF.obj" >>"%BF2_TEMPLATE_CONSOLE_OUT%\compile.log" 2>&1
  if errorlevel 1 goto failed
  echo "%BF2_TEMPLATE_CONSOLE_OUT%\%%~nF.obj" >>"%BF2_TEMPLATE_CONSOLE_OUT%\objects.rsp"
)
cl %TASK_FLAGS% %TASK_FI% "%BF2_TEMPLATE_CONSOLE_OUT%\template_console_actual.cpp" /Fo"%BF2_TEMPLATE_CONSOLE_OUT%\actual.obj" >>"%BF2_TEMPLATE_CONSOLE_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%BF2_TEMPLATE_CONSOLE_OUT%" %TASK_FI% "%~dp0template_console_runtime.cpp" /Fo"%BF2_TEMPLATE_CONSOLE_OUT%\fixture.obj" >>"%BF2_TEMPLATE_CONSOLE_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_TEMPLATE_CONSOLE_OUT%\template_console_runtime.exe" @"%BF2_TEMPLATE_CONSOLE_OUT%\objects.rsp" "%BF2_TEMPLATE_CONSOLE_OUT%\actual.obj" "%BF2_TEMPLATE_CONSOLE_OUT%\fixture.obj" >"%BF2_TEMPLATE_CONSOLE_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0template_console_runtime.py" run "%BF2_TEMPLATE_CONSOLE_OUT%\template_console_runtime.exe" >"%BF2_TEMPLATE_CONSOLE_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_TEMPLATE_CONSOLE_OUT%\runtime.log"
exit /b %TASK_RESULT%
:failed
type "%BF2_TEMPLATE_CONSOLE_OUT%\compile.log"
if exist "%BF2_TEMPLATE_CONSOLE_OUT%\link.log" type "%BF2_TEMPLATE_CONSOLE_OUT%\link.log"
exit /b 1
