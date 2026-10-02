@rem SCAFFOLDING TEST - not recovered source and not part of the BF2 build.
@echo off
setlocal
call "%~dp0..\Build\vsdevcmd.cmd" x86
pushd "%~dp0"
cl /nologo /EHsc /O2 /std:c++14 /permissive /DDEDICATED=1 /FI"%~dp0..\BF2\Core\BuildConfig.h" bundle_serialization.cpp ^
  ..\BF2\Game\Objects\IObjectTemplate.cpp ^
  ..\BF2\Game\Objects\ObjectTemplate.cpp ^
  ..\BF2\Game\Objects\Base\SimpleObjectTemplate.cpp ^
  ..\BF2\Game\Objects\Base\BundleTemplate.cpp ^
  ..\BF2\Math\Mat4.cpp ^
  ..\BF2\Core\StringUtil.cpp ^
  ..\BF2\Core\Debug.cpp ^
  ..\BF2\Core\Globals.cpp ^
  /link /FORCE:UNRESOLVED /OUT:bundle_serialization.exe > bundle_serialization_build.log 2>&1
if errorlevel 1 ( type bundle_serialization_build.log & popd & exit /b 1 )
"%~dp0bundle_serialization.exe"
set RC=%errorlevel%
popd
exit /b %RC%
