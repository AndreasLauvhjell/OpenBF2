"""[TEMP] Chunk 00 gate: load the rebuilt AIDLL_w32ded.dll from a coop boot and read where it stops.

SCAFFOLDING - NOT RECOVERED SOURCE.

What this proves, and nothing more: that `GameServer::loadPath` resolves UseAI, that
`BF2Engine::loadAI` -> `AIInterfaceModule::initAIConnection` -> `AIConnection::init` ->
`LoadLibraryBf(".\\<AIDLL>")` maps the rebuilt DLL, that its `initDll` returns true, that
`AIModule::init` publishes the simple singleton "AI", and that `loadPath` then calls
`aiSingleton->init()`. The stop is the measurement: whichever named frontier the run reaches
FIRST is the finding. The initialization gate alone is not a claim that any bot exists.
--probe-squads optionally reads the engine's player/squad state after game-logic update and
before AI update; it observes membership only, not a spawned or acting bot.
--probe-kits reads the populated engine kit vector and strength arrays at that same point.
It also reports stored spawn group/kit after the real Player::setKit store inside AIPlayer::setSpawnKit.
--probe-templates (chunk 03B's gate) counts what the stock ai/Objects.ai scripts put into the AI
template and plug-in managers, reports it at `ai.init` (createWreckTemplates), and names the template
of the first AITemplate::createObject; per-entry lines go to the log only.

Reach levels, never collapsed (AGENTS.md): compile | diagnostic link | DLL load |
BF2AI::init complete | scripts parsed | level data loaded | bots joined | bots playable.

The AI defaults stay off in production. This harness turns them on for one run, in a PRIVATE
ServerSettings.con, on an explicitly configured port that is not 16567 (the owner's) and not
16986 (tier2's), on loopback only. It never writes into the game directory and never copies the
DLL anywhere - the engine reaches it through a RELATIVE AIDLL setting, because LoadLibraryBf
builds ".\\" + name against the CURRENT DIRECTORY.

    python Code\\Tests\\aidll\\run.py --build-out Code\\Build\\obj_<agent> --out Code\\Build\\aidll_<agent>
"""
from pathlib import Path
import argparse
import json
import os
import re
import shutil
import socket
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def default_vcvars():
    """Resolve VsDevCmd.bat: BF2_VSDEVCMD, then vswhere."""
    override = os.environ.get('BF2_VSDEVCMD')
    if override:
        return Path(override)
    program_files_x86 = os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)')
    vswhere = Path(program_files_x86) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    if vswhere.is_file():
        try:
            found = subprocess.run(
                [str(vswhere), '-latest', '-products', '*',
                 '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
                 '-property', 'installationPath'],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                universal_newlines=True, check=True)
        except (OSError, subprocess.CalledProcessError):
            found = None
        if found is not None:
            for line in found.stdout.splitlines():
                candidate = Path(line.strip()) / 'Common7/Tools/VsDevCmd.bat'
                if candidate.is_file():
                    return candidate
    raise RuntimeError('Visual Studio developer environment not found; '
                       'set BF2_VSDEVCMD to your VsDevCmd.bat')


# The gate lines, in the order the boot must produce them. Each is a DIFFERENT claim, and the run
# reports which prefix it reached rather than one pass/fail - the stop IS the measurement.
#
# (!) `[boot] BF2::init returned TRUE` IS DELIBERATELY NOT IN THIS LIST. With `+loadLevel` on the
# command line the whole map load happens INSIDE BF2::init (BF2.cpp:181 -> BF2Engine.cpp:2160 ->
# GameServer::loadPath), so on a successful run the DLL's frontier aborts the process BEFORE init
# ever returns. Requiring that line would make a fully passing gate report as a failure at step 1,
# which is what it did on the first green run (2026-09-08).
GATE = [
    # LoadLibraryBf mapped the rebuilt DLL AND its initDll returned true. Both, in one line: a false
    # initDll makes LoadLibraryBf FreeLibrary the module and return 0, and AIConnection::init then
    # takes its shipped abort() rather than reporting anything.
    ('[aidll] BF2Engine::loadAI() = 1',  'loadAI: the DLL is mapped and initDll returned true'),
    # AIModule::init ran inside initDll and its `new BF2AI` published the simple singleton "AI".
    ('[aidll] AI singleton present = 1', 'AIModule::init published the "AI" singleton'),
    # GameServer::loadPath called aiSingleton->init() and the DLL stopped in its own named frontier.
    #
    # (!) THE MESSAGE MOVED ON 2026-09-08 (chunk 02) AND THAT IS THE POINT OF THE GATE. It used to
    # read "BF2AI::init reached", i.e. the very first instruction of the body. It now names step 4,
    # which means steps 1-3 and the first wiring pass RAN: the information pool, the weapon-info
    # pool, the AISingletonServerUpdater, AISingletonServer::updateStaticMethods(), and
    # IBFModuleCourier::initSingletons().
    #
    # (!) SO THIS ONE LINE ALSO PROVES CHUNK 01'S SIXTEEN ADAPTERS ARE BUILT ON THE PRODUCTION PATH.
    # initSingletons() forwards to BFConsole::createSingletons(), and the run between it and this
    # stop is straight-line with no branches - if createSingletons had reached a frontier of its own
    # the process would have aborted with THAT name instead. --probe-adapters remains available for
    # reading the roster itself, but it is no longer the only evidence that createSingletons runs.
    # (!!) THE GATE NO LONGER LOOKS FOR A FRONTIER HERE, AND THAT IS THE HEADLINE. Until
    # 2026-09-09 every row of this list ended in a named stop inside `BF2AI::init`; the two
    # below are the first that assert SUCCESS. `init` now runs all twenty-three subsystems,
    # both wiring passes and `AIRandom::init`'s wall-clock seed, and returns true.
    #
    # (!) THE SECOND ROW IS THE ONE THAT COSTS SOMETHING TO FAKE. `isInitialized` reads the
    # flag `init` sets after step 2 and `isShuttingDown` the byte its tail clears, so the pair
    # separates a real completed run from an early `return true`.
    ('[aidll] BF2AI::init() = 1',
     'BF2AI::init ran all 23 subsystems, both wiring passes and the wall-clock seed, and '
     'returned true'),
    ('[aidll] isInitialized = 1 isShuttingDown = 0',
     'the AI reports itself initialized and not shutting down'),
]


