@rem SCAFFOLDING TEST - not recovered source and not part of the BF2 build.
@rem Function-level linking keeps this test honest about its scope: it links TemplateInfo's recovered
@rem ctor/dtor while discarding Bundle methods that intentionally still depend on unrecovered runtime
@rem object, stream, script, and sound boundaries.
@echo off
setlocal
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
pushd "%~dp0"
cl /nologo /EHsc /O2 /std:c++14 /permissive /Gy /DDEDICATED=1 /FI"%~dp0..\BF2\Core\BuildConfig.h" bundle_template_core.cpp ..\BF2\Game\Objects\Base\BundleTemplate.cpp ..\BF2\Math\Mat4.cpp /link /OPT:REF /FORCE:UNRESOLVED /OUT:bundle_template_core.exe > bundle_template_core_build.log 2>&1
if errorlevel 1 ( type bundle_template_core_build.log & popd & exit /b 1 )
"%~dp0bundle_template_core.exe"
set RC=%errorlevel%
popd
exit /b %RC%
