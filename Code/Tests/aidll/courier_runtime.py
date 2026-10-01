"""[TEMP] Generate typed IAIMain observations and two real-header include-order gates."""
from pathlib import Path
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]


def generate(out):
    generated = ["// [TEMP] Real typed receiver; all calls except the observed notification fail.",
                 "namespace dice { namespace hfe { namespace ai {"]
    specs = [
        ("IAIMain", "AI/AIDLL/AI/Main/AIMain.h", "ObservedMain",
         {"commanderOrderAccepted": "received.push_back(a);"}),
        ("IAIGame", "AI/AIInterface/BFKnowledge/BFGame.h", "ObservedGame",
         {"getMaxNumberOfSquads": 'trace("capacity"); return 4;'}),
        ("IAIPlayerFactory", "AI/AIInterface/BFKnowledge/AIPlayerFactory.h", "ObservedFactory",
         {"getPlayerFromId": 'trace("factory:" + std::to_string(id)); return expectedPlayer;'}),
        # 2026-09-29: the destroyable map-part events. The real declarations leave their parameters
        # unnamed, so these four supply a named declaration; `override` still checks each signature.
        # ⚠ `::trace` - IAIPathfinding has three `trace` virtuals of its own, which hide the fixture's.
        ("IAIPathfinding", "AI/AIDLL/AI/Main/IAIPathfinding.h", "ObservedPathfinding",
         {"initializeDestroyableMapPart": (
             "void initializeDestroyableMapPart(int& id, bool walkable, const Vec3& minCorner, "
             "const Vec3& maxCorner, const Mat4& transformation)",
             '::trace("mapPart:" + std::to_string(id) + (walkable ? ":walkable" : ":blocked")); '
             'require(&minCorner == expectedMin && &maxCorner == expectedMax && '
             '&transformation == expectedTransformation, "corners and transformation by reference"); '
             'id = 4242;'),
          "destroyMapPart": ("void destroyMapPart(int id)", '::trace("destroy:" + std::to_string(id));'),
          "restoreMapPart": ("void restoreMapPart(int id)", '::trace("restore:" + std::to_string(id));'),
          "resetOriginalMaterialOfDestroyableMapPart": (
             "void resetOriginalMaterialOfDestroyableMapPart(int id)",
             '::trace("original:" + std::to_string(id));')})]
    for base, relative, name, implementations in specs:
        source = (CODE / "BF2" / relative).read_text(encoding="utf-8")
        matches = re.findall(r"^class " + base + r"\s*\{(.*?)^};", source, re.M | re.S)
        if len(matches) != 1:
            raise RuntimeError("Expected one real " + base + " declaration")
        body = re.sub(r"//[^\n]*", "", matches[0])
        generated.append("class " + name + " : public " + base + " { public:")
        found = set()
        for declaration in re.findall(r"virtual\s+([^;]+);", body):
            declaration = " ".join(declaration.split())
            method = re.search(r"(~?\w+)\s*\(", declaration).group(1)
            if method.startswith("~"):
                continue
            declaration = re.sub(r"\s*=\s*0$", "", declaration)
            implementation = implementations.get(method, 'unexpected("' + base + "::" + method + '");')
            if method in implementations:
                found.add(method)
            if isinstance(implementation, tuple):
                declaration, implementation = implementation
            generated.append("    " + declaration + " override { " + implementation + " }")
        if found != set(implementations):
            raise RuntimeError("Missing expected override for " + base)
        generated.append("};")
    generated.append("} } }")
    out.mkdir(parents=True, exist_ok=True)
    (out / "courier_runtime_mocks.inl").write_text("\n".join(generated), encoding="utf-8")
    engine = (CODE / "BF2/AI/AIInterface/IAIEventCourier.h").as_posix()
    dll = (CODE / "BF2/AI/AIDLL/AI/Main/AIEventCourier.h").as_posix()
    for name, paths in (("Engine", (engine, dll)), ("Dll", (dll, engine))):
        content = '// [TEMP] Both real headers in opposite include orders.\n'
        content += ''.join('#include "' + path + '"\n' for path in paths)
        content += ('void courierFrom' + name + '(dice::hfe::ai::IAIEventCourier* courier, int id)\n'
                    '{ courier->event_commanderOrderAccepted(id); }\n')
        content += ('void leaderFrom' + name + '(dice::hfe::ai::IAIEventCourier* courier, int side, int squad)\n'
                    '{ courier->eventNewSquadLeader(side, squad); }\n')
        content += ('void joinedFrom' + name + '(dice::hfe::ai::IAIEventCourier* courier, int player, int squad, int side)\n'
                    '{ courier->event_joinedSquad(player, squad, side); }\n')
        # 2026-09-29: the eight destroyable map-part and bridge events, through both include orders.
        content += ('void mapPartFrom' + name + '(dice::hfe::ai::IAIEventCourier* courier, int& id, bool walkable,'
                    ' const dice::hfe::Vec3& minCorner, const dice::hfe::Vec3& maxCorner,'
                    ' const dice::hfe::Mat4& transformation)\n'
                    '{ courier->event_retrievePathfindingMapId(id, walkable, minCorner, maxCorner, transformation); }\n')
        content += ('void mapPartDestroyedFrom' + name + '(dice::hfe::ai::IAIEventCourier* courier, int id,'
                    ' dice::hfe::ai::IAIObject* object)\n'
                    '{ courier->event_pathfindingMapIdDestroyed(id, object); }\n')
        content += ('void mapPartRestoredFrom' + name + '(dice::hfe::ai::IAIEventCourier* courier, int id)\n'
                    '{ courier->event_pathfindingMapIdRestored(id); }\n')
        content += ('void mapPartOriginalFrom' + name + '(dice::hfe::ai::IAIEventCourier* courier, int id,'
                    ' dice::hfe::ai::IAIObject* object)\n'
                    '{ courier->event_pathfindingMapIdRestoreOriginalMaterials(id, object); }\n')
        content += ('void bridgeEventFrom' + name + '(dice::hfe::ai::IAIEventCourier* courier, int which,'
                    ' dice::hfe::ai::IAIObject* object, int value)\n'
                    '{ if (which == 0) courier->event_bridgeCreated(object);'
                    ' else if (which == 1) courier->event_bridgeDestroyed(object, value);'
                    ' else if (which == 2) courier->event_bridgeRepaired(object);'
                    ' else courier->event_bridgeRemoved(object); }\n')
        (out / ("include_" + name.lower() + ".cpp")).write_text(content, encoding="utf-8")


