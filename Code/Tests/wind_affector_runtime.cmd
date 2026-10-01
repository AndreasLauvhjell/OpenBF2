@rem SCAFFOLDING TEST - not recovered source and not part of the BF2 build.
@rem Builds x86 and clean-links the focused WindAffector/WindManager runtime suite.
@echo off
setlocal
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
pushd "%~dp0"
cl /nologo /EHsc /O2 /std:c++14 /permissive /Gy /DDEDICATED=1 /FI"%~dp0..\BF2\Core\BuildConfig.h" wind_affector_runtime.cpp ^
  ..\BF2\Game\Objects\IObjectComponent.cpp ^
  ..\BF2\Game\Objects\WindAffector.cpp ^
  ..\BF2\Scene\Object\WindManager.cpp ^
  ..\BF2\Core\StringUtil.cpp ^
  /link /OPT:REF /OUT:wind_affector_runtime.exe > wind_affector_runtime_build.log 2>&1
if errorlevel 1 ( type wind_affector_runtime_build.log & popd & exit /b 1 )
"%~dp0wind_affector_runtime.exe"
set RC=%errorlevel%
popd
exit /b %RC%
