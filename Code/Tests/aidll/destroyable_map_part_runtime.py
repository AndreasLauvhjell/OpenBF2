"""[TEMP] Attest and run the strict x86 destroyable-map-part lifecycle fixture."""

from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess
import sys


CODE = Path(__file__).resolve().parents[2]
BF2 = CODE / "BF2"
FIXTURE = Path(__file__).resolve()
SOURCES = [
    BF2 / "AI/AIDLL/AI/AIPathFinding/NavMesh/QuadTree.cpp",
    BF2 / "AI/AIDLL/AI/AIPathFinding/NavMesh/QuadTree.h",
    BF2 / "AI/AIDLL/AI/AIPathFinding/NavMesh/QuadTreeManager.cpp",
    BF2 / "AI/AIDLL/AI/AIPathFinding/NavMesh/QuadTreeManager.h",
    BF2 / "AI/AIDLL/AI/AIPathFinding/Cluster/Cluster.h",
    BF2 / "AI/AIDLL/AI/AIPathFinding/Cluster/ClusterGraph.h",
    BF2 / "Math/Mat4Inverse.cpp",
]


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def body(source, qualified_name):
    start = source.find(qualified_name)
    if start < 0:
        raise RuntimeError(("missing production body", qualified_name))
    brace = source.find("{", start)
    if brace < 0:
        raise RuntimeError(("missing body brace", qualified_name))
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[brace : index + 1]
    raise RuntimeError(("unterminated production body", qualified_name))


