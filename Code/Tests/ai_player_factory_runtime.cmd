@echo off
setlocal
rem [TEMP] Actual-source AIPlayerFactory lookup fixture; no bot spawn or server run.
rem /FORCE is only for uncalled creation/registration dependencies, not link acceptance.
call "%~dp0..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_AI_FACTORY_OUT set "BF2_AI_FACTORY_OUT=%~dp0..\Build\ai_player_factory_runtime"
if not exist "%BF2_AI_FACTORY_OUT%" mkdir "%BF2_AI_FACTORY_OUT%"
set "TASK_FLAGS=/nologo /c /O2 /Gy /EHsc /MD /Z7 /std:c++14 /permissive /DDEDICATED=1"
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /Fo"%BF2_AI_FACTORY_OUT%\factory.obj" "%TASK_ROOT%\Code\BF2\AI\AIInterface\BFKnowledge\AIPlayerFactory.cpp" >"%BF2_AI_FACTORY_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_AI_FACTORY_OUT%\compile.log" & exit /b 1)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /Fo"%BF2_AI_FACTORY_OUT%\fixture.obj" "%~dp0ai_player_factory_runtime.cpp" >>"%BF2_AI_FACTORY_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_AI_FACTORY_OUT%\compile.log" & exit /b 1)
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /OUT:"%BF2_AI_FACTORY_OUT%\factory_runtime.exe" /PDB:"%BF2_AI_FACTORY_OUT%\factory_runtime.pdb" "%BF2_AI_FACTORY_OUT%\factory.obj" "%BF2_AI_FACTORY_OUT%\fixture.obj" >"%BF2_AI_FACTORY_OUT%\link_force.log" 2>&1
if errorlevel 1 (type "%BF2_AI_FACTORY_OUT%\link_force.log" & exit /b 1)
"%BF2_AI_FACTORY_OUT%\factory_runtime.exe" >"%BF2_AI_FACTORY_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_AI_FACTORY_OUT%\runtime.log"
exit /b %TASK_RESULT%
