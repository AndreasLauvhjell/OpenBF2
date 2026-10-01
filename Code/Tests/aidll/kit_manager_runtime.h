// [TEMP] Real headers and chosen external collaborators; production manager/template are whole TUs.
#pragma once
#include "../../BF2/AI/AIInterface/BFKnowledge/KitTemplateManager.h"
#include "../../BF2/AI/AIInterface/BFKnowledge/KitTemplate.h"
#include "../../BF2/AI/AIInterface/SingletonServer.h"
#include "../../BF2/AI/AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../BF2/Core/ClassManager.h"
#include "../../BF2/IO/Console/Console.h"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <type_traits>

[[noreturn]] void unexpected(const char* method);