def once(source, old, new):
    if source.count(old) != 1:
        raise RuntimeError('Instrumentation anchor changed: ' + old.strip()[:80])
    return source.replace(old, new)


def prepend(source, line):
    """Put `line` at the very top - but AFTER a UTF-8 BOM if the file carries one.

    read_text(encoding='utf-8') keeps the BOM as a leading U+FEFF character. Several engine sources
    have one (Core/ModuleRegistry.cpp does), and moving it off line 1 makes cl reject the file with
    C3872 "this character is not allowed in an identifier".
    """
    if source.startswith('\ufeff'):
        return '\ufeff' + line + source[1:]
    return line + source


def includes(source, origin):
    return re.sub(r'#include "([^"]+)"', lambda m: '#include "' +
                  (origin / m[1]).resolve().as_posix() + '"', source)


def boot():
    """bootmain with one addition: AIDLL registered as a settable var before BF2 is constructed.

    `AIDLL` is not a retail user variable - the retail server takes it from its own config - so
    the private ServerSettings.con below cannot set it until it exists. tier2/run.py registers it
    the same way, for the same reason, and sets it to the empty string.
    """
    source = includes((ROOT / 'Code/Build/bootmain.cpp').read_text(encoding='utf-8'),
                      ROOT / 'Code/Build')
    source = once(source, 'int main(int argc, const char** argv)',
                  '#include "' + (ROOT / 'Code/BF2/IO/SettingsManager.h').as_posix() + '"\n'
                  'int main(int argc, const char** argv)')
    source = once(source, '    dice::hfe::BF2* bf2 = new dice::hfe::BF2();',
                  '    dice::hfe::io::settingsManager->registerUserVar("AIDLL", false, dice::hfe::io::VariableSet(0));\n'
                  '    dice::hfe::BF2* bf2 = new dice::hfe::BF2();')
    # Loopback only, exactly as tier2 does it - this harness must never bind a routable interface.
    source = once(source, 'addr.sin_addr.s_addr = INADDR_ANY;',
                  'addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);')
    source = source.replace('rebind probe: bind(0.0.0.0:%d)', 'rebind probe: bind(127.0.0.1:%d)')
    return source