# 2026-09-29: a negative map-part id is retail's FATAL Debug(Error) at these two lines. Each mode
# installs a printing Error callback (the real setDebugCallback), the callback returns and ~Debug
# aborts; the process must die there, before any pathfinder or bridge call and before returning.
FATAL_MODES = {"fatal-destroyed": "AIEventCourier.cpp(508): Object destroyed that has unknown "
                                  "connection to the the pathfinding.",
               "fatal-original": "AIEventCourier.cpp(532): Object destroyed that has unknown "
                                 "connection to the the pathfinding."}


def run(exe):
    result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=15)
    output = result.stdout + result.stderr
    (exe.parent / "runtime.log").write_text(output, encoding="utf-8")
    if result.returncode != 0 or "PASS courier/manager:" not in output or "UNEXPECTED" in output or "[FRONTIER]" in output:
        raise RuntimeError((result.returncode, output))
    print(output, end="")
    for mode, text in FATAL_MODES.items():
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + ".log")).write_text(output, encoding="utf-8")
        if (result.returncode == 0 or text not in output or "REACHED-AFTER" in output
                or "UNEXPECTED" in output or "CALLED" in output or "[FRONTIER]" in output):
            raise RuntimeError((mode, result.returncode, output))
        print("PASS " + mode + ": fatal Debug(Error) before any forward, exit " + str(result.returncode))


if __name__ == "__main__":
    if len(sys.argv) != 3 or sys.argv[1] not in ("generate", "run"):
        raise SystemExit("Usage: courier_runtime.py generate OUTPUT_DIR | run EXE")
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == "generate" else run(target)