def preflight(output):
    output.mkdir(parents=True, exist_ok=True)
    quad = SOURCES[0].read_text(encoding="utf-8")
    manager = SOURCES[2].read_text(encoding="utf-8")
    initializer = body(quad, "void QuadTree::initializeDestroyableMapPart")
    boundary = body(quad, "void QuadTree::getPolygonsWithinBoundary(")
    doubles = body(quad, "void QuadTree::getPolygonsWithinBoundaryDoubles")
    destroy = body(quad, "void QuadTree::destroyMapPart")
    restore = body(quad, "void QuadTree::restoreMapPart")
    reset_one = body(quad, "void QuadTree::resetOriginalMaterialOfDestroyableMapPart")
    reset_all = body(quad, "void QuadTree::resetDestroyableMapParts")
    manager_init = body(manager, "void QuadTreeManager::initializeDestroyableMapPart")

    checks = {
        "exact_zero_translation_test": all(
            f"transformation.m[{slot}] == 0.0f" in initializer for slot in (12, 13, 14)
        ),
        "ordinary_and_double_dispatch": (
            "getPolygonsWithinBoundaryDoubles" in initializer
            and "getPolygonsWithinBoundary(" in initializer
        ),
        "append_before_initial_restore": (
            initializer.find("m_destroyableMapParts.push_back(part)")
            < initializer.rfind("restoreMapPart(mapPartId, graph)")
        ),
        "material_rescan_starts_at_zero": bool(
            re.search(r"for\s*\(unsigned int i = 0; i < part->m_polygons.size\(\); \+\+i\)", initializer)
        ),
        "ordinary_output_not_cleared": ".clear(" not in boundary,
        "double_output_not_cleared": ".clear(" not in doubles,
        "first_qualifying_corner_then_break": (
            "polygons.push_back(polygonId);" in boundary and "break;" in boundary
        ),
        "destroy_only_top_level_empty_guard": (
            "if (!m_destroyableMapParts.empty())" in destroy
            and "mapPartId <" not in destroy
            and ".at(" not in destroy
        ),
        "restore_minus_one_only_id_guard": (
            "if (mapPartId != -1)" in restore and "mapPartId <" not in restore and ".at(" not in restore
        ),
        "reset_original_minus_one_only_id_guard": (
            "if (mapPartId != -1)" in reset_one and "mapPartId <" not in reset_one and ".at(" not in reset_one
        ),
        "reset_all_ascending_size_reread": bool(
            re.search(r"for\s*\(unsigned int mapPartId = 0; mapPartId < m_destroyableMapParts.size\(\); \+\+mapPartId\)", reset_all)
        ),
        "manager_pairs_tree_and_graph_and_passes_false": (
            ".first->initializeDestroyableMapPart" in manager_init
            and ".second, false" in manager_init
        ),
        "no_destroyable_frontier_left": all(
            "BF2_AI_FRONTIER" not in body(manager, name)
            for name in (
                "void QuadTreeManager::initializeDestroyableMapPart",
                "void QuadTreeManager::destroyMapPart",
                "void QuadTreeManager::restoreMapPart",
                "void QuadTreeManager::resetOriginalMaterialOfDestroyableMapPart",
                "void QuadTreeManager::resetDestroyableMapParts",
            )
        ),
    }
    if not all(checks.values()):
        raise RuntimeError(("production source contract", checks))

    fixture_sources = [FIXTURE.with_suffix(suffix) for suffix in (".cpp", ".cmd", ".py")]
    (output / "static_checks.json").write_text(
        json.dumps(checks, indent=2) + "\n", encoding="utf-8"
    )
    (output / "source_hashes.json").write_text(
        json.dumps(
            {
                "production": {str(path.relative_to(CODE)).replace("\\", "/"): sha256(path) for path in SOURCES},
                "fixture": {path.name: sha256(path) for path in fixture_sources},
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )


def run(executable, output):
    image = executable.read_bytes()
    if image[:2] != b"MZ":
        raise RuntimeError("fixture executable has no DOS header")
    pe_offset = struct.unpack_from("<I", image, 0x3C)[0]
    if image[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise RuntimeError("fixture executable has no PE signature")
    machine = struct.unpack_from("<H", image, pe_offset + 4)[0]
    if machine != 0x014C:
        raise RuntimeError(("fixture executable is not x86", hex(machine)))

    result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
    combined = result.stdout + result.stderr
    match = re.fullmatch(
        r"PASS destroyable_map_part_runtime ([1-9][0-9]*) checks across ([1-9][0-9]*) cases\n",
        combined,
    )
    if result.returncode != 0 or not match or "FAIL" in combined or "UNEXPECTED" in combined:
        raise RuntimeError(("runtime", result.returncode, combined))

    (output / "runtime.log").write_text(combined, encoding="utf-8")
    (output / "binary_identity.json").write_text(
        json.dumps(
            {
                "path": executable.as_posix(),
                "sha256": sha256(executable),
                "pe_machine": "0x014C (IMAGE_FILE_MACHINE_I386)",
                "runtime_checks": int(match.group(1)),
                "runtime_cases": int(match.group(2)),
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )

    static_count = len(json.loads((output / "static_checks.json").read_text(encoding="utf-8")))
    readme = f"""# Destroyable map-part focused validation

This `[TEMP]` fixture compiled the real production `QuadTree.cpp`, `QuadTreeManager.cpp`, and
`Mat4Inverse.cpp` bodies with the AIDLL image defines, then linked them into an MSVC x86 executable
without `/FORCE`.

## Result

```text
{combined.rstrip()}
PASS {static_count} production-shape checks
PASS strict x86 link without /FORCE
```

The runtime covers ordinary and double boundary selection, inverse translation and rotation,
inclusive faces, first-hit behavior, float materialization from double vertices, one appended
record per call and island (including empty selections), exact-zero and nonzero translation arms,
original-material selection, both initially-walkable transition families, first-polygon cluster
updates, null/empty suppression, destroy idempotence, `-1` no-ops, reset-all order, manager island
order, and exact tree/graph pairing. The source checks separately preserve the helpers'
append-without-clear contract and invalid non-sentinel id fault shapes without executing undefined
behavior.

`binary_identity.json` independently records `IMAGE_FILE_MACHINE_I386`; `source_hashes.json`
attests the exact production and fixture inputs. This is a focused in-memory fixture, not a server,
socket, bot frame, map load, or retail-client run.
"""
    (output / "README.md").write_text(readme, encoding="utf-8")
    print(combined, end="")
    print(f"PASS {static_count} production-shape checks")
    print("PASS strict x86 link without FORCE")


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "preflight":
        preflight(Path(sys.argv[2]).resolve())
    elif len(sys.argv) == 4 and sys.argv[1] == "run":
        run(Path(sys.argv[2]).resolve(), Path(sys.argv[3]).resolve())
    else:
        raise SystemExit("preflight OUT | run EXE OUT")
