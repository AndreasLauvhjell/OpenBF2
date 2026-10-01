"""[TEMP] Generate typed abort-by-default collaborators and run isolated squad cases.

SquadMember.cpp and SquadFormation.cpp compile whole, unchanged. Mock overrides are
generated from the current real interface declarations, never from raw vtable slots.
The interface constructors/registration, coordinator and game/player/bot/clock are
test collaborators. No engine, server, spawned object or AI tick is exercised.
"""
from pathlib import Path
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]


def generate(out):
    specs = [
        ("IAIPlayer", "AI/AIInterface/BFKnowledge/AIPlayer.h", "TestPlayer",
         "int id = 123; bool alive = false, down = false; IAIObject* vehicle = nullptr;",
         {"getId": 'trace("id"); return id;',
          "getIsAlive": 'trace("alive"); return alive;',
          "isManDown": 'trace("down"); return down;',
          "getCurrentVehicle": 'trace("vehicle"); observeVehicleRead(); return vehicle;'}),
        ("Bot", "AI/AIDLL/AI/AIBot/BotMain.h", "TestBot", "SquadMember* member = nullptr;",
         {"setSquadMember": 'trace("bind"); member = m; observeBinding(m);'}),
        ("IBotManager", "AI/AIDLL/AI/Main/IBotManager.h", "TestManager", "Bot* result = nullptr;",
         {"getBotFromPlayerId": 'trace("lookup"); require(playerId == 123, "lookup ID"); return result;'}),
        ("IAIGame", "AI/AIInterface/BFKnowledge/BFGame.h", "TestGame", "",
         {"getMaxNumberOfSquadMembers": 'trace("size"); return 3;'}),
        ("IBFTimer", "AI/AIInterface/BFKnowledge/BFTimer.h", "TestTimer", "double now = 10.0;",
         {"getGameTime": 'trace("time"); observeTimerRead(); return now;'}),
    ]
    generated = ["// [TEMP] Generated typed mocks. Every unused virtual aborts.",
                 "namespace dice { namespace hfe { namespace ai {"]
    for base, rel, name, fields, bodies in specs:
        source = (CODE / "BF2" / rel).read_text(encoding="utf-8")
        matches = re.findall(r"^class " + base + r"\s*\{(.*?)^};", source, re.M | re.S)
        if len(matches) != 1:
            raise RuntimeError("Expected one class definition: " + base)
        body = re.sub(r"//[^\n]*", "", matches[0])
        declarations = re.findall(r"virtual\s+([^;]+);", body)
        generated.append("class " + name + " : public " + base + " { public:\n" + fields)
        used = set()
        for declaration in declarations:
            declaration = " ".join(declaration.split())
            method = re.search(r"(~?\w+)\s*\(", declaration).group(1)
            if method.startswith("~"):
                continue
            declaration = re.sub(r"\s*=\s*0$", "", declaration)
            implementation = bodies.get(method, 'unexpected("' + name + "::" + method + '");')
            if method in bodies:
                used.add(method)
            generated.append("    " + declaration + " override { " + implementation + " }")
        if used != set(bodies):
            raise RuntimeError("Missing expected override: " + str(set(bodies) - used))
        generated.append("};")
    generated.append("} } }")
    out.mkdir(parents=True, exist_ok=True)
    (out / "squad_runtime_mocks.inl").write_text("\n".join(generated), encoding="utf-8")


def run(exe):
    ordinary = ["constructor_bot", "constructor_human", "dead", "man_down", "no_vehicle",
                "position_null", "formation_constructor", "join_first_free", "join_full",
                "join_duplicate", "movement_empty", "movement_distance_boundary",
                "movement_deadline_boundary", "movement_success", "movement_nan_distance",
                "movement_nan_time", "movement_forced", "reset_no_position", "reset_empty"]
    stops = {
        "allowed_frontier": "SquadMember::isAllowedInFormation (IAIObject information handle)",
        "position_frontier": "SquadMember::updateMemberPos (IAIObject information handle)",
        "reset_frontier": "SquadFormation::updateSlotZeroPos (getSlotWorldPos)",
    }
    for mode in ordinary + list(stops):
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=10)
        output = result.stdout + result.stderr
        (exe.parent / (mode + ".log")).write_text(output, encoding="utf-8")
        if mode in stops:
            expected = "[FRONTIER] dice::hfe::ai::" + stops[mode] + " reached"
            if result.returncode != 3:
                raise RuntimeError((mode, result.returncode, output))
            if expected not in result.stderr or result.stderr.count("[FRONTIER]") != 1:
                raise RuntimeError((mode, output))
            if "UNEXPECTED" in output:
                raise RuntimeError((mode, output))
        else:
            if result.returncode != 0 or "PASS " + mode not in output:
                raise RuntimeError((mode, result.returncode, output))
            if "[FRONTIER]" in output:
                raise RuntimeError((mode, output))
        print("PASS", mode)
    print("PASS SquadMember/SquadFormation: 19 ordinary cases, 3 isolated production frontier stops; strict link")


if __name__ == "__main__":
    if len(sys.argv) != 3 or sys.argv[1] not in ("generate", "run"):
        raise SystemExit("Usage: squad_runtime.py generate OUTPUT_DIR | run EXE")
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == "generate" else run(target)
