"""[TEMP] Navmesh loader fixture driver (chunk 04, 2026-09-29).

extract OUT : copy AIPathFinding/*.qtr|*.clb out of every stock level's server.zip (READ-ONLY on the
              game directory) into OUT/levels/<Level>/AIPathFinding/, write the synthetic cases into
              OUT/synthetic/<case>/AIPathFinding/, and record the production sources' hashes.
run EXE OUT : drive the production QuadTreeManager::loadQuadTrees over every case and compare the
              whole trace with the one this script derives from its own parse of each file - every
              raw read size in order, the final stream position against the file size, the island
              state and the IDPManager::deleteMesh count.
"""
import hashlib
import json
import shutil
import struct
import subprocess
import sys
import zipfile
from pathlib import Path

CODE = Path(__file__).resolve().parents[2]
LEVELS = Path(r"D:\B2BF Launcher\Battlefield2\mods\bf2\Levels")
SOURCES = [
    "BF2/AI/AIDLL/AI/AIPathFinding/NavMesh/QuadTreeManager.cpp",
    "BF2/AI/AIDLL/AI/AIPathFinding/NavMesh/QuadTree.cpp",
    "BF2/AI/AIDLL/AI/AIPathFinding/NavMesh/QuadTreeNode.cpp",
    "BF2/AI/AIDLL/AI/AIPathFinding/Cluster/ClusterGraph.cpp",
    "BF2/AI/AIDLL/AI/AIPathFinding/Material/MaterialManager.cpp",
    "BF2/Math/Mat4Inverse.cpp",
    "BF2/Math/Plane3.cpp",
    "BF2/Math/Vec3.cpp",
]
QTR_ISLAND_ELEMENTS = (24, 20, 4, 12, 4, 4)      # nodes, polygons, node list, vertices, indices, neighbours
CLB_ELEMENTS = ((16, False), (4, False), (20, True), (4, True))   # (size, allocated only if non-zero)
ABORT = 3                                        # UCRT abort() with the report fault disabled
BREAKPOINT = (0x80000003, 0x80000003 - (1 << 32))


def i32(data, offset):
    return struct.unpack_from("<i", data, offset)[0]


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def qtr_island_reads(data, offset):
    """The reads QuadTree::readFromFile makes for one version-3 island starting at `offset`."""
    reads = [4, 8, 8, 4]
    offset += 24
    for element in QTR_ISLAND_ELEMENTS:
        count = u32(data, offset)
        reads += [4, element * count]
        offset += 4 + element * count
    return reads, offset


def clb_reads(data):
    reads = [4]
    if i32(data, 0) != 5:
        return reads, 4
    offset = 4
    for element, optional in CLB_ELEMENTS:
        count = u32(data, offset)
        reads.append(4)
        offset += 4
        if count or not optional:
            reads.append(element * count)
            offset += element * count
    return reads, offset


def joined(reads):
    return ",".join(str(r) for r in reads)


def write_if_changed(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_bytes() != data:
        path.write_bytes(data)


def extract(out):
    out.mkdir(parents=True, exist_ok=True)
    stock = []
    for level in sorted(p.name for p in LEVELS.iterdir() if (p / "server.zip").is_file()):
        with zipfile.ZipFile(LEVELS / level / "server.zip") as archive:
            names = {n.split("/")[-1]: n for n in archive.namelist()
                     if n.lower().startswith("aipathfinding/") and n.lower().endswith((".qtr", ".clb"))}
            for short, full in names.items():
                write_if_changed(out / "levels" / level / "AIPathFinding" / short, archive.read(full))
            if names:
                stock.append(level)
    dalian = out / "levels" / "Dalian_plant" / "AIPathFinding"
    qtr = (dalian / "Infantry.qtr").read_bytes()
    clb = (dalian / "Infantry.clb").read_bytes()
    island = qtr[4:]                                      # the one island after the count
    synthetic = {
        "qtr_v2": (struct.pack("<ii", 1, 2), clb),
        "qtr_v4": (struct.pack("<ii", 1, 4), clb),
        "islands_2": (struct.pack("<i", 2) + island, clb),
        "islands_negative": (struct.pack("<i", -1) + island, clb),
        "clb_v4": (qtr, struct.pack("<i", 4) + clb[4:]),
    }
    for case, (qtr_bytes, clb_bytes) in synthetic.items():
        base = out / "synthetic" / case / "AIPathFinding"
        write_if_changed(base / "Infantry.qtr", qtr_bytes)
        write_if_changed(base / "Infantry.clb", clb_bytes)
    (out / "synthetic" / "no_navmesh").mkdir(parents=True, exist_ok=True)
    evidence = {"levels_with_navmesh": stock, "sources": {}}
    for rel in SOURCES:
        evidence["sources"][rel] = hashlib.sha256((CODE / rel).read_bytes()).hexdigest()
    (out / "sources.json").write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"extracted {len(stock)} levels with navmesh files; {len(synthetic) + 1} synthetic cases")


