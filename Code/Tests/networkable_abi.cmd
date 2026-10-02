@rem SCAFFOLDING TEST - not recovered source and not part of the BF2 build.
@rem Builds x86 so size checks and raw MSVC vtable dispatch exercise the target ABI.
@echo off
setlocal
call "%~dp0..\Build\vsdevcmd.cmd" x86
pushd "%~dp0"
cl /nologo /EHsc /O2 /std:c++14 /permissive /DDEDICATED=1 /FI"%~dp0..\BF2\Core\BuildConfig.h" networkable_abi.cpp ^
  ..\BF2\IO\Network\INetworkable.cpp ^
  ..\BF2\IO\Network\NetworkableBase.cpp ^
  /link /OUT:networkable_abi.exe > networkable_abi_build.log 2>&1
if errorlevel 1 ( type networkable_abi_build.log & popd & exit /b 1 )
"%~dp0networkable_abi.exe"
set RC=%errorlevel%
popd
exit /b %RC%
