// Opt-in factory instantiations for classes whose construction and virtual surface are linkable.
// [CHOSEN, not recovered] file organization: ModuleRegistry declares factories without instantiating
// every class. Verify each new class with a factory-only link probe before adding it here.
// The separate template shapes are an intentional ODR compromise. Once all registry classes can
// be instantiated, ModuleRegistry can include ClassFactory.h directly and this file can be removed.

#include "ClassFactory.h"

// The five classes ClassManager::initSingletons reaches on the boot path, plus ai::AIConnection,
// which BF2EngineSetup::initAI reaches immediately after. Core/ClassFactory.h's sequencing note has
// the evidence for why these six and no others are boot-critical.
#include "../Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../Scene/Object/WorldObjTemplPortal/PortalManager.h"
#include "../Physics/BasicPhysicsSystem.h"
#include "../Physics/PhysicsNodeManager.h"
#include "../Physics/PhysicsNode.h"
#include "../Physics/ResponsePhysics.h"
#include "../Physics/ResponsePhysicsManager.h"
#include "../AI/AIInterface/Main/AIConnection.h"

// World::ClassFactory<T>
// 122 of the 124 instantiations live in the `world` template, not `dice::hfe`'s. The three
// namespaces mangle differently under MSVC and are three distinct templates - see the census in
// Core/ClassFactory.h. Writing `dice::hfe::ClassFactory<world::X>` here would emit a symbol nobody
// asks for and leave the real one unresolved.
template struct dice::hfe::world::ClassFactory<dice::hfe::world::ObjectTemplateManager>;
template struct dice::hfe::world::ClassFactory<dice::hfe::world::PortalManager>;
template struct dice::hfe::world::ClassFactory<dice::hfe::world::BasicPhysicsSystem>;
template struct dice::hfe::world::ClassFactory<dice::hfe::world::PhysicsNodeManager>;
// ELF 0x8846780 / 0x8846840; PE 0x692180 / 0x692220 (roster 0x691E50):
// the normal outer guard, allocation, query and release on unsupported interface.
template struct dice::hfe::world::ClassFactory<dice::hfe::world::PhysicsNode>;
template struct dice::hfe::world::ClassFactory<dice::hfe::world::ResponsePhysics>;
template struct dice::hfe::world::ClassFactory<dice::hfe::world::ResponsePhysicsManager>;

// Ai::ClassFactory<T>
// The only instantiation of the `ai` template in the whole binary.
template struct dice::hfe::ai::ClassFactory<dice::hfe::ai::AIConnection>;