def instrumented():
    """Observed copies of engine sources. The tree itself is never edited.

    GameServer's loadPath reports the existing initialization calls. An optional, test-only
    observer reads player/squad state once after game-logic update and reports spawn metadata
    after the real kit setter. ModuleRegistry reports
    required singleton lookups. No original calls, arguments or control decisions are replaced.
    """
    result = {}

    original = ROOT / 'Code/BF2/Game/GameServer/GameServer.cpp'
    source = includes(original.read_text(encoding='utf-8'), original.parent)
    source = prepend(source, '#include <cstdio>\n#include <cstdlib>\n')
    source = prepend(source,
                     '#include "' + (ROOT / 'Code/BF2/AI/AIInterface/BFKnowledge/KitTemplate.h').as_posix() + '"\n'
                     '#include "' + (ROOT / 'Code/BF2/AI/AIInterface/BFKnowledge/KitTemplateManager.h').as_posix() + '"\n')

    source = once(source,
                  '    if (settings->boolGet("UseAI", false))\n'
                  '        bf2Engine->loadAI();',
                  '    if (settings->boolGet("UseAI", false))\n'
                  '    {\n'
                  '        const bool aidllUp = bf2Engine->loadAI();\n'
                  '        std::printf("[aidll] BF2Engine::loadAI() = %d\\n", (int)aidllUp);\n'
                  '        std::fflush(stdout);\n'
                  '    }\n'
                  '    else if (false)\n'
                  '        bf2Engine->loadAI();')

    source = once(source,
                  '    if (aiSingleton)\n'
                  '        aiSingleton->init();',
                  '    std::printf("[aidll] getSimpleSingleton(\\"AI\\") = %p\\n", (void*)aiSingleton);\n'
                  '    std::printf("[aidll] AI singleton present = %d\\n", aiSingleton ? 1 : 0);\n'
                  '    std::fflush(stdout);\n'
                  '    if (aiSingleton)\n'
                  '    {\n'
                  '        const bool aiInitOk = aiSingleton->init();\n'
                  '        std::printf("[aidll] BF2AI::init() = %d\\n", (int)aiInitOk);\n'
                  '        std::printf("[aidll] isInitialized = %d isShuttingDown = %d\\n",\n'
                  '                    (int)aiSingleton->isInitialized(), (int)aiSingleton->isShuttingDown());\n'
                  '        std::fflush(stdout);\n'
                  '    }')
    source = once(source,
                  '    gameLogic->update(dt);                                      // +0x20\n',
                  '    gameLogic->update(dt);                                      // +0x20\n'
                  '    // [TEMP] Observe actual script-populated templates; no injected kits.\n'
                  '    static bool reportedKits = false;\n'
                  '    if (!reportedKits && std::getenv("BF2_AIDLL_KIT_PROBE"))\n'
                  '    {\n'
                  '        reportedKits = true;\n'
                  '        const ai::KitTemplateManager* manager = ai::KitTemplateManager::m_instance;\n'
                  '        unsigned int populated = 0;\n'
                  '        if (manager)\n'
                  '        {\n'
                  '            const std::vector<ai::IKitTemplate*>& kits = manager->getAllTemplates();\n'
                  '            for (unsigned int i = 0; i < kits.size(); ++i)\n'
                  '            {\n'
                  '                const ai::IKitTemplate* kit = kits[i];\n'
                  '                if (!kit) continue;\n'
                  '                ++populated;\n'
                  '                const int* strategic = kit->getStrategicStrengths();\n'
                  '                const int* battle = kit->getBattleStrengths();\n'
                  '                std::printf("[aidll-kits] index=%u type=%d strategic=%d,%d battle=%d,%d,%d,%d,%d,%d\\n",\n'
                  '                    i, (int)kit->getType(), strategic[0], strategic[1],\n'
                  '                    battle[0], battle[1], battle[2], battle[3], battle[4], battle[5]);\n'
                  '            }\n'
                  '            std::printf("[aidll-kits] slots=%u populated=%u after gameLogic update\\n",\n'
                  '                (unsigned int)kits.size(), populated);\n'
                  '        }\n'
                  '        else std::printf("[aidll-kits] manager missing\\n");\n'
                  '        std::fflush(stdout);\n'
                  '    }\n'
                  '    // [TEMP] Read-only engine roster observation; no admission is injected.\n'
                  '    static bool reportedSquads = false;\n'
                  '    if (!reportedSquads && std::getenv("BF2_AIDLL_SQUAD_PROBE"))\n'
                  '    {\n'
                  '        reportedSquads = true;\n'
                  '        const std::list<world::IPlayer*>& players = world::playerManager->getPlayers();\n'
                  '        unsigned int aiPlayers = 0;\n'
                  '        for (world::IPlayer* player : players)\n'
                  '        {\n'
                  '            if (!player->getIsAIPlayer()) continue;\n'
                  '            ++aiPlayers;\n'
                  '            const int id = player->getId();\n'
                  '            const int team = player->getTeam();\n'
                  '            const int squad = player->getSquadId();\n'
                  '            const bool leader = player->getIsSquadLeader();\n'
                  '            const bool alive = player->getIsAlive();\n'
                  '            std::printf("[aidll-squads] player=%d team=%d squad=%d leader=%d alive=%d\\n",\n'
                  '                        id, team, squad, (int)leader, (int)alive);\n'
                  '        }\n'
                  '        std::printf("[aidll-squads] engine AI players=%u after gameLogic update\\n", aiPlayers);\n'
                  '        std::fflush(stdout);\n'
                  '    }\n')
    result[original] = source

    # [TEMP] A later squad frontier can also be reached by a budget return before
    # selection. Observe actual engine metadata after the unmodified adapter store
    # to distinguish a completed kit assignment from that earlier return.
    original = ROOT / 'Code/BF2/AI/AIInterface/BFKnowledge/AIPlayer.cpp'
    source = includes(original.read_text(encoding='utf-8'), original.parent)
    source = prepend(source, '#include <cstdio>\n#include <cstdlib>\n')
    source = once(source,
                  'void AIPlayer::setSpawnKit(int kit)\n'
                  '{\n'
                  '    m_player->setKit(kit);\n'
                  '}\n',
                  'void AIPlayer::setSpawnKit(int kit)\n'
                  '{\n'
                  '    m_player->setKit(kit);\n'
                  '    if (std::getenv("BF2_AIDLL_KIT_PROBE"))\n'
                  '    {\n'
                  '        std::printf("[aidll-spawn] player=%d group=%d kit=%d alive=%d after AIPlayer::setSpawnKit\\n",\n'
                  '            m_player->getId(), m_player->getSpawnGroup(), m_player->getKit(),\n'
                  '            (int)m_player->getIsAlive());\n'
                  '        std::fflush(stdout);\n'
                  '    }\n'
                  '}\n')
    result[original] = source

    # ModuleRegistry: read the host registry with the SAME nineteen names initDll will use, one
    # step before LoadLibraryBf. initDll returns a 16-term && chain over them and reports nothing
    # when it fails - and a false return makes LoadLibraryBf FreeLibrary the module and hand back 0,
    # which sends AIConnection::init straight into its shipped abort(). This probe is the only way
    # to see WHICH name was missing; it reads the registry and changes nothing.
    registry = ROOT / 'Code/BF2/Core/ModuleRegistry.cpp'
    text = includes(registry.read_text(encoding='utf-8'), registry.parent)
    text = prepend(text, '#include <cstdio>\n#include <cstdlib>\n')
    text = once(text,
                '    std::string dll = settings->stringGet(std::string("AIDLL"), std::string(""));\n'
                '    if (dll.empty())\n'
                '        return false;\n',
                '    std::string dll = settings->stringGet(std::string("AIDLL"), std::string(""));\n'
                '    if (dll.empty())\n'
                '        return false;\n'
                '    {\n'
                '        static const char* const kInitDllNames[] = {\n'
                '            "SettingsRepostitory", "MainConsoleObjects", "MainConsole", "FileManager",\n'
                '            "FileChangeManager", "BfGlobal_callback", "BfGlobal_callbackData", "DebugOn",\n'
                '            "DebugModules", "Profiler", "System", "LoadStat", "EventManager",\n'
                '            "FrameEventManager", "ObjectManager", "ObjectTemplateManager",\n'
                '            "PlayerManager", "Game", "AIConnection" };\n'
                '        for (unsigned i = 0; i < sizeof(kInitDllNames)/sizeof(kInitDllNames[0]); ++i)\n'
                '        {\n'
                '            void* const found = classManager->getSingleton(std::string(kInitDllNames[i]));\n'
                '            std::printf("[aidll] getSingleton(\\"%s\\") = %p%s\\n", kInitDllNames[i], found,\n'
                '                        found ? "" : "   <- NULL");\n'
                '        }\n'
                '        std::fflush(stdout);\n'
                '    }\n')

    # CHUNK 01'S GATE, opt-in through BF2_AIDLL_ADAPTER_PROBE. `BFConsole::createSingletons` is
    # normally reached from BF2AI::init's first wiring pass, which is chunk 02's frontier - so
    # nothing calls it yet. This probe calls the REAL function one step early, from the point where
    # initAIConnection has just built the BFConsole, and then reads the roster it is supposed to
    # have filled. It adds no behaviour: createSingletons is idempotent (both its halves are
    # guarded) and the three refreshes at its tail are the same three initAIConnection already runs.
    text = once(text,
                '    AIInstanceServer::updateInstance();\n'
                '    BFSingletonServer::updateStaticMethods();\n'
                '    return true;\n'
                '}\n',
                '    AIInstanceServer::updateInstance();\n'
                '    BFSingletonServer::updateStaticMethods();\n'
                '    if (std::getenv("BF2_AIDLL_ADAPTER_PROBE") != 0 && BFConsole::m_instance)\n'
                '    {\n'
                '        std::printf("[adapters] calling BFConsole::createSingletons()\\n");\n'
                '        std::fflush(stdout);\n'
                '        BFConsole::m_instance->createSingletons();\n'
                '        std::printf("[adapters] createSingletons returned\\n");\n'
                '        static const char* const kRoster[] = {\n'
                '            "IBFTimer", "IAITerrain", "IAIGame", "IAIGameEffector",\n'
                '            "IAIObjectEnvironment", "IAIPhysics", "IBFCollisionHandler",\n'
                '            "IKitTemplateManager", "IAIPlayerFactory", "IMapRenderer",\n'
                '            "IDebugRenderer", "IBFModuleCourier", "IBFCommander", "IBFSquad",\n'
                '            "IBFRadioComm", "IBFGraphCreator" };\n'
                '        int filled = 0;\n'
                '        for (unsigned i = 0; i < sizeof(kRoster)/sizeof(kRoster[0]); ++i)\n'
                '        {\n'
                '            void* const found = classManager->getSimpleSingleton(std::string(kRoster[i]));\n'
                '            if (found) ++filled;\n'
                '            std::printf("[adapters] %-22s = %p%s\\n", kRoster[i], found,\n'
                '                        found ? "" : "   <- NULL");\n'
                '        }\n'
                '        std::printf("[adapters] roster filled = %d of 16\\n", filled);\n'
                '        std::fflush(stdout);\n'
                '    }\n'
                '    return true;\n'
                '}\n')
    result[registry] = text

    # [TEMP] Chunk 03B's gate (2026-09-28): what the stock ai/Objects.ai files put into the template
    # managers. Opt-in through BF2_AIDLL_TEMPLATE_PROBE. Every addition only PRINTS - after the
    # recovered statement it follows, never instead of one. Per-entry lines use the `[aidll-tpl]`
    # prefix (log only); the summaries use `[aidll-templates]` (echoed by the driver).
    plugins = ROOT / 'Code/BF2/AI/AIInterface/BFKnowledge/AITemplatePlugInManager.cpp'
    text = includes(plugins.read_text(encoding='utf-8'), plugins.parent)
    text = prepend(text, '#include <cstdio>\n#include <cstdlib>\n'
                         'namespace { unsigned g_probePlugIns[6]; unsigned g_probeBadTypes; }\n'
                         'extern "C" unsigned aidllProbePlugInCount(int kind)\n'
                         '{ return kind < 0 ? g_probeBadTypes : g_probePlugIns[kind]; }\n')
    text = once(text,
                '    m_byName.insert(std::make_pair(name, m_active));\n',
                '    m_byName.insert(std::make_pair(name, m_active));\n'
                '    if (std::getenv("BF2_AIDLL_TEMPLATE_PROBE"))\n'
                '    {\n'
                '        ++g_probePlugIns[m_active->m_plugIn->getId()];\n'
                '        std::printf("[aidll-tpl] plugin %s %s\\n", type.c_str(), name.c_str());\n'
                '    }\n')
    text = once(text,
                '        return "TemplatePlugIn type \'" + type + "\' doesn\'t exist";\n',
                '        if (std::getenv("BF2_AIDLL_TEMPLATE_PROBE"))\n'
                '        {\n'
                '            ++g_probeBadTypes;\n'
                '            std::printf("[aidll-tpl] BAD plugin type %s for %s\\n", type.c_str(), name.c_str());\n'
                '        }\n'
                '        return "TemplatePlugIn type \'" + type + "\' doesn\'t exist";\n')
    result[plugins] = text

    weapons = ROOT / 'Code/BF2/AI/AIInterface/BFKnowledge/WeaponTemplateManager.cpp'
    text = includes(weapons.read_text(encoding='utf-8'), weapons.parent)
    text = prepend(text, '#include <cstdio>\n#include <cstdlib>\n'
                         'namespace { unsigned g_probeWeapons; }\n'
                         'extern "C" unsigned aidllProbeWeaponCount() { return g_probeWeapons; }\n')
    text = once(text,
                '    m_byName[name] = m_active;\n',
                '    m_byName[name] = m_active;\n'
                '    ++g_probeWeapons;\n')
    result[weapons] = text

    templates = ROOT / 'Code/BF2/AI/AIInterface/BFKnowledge/AITemplateManager.cpp'
    text = includes(templates.read_text(encoding='utf-8'), templates.parent)
    text = prepend(text, '#include <cstdio>\n#include <cstdlib>\n'
                         'extern "C" unsigned aidllProbePlugInCount(int kind);\n'
                         'extern "C" unsigned aidllProbeWeaponCount();\n'
                         'namespace { unsigned g_probeAdded; unsigned g_probeMissing; }\n')
    text = once(text,
                '    m_byName[name] = m_active;\n',
                '    m_byName[name] = m_active;\n'
                '    if (std::getenv("BF2_AIDLL_TEMPLATE_PROBE"))\n'
                '        std::printf("[aidll-tpl] template %s\\n", name.c_str());\n')
    text = once(text,
                '            m_active->m_template->addPlugIn(plugIn);\n',
                '            m_active->m_template->addPlugIn(plugIn);\n'
                '            ++g_probeAdded;\n')
    text = once(text,
                '        return "AITemplatePlugIn \'" + name + "\' doesn\'t exist";\n',
                '        ++g_probeMissing;\n'
                '        if (std::getenv("BF2_AIDLL_TEMPLATE_PROBE"))\n'
                '            std::printf("[aidll-tpl] MISSING plugin %s for template %s\\n",\n'
                '                        name.c_str(), m_active->m_name.c_str());\n'
                '        return "AITemplatePlugIn \'" + name + "\' doesn\'t exist";\n')
    text = once(text,
                'void AITemplateManager::createWreckTemplates()\n'
                '{\n',
                'void AITemplateManager::createWreckTemplates()\n'
                '{\n'
                '    if (std::getenv("BF2_AIDLL_TEMPLATE_PROBE"))\n'
                '    {\n'
                '        std::printf("[aidll-templates] at ai.init: weapons=%u\\n", aidllProbeWeaponCount());\n'
                '        std::printf("[aidll-templates] at ai.init: templates=%u plugins=%u'
                ' (armament=%u controlInfo=%u cover=%u mobile=%u physical=%u unit=%u) badTypes=%u'
                ' addPlugIn ok=%u missing=%u\\n",\n'
                '            (unsigned)m_byName.size(),\n'
                '            aidllProbePlugInCount(0) + aidllProbePlugInCount(1) + aidllProbePlugInCount(2)\n'
                '                + aidllProbePlugInCount(3) + aidllProbePlugInCount(4) + aidllProbePlugInCount(5),\n'
                '            aidllProbePlugInCount(0), aidllProbePlugInCount(1), aidllProbePlugInCount(2),\n'
                '            aidllProbePlugInCount(3), aidllProbePlugInCount(4), aidllProbePlugInCount(5),\n'
                '            aidllProbePlugInCount(-1), g_probeAdded, g_probeMissing);\n'
                '        std::fflush(stdout);\n'
                '    }\n')
    text = once(text,
                '        if (it->second)\n'
                '            it->second->m_template->createWreckTemplate();\n'
                '    }\n',
                '        if (it->second)\n'
                '            it->second->m_template->createWreckTemplate();\n'
                '    }\n'
                '    if (std::getenv("BF2_AIDLL_TEMPLATE_PROBE"))\n'
                '    {\n'
                '        unsigned wrecks = 0;\n'
                '        for (std::map<std::string, MgrTemplate*, NoCaseStringCompare>::iterator it = m_byName.begin();\n'
                '             it != m_byName.end(); ++it)\n'
                '            if (it->second && it->second->m_template->m_wreckTemplate) ++wrecks;\n'
                '        std::printf("[aidll-templates] wreck templates built=%u\\n", wrecks);\n'
                '        std::fflush(stdout);\n'
                '    }\n')
    result[templates] = text

    # 2026-09-29 (chunk 07): createObject is WRITTEN, so it now runs once per level object that names
    # an AI template. Every call is logged with a running count (`[aidll-tpl]`, log only) and the
    # FIRST is echoed (`[aidll-templates]`); the last logged count is the number of AI objects built
    # before the run's stop. Printing only - the recovered body follows unchanged.
    aiTemplate = ROOT / 'Code/BF2/AI/AIInterface/BFKnowledge/AITemplate.cpp'
    text = includes(aiTemplate.read_text(encoding='utf-8'), aiTemplate.parent)
    text = prepend(text, '#include <cstdio>\n#include <cstdlib>\n'
                         'extern "C" __declspec(dllimport) unsigned short __stdcall\n'
                         '    RtlCaptureStackBackTrace(unsigned long, unsigned long, void**, unsigned long*);\n'
                         'extern "C" __declspec(dllimport) void* __stdcall GetModuleHandleA(const char*);\n'
                         'namespace { unsigned g_probeCreated; }\n')
    text = once(text,
                'IAIObject* AITemplate::createObject(int side, world::IObject* object)\n'
                '{\n',
                'IAIObject* AITemplate::createObject(int side, world::IObject* object)\n'
                '{\n'
                '    if (std::getenv("BF2_AIDLL_TEMPLATE_PROBE"))\n'
                '    {\n'
                '        ++g_probeCreated;\n'
                '        std::printf("%s createObject #%u template=%s types=0x%X plugInMask=0x%X'
                ' secondary=%d commonKnowledge=%d side=%d\\n",\n'
                '                    g_probeCreated == 1 ? "[aidll-templates]" : "[aidll-tpl]",\n'
                '                    g_probeCreated, m_name.c_str(), (unsigned)m_types,\n'
                '                    (unsigned)m_plugInMask, (int)m_secondary, (int)m_commonKnowledge, side);\n'
                '        if (g_probeCreated <= 2)\n'
                '        {\n'
                '            // The caller chain, as module RVAs (symbolise against runtime.map).\n'
                '            void* frames[24];\n'
                '            const unsigned short n = RtlCaptureStackBackTrace(0, 24, frames, 0);\n'
                '            const unsigned long base = (unsigned long)GetModuleHandleA(0);\n'
                '            for (unsigned short i = 0; i < n; ++i)\n'
                '                std::printf("[aidll-tpl]   createObject #%u frame %u RVA 0x%08lX\\n",\n'
                '                            g_probeCreated, (unsigned)i, (unsigned long)frames[i] - base);\n'
                '        }\n'
                '        std::fflush(stdout);\n'
                '    }\n')
    result[aiTemplate] = text

    # ...and every AI object whose init() actually runs (Real and Shared both open with the shared
    # IAIObjectInit): the evidence that create -> setComponent -> release -> init completed.
    aiObject = ROOT / 'Code/BF2/AI/AIInterface/BFKnowledge/AIObject.cpp'
    text = includes(aiObject.read_text(encoding='utf-8'), aiObject.parent)
    text = prepend(text, '#include <cstdio>\n#include <cstdlib>\n'
                         'namespace { unsigned g_probeInits; }\n')
    text = once(text,
                'void AIObject::IAIObjectInit()\n'
                '{\n',
                'void AIObject::IAIObjectInit()\n'
                '{\n'
                '    if (std::getenv("BF2_AIDLL_TEMPLATE_PROBE"))\n'
                '    {\n'
                '        ++g_probeInits;\n'
                '        std::printf("%s IAIObjectInit #%u template=%s side=%d plugIns=%d%d%d%d%d%d\\n",\n'
                '                    g_probeInits == 1 ? "[aidll-templates]" : "[aidll-tpl]", g_probeInits,\n'
                '                    m_template->m_name.c_str(), m_side, m_plugIns[0] != 0, m_plugIns[1] != 0,\n'
                '                    m_plugIns[2] != 0, m_plugIns[3] != 0, m_plugIns[4] != 0, m_plugIns[5] != 0);\n'
                '        std::fflush(stdout);\n'
                '    }\n')
    result[aiObject] = text

    # 2026-09-29 (chunk 07, WeaponObject): every AI weapon component's init - GenericFireArm::init and
    # enableCachedObject call it right after createWeapon attaches the component. The line records the
    # firearm and the two gates the recovered body tests first: a template already measured, or the AI
    # game still precaching, returns before WeaponTemplate::init. Printing only.
    weaponObject = ROOT / 'Code/BF2/AI/AIInterface/BFKnowledge/WeaponObject.cpp'
    text = includes(weaponObject.read_text(encoding='utf-8'), weaponObject.parent)
    text = prepend(text, '#include <cstdio>\n#include <cstdlib>\n'
                         'namespace { unsigned g_probeWeaponInits; }\n')
    text = once(text,
                'void WeaponObject::init(world::IObject* object)\n'
                '{\n',
                'void WeaponObject::init(world::IObject* object)\n'
                '{\n'
                '    if (std::getenv("BF2_AIDLL_TEMPLATE_PROBE"))\n'
                '    {\n'
                '        ++g_probeWeaponInits;\n'
                '        std::printf("%s WeaponObject::init #%u firearm=%s templateMeasured=%d precaching=%d\\n",\n'
                '                    g_probeWeaponInits == 1 ? "[aidll-templates]" : "[aidll-tpl]",\n'
                '                    g_probeWeaponInits, object->getTemplate()->getName().c_str(),\n'
                '                    (int)m_template->m_initialized, (int)game()->getIsPrecaching());\n'
                '        std::fflush(stdout);\n'
                '    }\n')
    result[weaponObject] = text

    return result


