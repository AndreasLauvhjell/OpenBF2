// [TEMP]
// Test-only fallback symbols for console fixtures that omit the AI object runtime.
// Include once at namespace scope. /alternatename yields to real definitions; unexpected calls report and abort.
#include <cstdio>
#include <cstdlib>

extern "C" [[noreturn]] void fixtureAiRuntimeUnreached()
{
    std::printf("UNEXPECTED the AI object runtime (not linked into this fixture)\n");
    std::fflush(stdout);
    std::abort();
}

#pragma comment(linker, "/alternatename:??0AIObjectArmament@ai@hfe@dice@@QAE@PAVIAIObject@123@PAVAITemplateArmament@123@@Z=_fixtureAiRuntimeUnreached")
#pragma comment(linker, "/alternatename:??0AIObjectControlInfo@ai@hfe@dice@@QAE@PAVIAIObject@123@PAVAITemplateControlInfo@123@@Z=_fixtureAiRuntimeUnreached")
#pragma comment(linker, "/alternatename:??0AIObjectCover@ai@hfe@dice@@QAE@PAVIAIObject@123@PAVAITemplateCover@123@@Z=_fixtureAiRuntimeUnreached")
#pragma comment(linker, "/alternatename:??0AIObjectMobile@ai@hfe@dice@@QAE@PAVIAIObject@123@PAVAITemplateMobile@123@@Z=_fixtureAiRuntimeUnreached")
#pragma comment(linker, "/alternatename:??0AIObjectPhysical@ai@hfe@dice@@QAE@PAVIAIObject@123@PAVAITemplatePhysical@123@@Z=_fixtureAiRuntimeUnreached")
#pragma comment(linker, "/alternatename:??0AIObjectPrimaryReal@ai@hfe@dice@@QAE@PAVAITemplate@123@HPAVIObject@world@23@@Z=_fixtureAiRuntimeUnreached")
#pragma comment(linker, "/alternatename:??0AIObjectPrimaryShared@ai@hfe@dice@@QAE@PAVAITemplate@123@HPAVIObject@world@23@@Z=_fixtureAiRuntimeUnreached")
#pragma comment(linker, "/alternatename:??0AIObjectSecondaryReal@ai@hfe@dice@@QAE@PAVAITemplate@123@HPAVIObject@world@23@@Z=_fixtureAiRuntimeUnreached")
#pragma comment(linker, "/alternatename:??0AIObjectSecondaryShared@ai@hfe@dice@@QAE@PAVAITemplate@123@HPAVIObject@world@23@@Z=_fixtureAiRuntimeUnreached")
#pragma comment(linker, "/alternatename:??0AIObjectUnit@ai@hfe@dice@@QAE@PAVIAIObject@123@PAVAITemplateUnit@123@@Z=_fixtureAiRuntimeUnreached")
#pragma comment(linker, "/alternatename:??0PlanePolygon@hfe@dice@@QAE@ABV?$list@UVec2@Math@hfe@dice@@V?$allocator@UVec2@Math@hfe@dice@@@std@@@std@@@Z=_fixtureAiRuntimeUnreached")
#pragma comment(linker, "/alternatename:?findCameras@world@hfe@dice@@YAXPAVIObject@123@HAAV?$map@HPAVICameraObject@world@hfe@dice@@U?$less@H@std@@V?$allocator@U?$pair@$$CBHPAVICameraObject@world@hfe@dice@@@std@@@6@@std@@@Z=_fixtureAiRuntimeUnreached")
#pragma comment(linker, "/alternatename:?getFloat@Vars@hfe@dice@@SAPAMPBDM@Z=_fixtureAiRuntimeUnreached")
#pragma comment(linker, "/alternatename:??0WeaponObject@ai@hfe@dice@@QAE@PAVWeaponTemplate@123@@Z=_fixtureAiRuntimeUnreached")
