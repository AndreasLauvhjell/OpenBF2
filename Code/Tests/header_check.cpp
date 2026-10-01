// Test scaffolding: includes otherwise-isolated engine headers for syntaxcheck.cmd.
// Fix declaration errors in their owning headers rather than adding workarounds here.
#include "../BF2/Math/Vec3.h"

// Core
#include "../BF2/Core/SettingsRepostitory.h"    // [sic] dice's spelling
#include "../BF2/Game/Objects/EngineTemplate.h"
#include "../BF2/Game/Objects/SupplyObjectTemplate.h"
#include "../BF2/Game/Objects/RemoteControlledObjectTemplate.h"
#include "../BF2/Game/Objects/DestroyableObjectTemplate.h"
#include "../BF2/Game/Objects/ForceObjectTemplate.h"
#include "../BF2/Game/Objects/AntennaObjectTemplate.h"
#include "../BF2/Game/Objects/TargetObjectTemplate.h"
#include "../BF2/Game/Objects/SpawnPointTemplate.h"
#include "../BF2/IO/BitStream.h"
#include "../BF2/IO/Network/INetworkable.h"
#include "../BF2/IO/Network/NetworkableInfo.h"
#include "../BF2/Game/GameLogic/ScoreSystem/TeamScore.h"
#include "../BF2/Game/Objects/IObjectComponent.h"

#include "../BF2/Game/Objects/WeaponComp.h"
#include "../BF2/IO/Network/NetworkableBase.h"
#include "../BF2/Game/Objects/Networkables/Networkables.h"

// Game/Objects: the template family
#include "../BF2/Game/Objects/IObject.h"
// Keep isolated Object and ObjectManager headers covered by the syntax gate.
#include "../BF2/Game/Objects/Object.h"
#include "../BF2/Scene/Object/ObjectManager.h"
#include "../BF2/Game/Objects/IObjectTemplate.h"
#include "../BF2/Game/Objects/IObjectTemplateComponent.h"
#include "../BF2/Game/Objects/ObjectTemplate.h"
#include "../BF2/Game/Objects/Base/BundleTemplate.h"
#include "../BF2/Game/Objects/Base/SimpleObjectTemplate.h"
#include "../BF2/Game/Objects/AnimatedBundleTemplate.h"
#include "../BF2/Game/Objects/RotationalBundleTemplate.h"
#include "../BF2/Physics/CollisionMeshTemplate.h"

#include "../BF2/Game/Objects/GameLogicComp/ArmorGLCompTemplate.h"
#include "../BF2/Game/Objects/GameLogicComp/ArmorGLComp.h"
#include "../BF2/Game/Objects/AbilityGLCompTemplate.h"
#include "../BF2/Game/Objects/RadioGLCompTemplate.h"

#include "../BF2/Game/Objects/AmmoCompTemplate.h"
#include "../BF2/Game/Objects/FireCompTemplate.h"
#include "../BF2/Game/Objects/SingleFireCompTemplate.h"
#include "../BF2/Game/Objects/ExplosivesFireCompTemplate.h"
#include "../BF2/Game/Objects/SpawnObjectFireCompTemplate.h"
#include "../BF2/Game/Objects/ZoomCompTemplate.h"
#include "../BF2/Game/Objects/ToggleCameraCompTemplate.h"
#include "../BF2/Game/Objects/GenericFireArmTemplate.h"

#include "../BF2/Game/Objects/WeaponHudCompTemplate.h"
#include "../BF2/Game/Objects/VehicleHudCompTemplate.h"
#include "../BF2/Game/Objects/HelpHudCompTemplate.h"
#include "../BF2/Game/Objects/WarningHudCompTemplate.h"
#include "../BF2/Game/Objects/AbilityHudCompTemplate.h"
#include "../BF2/Game/Objects/PostProcessCompTemplate.h"
#include "../BF2/Game/Objects/StrategicObjectCompTemplate.h"

#include "../BF2/Game/Objects/ItemTemplate.h"
#include "../BF2/Game/Objects/ItemContainerTemplate.h"
#include "../BF2/Game/Objects/Game/KitTemplate.h"
#include "../BF2/Game/Objects/Game/SoldierTemplate.h"
#include "../BF2/Game/Objects/PlayerControlObjectTemplate.h"

#include "../BF2/Game/Objects/CameraTemplate.h"
#include "../BF2/Game/Objects/FreeCameraTemplate.h"
#include "../BF2/Game/Objects/OverheadCameraTemplate.h"

#include "../BF2/Game/Objects/GrapplingHookRopeTemplate.h"
#include "../BF2/Game/Objects/WindAffectorTemplate.h"

// Component interfaces.
#include "../BF2/IO/Console/IConsoleSaveable.h"
#include "../BF2/Game/Objects/CompTemplateInterfaces.h"
#include "../BF2/Game/Objects/CompTemplateBases.h"
#include "../BF2/Game/Objects/CompTemplateLeaves.h"
#include "../BF2/Game/Objects/HudGLCompTemplates.h"
#include "../BF2/Game/Objects/Projectiles/ProjectileCompTemplates.h"
#include "../BF2/Game/Objects/Weapons/DeviationComponents/DeviationCompTemplate.h"

// Game
#include "../BF2/Game/MapInfo.h"