def opened(name, size):
    return f"open <level>/AIPathFinding/{name} mode=2 access=0 flags=0 size={size}"


def open_failed(name):
    return f"open-fail <level>/AIPathFinding/{name} mode=2 access=0 flags=0"


def full_load(level_dir, name):
    """The trace of one successful one-island load of <name>.qtr / .clb."""
    qtr = (level_dir / "AIPathFinding" / (name + ".qtr")).read_bytes()
    clb = (level_dir / "AIPathFinding" / (name + ".clb")).read_bytes()
    if i32(qtr, 0) != 1 or i32(qtr, 4) != 3:
        raise RuntimeError(("not a one-island version-3 file", level_dir, name))
    q_reads, q_end = qtr_island_reads(qtr, 4)
    c_reads, c_end = clb_reads(clb)
    return [
        "getLevelPath",
        opened(name + ".qtr", len(qtr)),
        opened(name + ".clb", len(clb)),
        f"close <level>/AIPathFinding/{name}.clb position={c_end} size={len(clb)} reads={joined(c_reads)}",
        f"close <level>/AIPathFinding/{name}.qtr position={q_end} size={len(qtr)} reads=4,{joined(q_reads)}",
    ], (q_end == len(qtr) and c_end == len(clb))


def cases(out):
    """Yield (label, mode, level_dir, map, expected lines, expected exit codes)."""
    levels = out / "levels"
    for level_dir in sorted(p for p in levels.iterdir() if p.is_dir()):
        for name in ("Infantry", "Vehicle"):
            qtr = level_dir / "AIPathFinding" / (name + ".qtr")
            clb = level_dir / "AIPathFinding" / (name + ".clb")
            if qtr.exists() and clb.exists():
                trace, exact = full_load(level_dir, name)
                if not exact:
                    raise RuntimeError(("the driver's own parse does not reach the end", level_dir, name))
                yield (f"{level_dir.name}/{name}", "load", level_dir, name, trace + [
                    "state load islands=1 [tree,graph] deleteMesh=1",
                    "state destroyQuadTrees islands=0 deleteMesh=2",
                    "PASS load"], (0,))
            elif qtr.exists():
                # Mashtuur_City: a .qtr with no .clb beside it - the load stops at the second open,
                # before the island count is read, and the .qtr stream is released by ~File.
                yield (f"{level_dir.name}/{name} (no .clb)", "load", level_dir, name, [
                    "getLevelPath",
                    opened(name + ".qtr", qtr.stat().st_size),
                    open_failed(name + ".clb"),
                    f"release <level>/AIPathFinding/{name}.qtr",
                    "state load islands=0 deleteMesh=0",
                    "state destroyQuadTrees islands=0 deleteMesh=0",
                    "PASS load"], (0,))

    dalian = levels / "Dalian_plant"
    trace, _ = full_load(dalian, "Infantry")
    yield ("reload Dalian_plant/Infantry", "reload", dalian, "Infantry",
           trace + ["state load islands=1 [tree,graph] deleteMesh=1"] + trace +
           ["state reload islands=1 [tree,graph] deleteMesh=3", "PASS reload"], (0,))
    trace, _ = full_load(dalian, "Vehicle")
    yield ("destroyQuadTree Dalian_plant/Vehicle", "destroyone", dalian, "Vehicle", trace + [
        "state load islands=1 [tree,graph] deleteMesh=1",
        "state destroyQuadTree islands=1 [null,graph] deleteMesh=2",
        "state destroyQuadTrees islands=0 deleteMesh=2",
        "PASS destroyone"], (0,))

    synth = out / "synthetic"
    yield ("no navmesh directory", "load", synth / "no_navmesh", "Infantry", [
        "getLevelPath",
        open_failed("Infantry.qtr"),
        "state load islands=0 deleteMesh=0",
        "state destroyQuadTrees islands=0 deleteMesh=0",
        "PASS load"], (0,))

    clb_size = (synth / "qtr_v2" / "AIPathFinding" / "Infantry.clb").stat().st_size
    for case, line, text in (
            ("qtr_v2", 132, "Quad-tree file is not up to date. Reload GTS-data end save new "
                            "quadtree-files for this level."),
            ("qtr_v4", 137, "Quad-tree file is newer than your build. Update to latest build.")):
        yield (f"version gate {case}", "load", synth / case, "Infantry", [
            "getLevelPath",
            opened("Infantry.qtr", 8),
            opened("Infantry.clb", clb_size),
            f"DEBUG type=6 file=AI/AIDLL/AI/AIPathFinding/NavMesh/QuadTree.cpp line={line} module=NavMesh",
            "TEXT " + text,
            "qtr position=8 size=8"], (ABORT,))

    qtr_size = (synth / "islands_2" / "AIPathFinding" / "Infantry.qtr").stat().st_size
    yield ("one-island trap (count 2)", "load", synth / "islands_2", "Infantry", [
        "getLevelPath",
        opened("Infantry.qtr", qtr_size),
        opened("Infantry.clb", clb_size)], BREAKPOINT)

    # A NEGATIVE count passes the signed trap and then drives the loop through an unsigned compare:
    # island 0 loads in full, and island 1's version read runs off the end of the file (0 here, the
    # [CHOSEN] initial value; stack garbage in retail) into the line-132 error.
    yield ("negative island count", "load", synth / "islands_negative", "Infantry", [
        "getLevelPath",
        opened("Infantry.qtr", qtr_size),
        opened("Infantry.clb", clb_size),
        "DEBUG type=6 file=AI/AIDLL/AI/AIPathFinding/NavMesh/QuadTree.cpp line=132 module=NavMesh",
        "TEXT Quad-tree file is not up to date. Reload GTS-data end save new quadtree-files for this level.",
        f"qtr position={qtr_size} size={qtr_size}"], (ABORT,))

    qtr = (dalian / "AIPathFinding" / "Infantry.qtr").read_bytes()
    q_reads, q_end = qtr_island_reads(qtr, 4)
    yield ("silent .clb version refusal", "load", synth / "clb_v4", "Infantry", [
        "getLevelPath",
        opened("Infantry.qtr", len(qtr)),
        opened("Infantry.clb", clb_size),
        f"close <level>/AIPathFinding/Infantry.clb position=4 size={clb_size} reads=4",
        f"close <level>/AIPathFinding/Infantry.qtr position={q_end} size={len(qtr)} reads=4,{joined(q_reads)}",
        "state load islands=1 [tree,graph] deleteMesh=1",
        "state destroyQuadTrees islands=0 deleteMesh=2",
        "PASS load"], (0,))


