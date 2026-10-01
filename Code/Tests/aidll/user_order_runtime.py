"""[TEMP] Selector fixture driver: each expected abort is isolated and time limited."""
import pathlib
import subprocess
import sys

exe = pathlib.Path(sys.argv[1]).resolve()
ordinary = ["empty", "guard_inactive", "guard_wrong_side", "guard_waiting", "guard_occupied",
            "patrol_inactive", "patrol_wrong_side", "patrol_full", "patrol_overfull", "exhausted", "ownership"]
stops = {"guard_selected": "GuardPosition", "guard_priority": "GuardPosition",
         "guard_after_rejected": "GuardPosition", "patrol_selected": "PatrolPath",
         "patrol_after_rejected": "PatrolPath"}
for mode in ordinary + list(stops):
    result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=10)
    output = result.stdout + result.stderr
    (exe.parent / (mode + ".log")).write_text(output, encoding="utf-8")
    if mode in stops:
        expected = "[FRONTIER] dice::hfe::ai::UserOrderManager::recruitBot (" + stops[mode] + "::recruit) reached"
        assert result.returncode == 3, (mode, result.returncode, output)
        assert expected in result.stderr, (mode, output)
        assert result.stderr.count("[FRONTIER]") == 1, (mode, output)
        assert "UNEXPECTED_RETURN" not in output, (mode, output)
    else:
        assert result.returncode == 0 and "PASS " + mode in output, (mode, result.returncode, output)
        assert "[FRONTIER]" not in output, (mode, output)
    print("PASS", mode)
print("PASS UserOrderManager: 10 exhausted/refusal cases, 5 isolated frontier stops, owning destruction")