def build(manifest, out, vcvars):
    folder = out / 'runtime'
    folder.mkdir(parents=True, exist_ok=True)
    for name in ('runtime.exe', 'runtime.pdb', 'runtime.ilk', 'runtime.lib', 'runtime.exp'):
        (folder / name).unlink(missing_ok=True)
    (folder / 'bootmain.cpp').write_text(boot(), encoding='utf-8')
    sources = [folder / 'bootmain.cpp']

    entries = manifest.read_text().splitlines()
    for original, source in instrumented().items():
        generated = folder / ('observed_' + original.name)
        generated.write_text(source, encoding='utf-8')
        sources.append(generated)
        matches = [row for row in entries
                   if re.search(r'[/\\]\d+_' + re.escape(original.stem) + r'\.obj"?$', row)]
        if len(matches) > 1:
            # Several images' sources share a basename. Match the compiler's own source-path
            # record inside the object, keeping the other member. tier2/run.py does the same.
            paths = (str(original).lower().encode(), original.as_posix().lower().encode())
            matches = [row for row in matches
                       if any(p in Path(row.strip('"')).read_bytes().lower() for p in paths)]
        if len(matches) != 1:
            raise RuntimeError('Ambiguous engine object: ' + str(original))
        entries.remove(matches[0])
    (folder / 'engine_objects.rsp').write_text('\n'.join(entries) + '\n')

    flags = ('/nologo /c /MD /Z7 /std:c++14 /permissive /EHsc /DDEDICATED=1 /FI"'
             + str(ROOT / 'Code/BF2/Core/BuildConfig.h') + '"')
    for include in ('Include', 'PC'):
        flags += ' /I"' + str(ROOT / 'Code/BF2/External/python-embedded/py-src-2.3.4' / include) + '"'
    # ZIPINC, as build.cmd passes it: ModuleRegistry.cpp reaches IO/File/ZipArchive.h, which
    # includes the vendored minizip's unzip.h.
    for include in ('Code/BF2/External/zlib-1.2.3', 'Code/BF2/External/zlib-1.2.3/contrib/minizip'):
        flags += ' /I"' + str(ROOT / include) + '"'

    commands = ['@echo off', 'setlocal', 'call "' + str(vcvars) + '" -arch=x86 -no_logo >nul 2>&1']
    objects = []
    for i, source in enumerate(sources):
        obj = folder / f'test_{i}.obj'
        objects.append('"' + str(obj) + '"')
        commands += ['cl ' + flags + ' /Fo"' + str(obj) + '" "' + str(source) + '"',
                     'if errorlevel 1 exit /b 1']
    pyroot = ROOT / 'Code/BF2/External/python-embedded/OBJ/python/release'
    commands += ['link /nologo /FORCE /DEBUG /INCREMENTAL:NO /PDBPAGESIZE:16384 '
                 '/NODEFAULTLIB:python23.lib /subsystem:console '
                 '/OUT:"' + str(folder / 'runtime.exe') + '" /PDB:"' + str(folder / 'runtime.pdb') + '" '
                 '/MAP:"' + str(folder / 'runtime.map') + '" /IMPLIB:"' + str(folder / 'runtime.lib') + '" '
                 + ' '.join(objects) + ' @"' + str(folder / 'engine_objects.rsp') + '" '
                 '"' + str(pyroot / 'dice_py.lib') + '" '
                 'advapi32.lib user32.lib shell32.lib Ws2_32.lib dbghelp.lib iphlpapi.lib',
                 'if not exist "' + str(folder / 'runtime.exe') + '" exit /b 1', 'exit /b 0']
    script = folder / 'build.cmd'
    script.write_text('\r\n'.join(commands) + '\r\n', encoding='ascii', newline='')
    with (folder / 'build.log').open('w') as log:
        subprocess.run(['cmd', '/c', str(script)], stdout=log, stderr=subprocess.STDOUT, check=True)
    shutil.copy2(pyroot / 'dice_py.dll', folder / 'dice_py.dll')
    return folder / 'runtime.exe'