def run(exe, out):
    log = []
    count = 0
    stock = 0
    for label, mode, level_dir, name, expected, codes in cases(out):
        result = subprocess.run([str(exe), mode, str(level_dir), name], capture_output=True,
                                text=True, timeout=120)
        lines = result.stdout.splitlines()
        if result.returncode not in codes or lines != expected or result.stderr.strip():
            detail = [f"FAIL {label}: exit {result.returncode} (expected {codes})"]
            for i in range(max(len(lines), len(expected))):
                got = lines[i] if i < len(lines) else "<missing>"
                want = expected[i] if i < len(expected) else "<none>"
                if got != want:
                    detail.append(f"  line {i}: got  {got}")
                    detail.append(f"  line {i}: want {want}")
            if result.stderr.strip():
                detail.append("  stderr: " + result.stderr.strip())
            raise SystemExit("\n".join(detail))
        count += 1
        if mode == "load" and "(no .clb)" not in label and "/" in label and not label.startswith(
                ("version", "one-island", "negative", "silent", "no navmesh")):
            stock += 1
        log.append(f"ok {label} (exit {result.returncode})")
    log.append(f"PASS navmesh loader: {count} cases, {stock} stock .qtr/.clb pairs read to the last byte")
    (exe.parent / "runtime.log").write_text("\n".join(log) + "\n", encoding="utf-8")
    print("\n".join(log))


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "extract":
        extract(Path(sys.argv[2]).resolve())
    elif len(sys.argv) == 4 and sys.argv[1] == "run":
        run(Path(sys.argv[2]).resolve(), Path(sys.argv[3]).resolve())
    else:
        raise SystemExit("Usage: navmesh_loader_runtime.py extract OUT | run EXE OUT")