def free_port(port, address):
    if port == 16567:
        raise ValueError('Port 16567 belongs to the owner')
    if port == 16986:
        raise ValueError('Port 16986 belongs to the tier2 suite')
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_EXCLUSIVEADDRUSE, 1)
        sock.bind((address, port))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-out', type=Path, required=True,
                        help='a completed isolated engine build directory (its engine_objects.rsp)')
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--dll', type=Path,
                        default=ROOT / 'Code/Build/bin_aidll/AIDLL_w32ded.dll')
    parser.add_argument('--game-dir', type=Path, default=Path(r'D:\B2BF Launcher\Battlefield2'))
    parser.add_argument('--vcvars', type=Path, default=None)
    parser.add_argument('--level', default='dalian_plant')
    parser.add_argument('--game-mode', default='gpm_coop')
    parser.add_argument('--frames', type=int, default=30)
    parser.add_argument('--port', type=int, default=16988)
    parser.add_argument('--query-port', type=int, default=29888)
    parser.add_argument('--timeout', type=int, default=600)
    parser.add_argument('--probe-adapters', action='store_true',
                        help="chunk 01's gate: call BFConsole::createSingletons and read the roster")
    parser.add_argument('--probe-squads', action='store_true',
                        help='read engine AI player/squad state after the first game-logic update')
    parser.add_argument('--probe-kits', action='store_true',
                        help='read actual kit types/strengths and stored AI spawn metadata')
    parser.add_argument('--probe-templates', action='store_true',
                        help="chunk 03B's gate: count the AI templates and plug-ins the scripts made")
    args = parser.parse_args()

    if args.vcvars is None:
        args.vcvars = default_vcvars()
    args.out = args.out.resolve()
    args.out.mkdir(parents=True, exist_ok=True)
    manifest = args.build_out.resolve() / 'engine_objects.rsp'
    if not manifest.is_file():
        raise ValueError('Completed isolated build manifest required: ' + str(manifest))
    if args.out == args.build_out.resolve():
        raise ValueError('Use a separate regression output directory')

    dll = args.dll.resolve()
    if not dll.is_file():
        raise ValueError('Run Code\\Build\\aidll.cmd first - no DLL at ' + str(dll))

    # LoadLibraryBf does LoadLibraryA(".\\" + name) against the CURRENT DIRECTORY, which is the game
    # folder. A relative path back into the build output is what keeps the DLL out of the game
    # directory. A backslash in the name also stops Windows searching the DLL path, so the only
    # image that can ever be loaded is this one.
    relative = os.path.relpath(dll, args.game_dir)
    if len(relative) > 100:
        # LoadLibraryBf copies the name into char[128] with strncpy and no explicit terminator - a
        # shipped defect this project preserves. Stay well clear of it.
        raise ValueError('AIDLL path too long for the shipped 128-byte name buffer: ' + relative)

    config = args.out / 'ServerSettings.con'
    config.write_text('sv.internet 0\nsv.ranked 0\nsv.punkBuster 0\nsv.voipEnabled 0\n'
                      'sv.serverIP "127.0.0.1"\nsv.serverPort ' + str(args.port) + '\n'
                      'sv.interfaceIP "127.0.0.2"\nsv.gameSpyPort ' + str(args.query_port) + '\n'
                      'sv.allowNATNegotiation 0\nsv.autoRecord 0\nsv.useGlobalRank 0\n'
                      'sv.maxPlayers 64\nsv.numPlayersNeededToStart 2\nsv.startDelay 15\n'
                      'sv.autoBalanceTeam 0\n'
                      'SettingsManager.stringSet AIDLL "' + relative + '"\n')

    # ---------------------------------------------------------------------------------------------
    # A PRIVATE MAP ROTATION, and it is not a convenience - without it the requested level is
    # REFUSED by recovered retail code.
    #
    # MapList::checkIfValidEntry enforces a rule this harness spent three passes falling foul of:
    # on a STOCK mod, every entry must use the SAME game mode as the rotation's first entry. The
    # shipped mods/bf2/settings/maplist.con holds exactly one line - wake_island_2007 gpm_cq 64 - so
    # `+loadLevel dalian_plant +gameMode gpm_coop` was rejected outright and the run silently loaded
    # WAKE ISLAND on conquest while this script printed 'dalian_plant gpm_coop'. wake_island_2007
    # ships no ai/AI.ai, so no level ever ran `ai.init` and no AIMain was ever built.
    #
    # The rule is skipped when the rotation is EMPTY, so a private file whose only entry is already
    # the coop one is accepted. `+mapList <path>` is a recovered command-line option (BF2Engine's
    # case 9, 0x81340DC); the reference game directory is not touched.
    maplist = args.out / 'maplist.con'
    maplist.write_text('mapList.append ' + args.level + ' ' + args.game_mode + ' 16\n')

    print('BUILD runtime', flush=True)
    exe = build(manifest, args.out, args.vcvars)

    free_port(args.port, '0.0.0.0')
    free_port(args.query_port, '0.0.0.0')

    env = {k: v for k, v in os.environ.items() if not k.upper().startswith('BF2_')}
    env.update(BF2_FRAMES=str(args.frames), BF2_QUIET='1', BF2_PORT=str(args.port),
               BF2_SERVER_SCREEN='0', BF2_LISTENER_PROBE='1')
    if args.probe_squads:
        env['BF2_AIDLL_SQUAD_PROBE'] = '1'
    if args.probe_kits:
        env['BF2_AIDLL_KIT_PROBE'] = '1'
    if args.probe_templates:
        env['BF2_AIDLL_TEMPLATE_PROBE'] = '1'
    gate = list(GATE)
    if args.probe_adapters:
        env['BF2_AIDLL_ADAPTER_PROBE'] = '1'
        # (!) THE ADAPTER PROBE REPLACES THE CHUNK 00 GATE RATHER THAN EXTENDING IT, and the reason
        # is a real finding rather than a harness detail.
        #
        # The probe runs inside initAIConnection, so `BFConsole::createSingletons` builds the sixteen
        # adapters EARLY - and one of them, AIEventListener, subscribes itself to five event
        # categories in its init(). From that instant the engine's very next dispatched event lands
        # in `AIEventListener::handleEvent`, which is a 3,680-byte body nobody has read. So the run
        # stops there, BEFORE loadPath ever reaches aiSingleton->init(), and the chunk 00 lines
        # cannot appear.
        #
        # ** That is not an artifact of running createSingletons early. ** It is what chunk 02 will
        # meet the moment BF2AI::init's first wiring pass calls the courier for real: handleEvent is
        # on the critical path as soon as the listener exists. Recorded here so the next agent does
        # not rediscover it by surprise.
        gate = [('[adapters] createSingletons returned',
                 'BFConsole::createSingletons ran to completion'),
                ('[adapters] roster filled = 13 of 16',
                 'thirteen of the sixteen roster entries are live'),
                ('[FRONTIER] dice::hfe::ai::AIEventListener::handleEvent reached',
                 'the subscribed listener took the next engine event')]
    command = [str(exe), '+modPath', 'mods/bf2', '+loadLevel', args.level,
               '+gameMode', args.game_mode, '+maxPlayers', '64', '+port', str(args.port),
               '+noStatusMonitor', '1', '+config', str(config),
               '+mapList', str(maplist)]

    logpath = args.out / 'coop_load.log'
    print('RUN', args.level, args.game_mode, 'port', args.port, '->', relative, flush=True)
    with logpath.open('w') as log:
        try:
            run = subprocess.run(command, cwd=args.game_dir, env=env, stdout=log,
                                 stderr=subprocess.STDOUT, timeout=args.timeout)
            code = run.returncode
        except subprocess.TimeoutExpired:
            code = 124
    output = logpath.read_text(errors='replace')

    reached = []
    for needle, claim in gate:
        if needle in output:
            reached.append(claim)
        else:
            break

    # The frontier line is the SUCCESS condition for chunk 00, and abort() is how it stops - so a
    # non-zero exit code here is the expected shape, not a failure. bootmain's fault handler turns
    # the SIGABRT into a symbolised stack and exits 8; that stack is the useful artifact.
    passed = len(reached) == len(gate)
    result = dict(exit=code, passed=passed, reached=reached,
                  stopped_before=None if passed else gate[len(reached)][1],
                  level=args.level, gameMode=args.game_mode, port=args.port,
                  aidll=relative, log=str(logpath), squadProbe=args.probe_squads, kitProbe=args.probe_kits,
                  templateProbe=args.probe_templates)
    (args.out / 'results.json').write_text(json.dumps(result, indent=2) + '\n')

    for claim in reached:
        print('  reached:', claim, flush=True)
    if not passed:
        print('  STOPPED BEFORE:', gate[len(reached)][1], flush=True)
    # [TEMP] Show the frontier and fault lines on a PASS too. Now that the gate ends in
    # success rather than in a named abort, the run still stops further down the boot - in
    # `gameLogic->load` - and that line is the next work item. Printing it only on failure
    # would hide exactly what a passing run is now supposed to report.
    for line in output.splitlines():
        if ('[FRONTIER]' in line or 'FAULT' in line or 'Exception' in line or '<- NULL' in line
                or '[aidll-squads]' in line or '[aidll-kits]' in line or '[aidll-spawn]' in line
                or '[aidll-templates]' in line):
            print('  ' + line.strip(), flush=True)
    print('PASS' if passed else 'FAIL', 'exit', code, flush=True)
    return 0 if passed else 1


if __name__ == '__main__':
    sys.exit(main())
