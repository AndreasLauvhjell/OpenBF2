"""[TEMP] Generate, attest, and run the strict x86 MapContainer::init fixture."""

from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess
import sys


CODE = Path(__file__).resolve().parents[2]
BF2 = CODE / "BF2"
AI = BF2 / "AI/AIDLL/AI"
FIXTURE = Path(__file__).resolve()


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def strip_comments(text):
    return re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)


def extract_init(source):
    pattern = (
        r"^void AIPathfinding::MapContainer::init\(\)\s*"
        r"\{[\s\S]*?^\}"
    )
    found = re.findall(pattern, source, re.M)
    if len(found) != 1:
        raise RuntimeError(("exact MapContainer::init extraction", len(found)))
    return found[0]


def map_container_virtuals():
    path = AI / "Main/AIPathfinding.h"
    source = strip_comments(path.read_text(encoding="utf-8"))
    found = re.findall(
        r"^    class MapContainer[^\{]*\{(.*?)^    };", source, re.M | re.S
    )
    if len(found) != 1:
        raise RuntimeError(("MapContainer declaration", len(found)))
    for signature in re.findall(r"virtual\s+([^;{}]+);", found[0]):
        signature = " ".join(signature.split())
        method = re.search(r"(~?\w+)\s*\(", signature).group(1)
        if not method.startswith("~"):
            yield method, signature


def settings_observer():
    path = AI / "Main/AISettingsInterfaces.h"
    source = strip_comments(path.read_text(encoding="utf-8"))
    body = re.search(r"^class IAISettings\s*\{(.*?)^};", source, re.M | re.S).group(1)
    lines = [
        "// [TEMP] Typed IAISettings observer. Only two recovered calls may run.",
        "class InitSettings : public IAISettings {",
        "public:",
        "    std::map<std::string, int> m_vehicleIds;",
        "    std::vector<int> m_clusterTypes;",
        "    mutable std::vector<std::string> m_calls;",
        "    mutable int m_clusterGetterCalls = 0;",
        "    int getVehicle(const std::string&) const override;",
        "    const std::vector<int>& getClusterCostTypes() const override;",
    ]
    for signature in re.findall(r"virtual\s+([^;{}]+);", body):
        signature = re.sub(r"\s*=\s*0$", "", " ".join(signature.split()))
        method = re.search(r"(~?\w+)\s*\(", signature).group(1)
        if method.startswith("~"):
            continue
        if signature in (
            "int getVehicle(const std::string&) const",
            "const std::vector<int>& getClusterCostTypes() const",
        ):
            continue
        lines.append(
            "    " + signature + " override { unexpected(\"unused settings " + method + "\"); }"
        )
    lines.append("};")
    return "\n".join(lines) + "\n"


def generate(output):
    output.mkdir(parents=True, exist_ok=True)
    source_path = AI / "Main/AIPathfinding.cpp"
    source = source_path.read_text(encoding="utf-8")
    body = extract_init(source)

    actual = (
        "// [TEMP] Exact production extract; no substitutions.\n"
        "#include \"" + FIXTURE.with_suffix(".h").as_posix() + "\"\n"
        "namespace dice { namespace hfe { namespace ai {\n"
        + body
        + "\n} } }\n"
    )
    (output / "actual.cpp").write_text(actual, encoding="utf-8")
    (output / "source.json").write_text(
        json.dumps(
            {
                "source": source_path.as_posix(),
                "source_sha256": sha256(source_path),
                "body_sha256": hashlib.sha256(body.encode("utf-8")).hexdigest(),
                "body": body,
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )

    unused = ["// [TEMP] Unexercised MapContainer virtuals stop visibly."]
    for method, signature in map_container_virtuals():
        definition = re.sub(
            r"\b" + re.escape(method) + r"(?=\s*\()",
            "AIPathfinding::MapContainer::" + method,
            signature,
            count=1,
        )
        unused.append(definition + " { unexpected(\"unused MapContainer::" + method + "\"); }")
    (output / "unused.inl").write_text("\n".join(unused) + "\n", encoding="utf-8")
    (output / "settings.inl").write_text(settings_observer(), encoding="utf-8")

    clean = strip_comments(body)
    checks = {
        "typed_string_vehicle_call": "settings->getVehicle(*i)" in clean,
        "unique_insert": "vehicleIndices.insert(std::make_pair(" in clean,
        "counter_advances_after_insert": clean.find("vehicleIndices.insert") < clean.find("++vehicleIndex"),
        "single_cluster_getter_call": clean.count("settings->getClusterCostTypes()") == 1,
        "resize_to_settings_length": "m_clusterCostTypes.resize(settingsClusterCostTypes.size())" in clean,
        "unchecked_find_dereference": bool(
            re.search(r"vehicleIndices\.find\([^\n;]+\)->second", clean)
        ),
        "no_map_find_end_check": "vehicleIndices.end()" not in clean,
        "no_fallback_subscript": "vehicleIndices[" not in clean,
    }
    if not all(checks.values()):
        raise RuntimeError(("static source checks", checks))
    (output / "static_checks.json").write_text(
        json.dumps(
            {
                "checks": checks,
                "missing_key_case": "paired-evidence/static-only; deliberately not executed",
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )

    cases = [
        "ordered_translation",
        "duplicate_first_wins",
        "minus_one_key",
        "grow_overwrite",
        "shrink_overwrite",
        "repeat_after_reorder",
        "empty_empty",
    ]
    (output / "cases.json").write_text(json.dumps(cases, indent=2) + "\n", encoding="utf-8")

    evidence_dir = output.parent / "init_evidence"
    evidence = {}
    if evidence_dir.is_dir():
        for path in sorted(evidence_dir.iterdir()):
            if path.is_file():
                evidence[path.name] = sha256(path)
    (output / "evidence_context.json").write_text(
        json.dumps(evidence, indent=2) + "\n", encoding="utf-8"
    )

    fixture_paths = [FIXTURE.with_suffix(suffix) for suffix in (".h", ".cpp", ".py", ".cmd")]
    (output / "fixture_sources.json").write_text(
        json.dumps({path.name: sha256(path) for path in fixture_paths}, indent=2) + "\n",
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
    (output / "binary_identity.json").write_text(
        json.dumps(
            {
                "path": executable.as_posix(),
                "sha256": sha256(executable),
                "pe_machine": "0x%04X (IMAGE_FILE_MACHINE_I386)" % machine,
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )

    cases = json.loads((output / "cases.json").read_text(encoding="utf-8"))
    summary = []
    total_checks = 0
    for case in cases:
        result = subprocess.run(
            [str(executable), case], capture_output=True, text=True, timeout=30
        )
        combined = result.stdout + result.stderr
        (output / (case + ".log")).write_text(combined, encoding="utf-8")
        match = re.search(
            r"^PASS " + re.escape(case) + r" ([1-9][0-9]*) checks\n\Z",
            combined,
            re.M,
        )
        if result.returncode != 0 or not match or "FAIL" in combined or "UNEXPECTED" in combined:
            raise RuntimeError((case, result.returncode, combined))
        total_checks += int(match.group(1))
        summary.append(match.group(0).strip())
        print(summary[-1])

    static_data = json.loads((output / "static_checks.json").read_text(encoding="utf-8"))
    static_count = len(static_data["checks"])
    summary.append(
        "PASS missing_key_static %d checks (undefined-behavior path not executed)" % static_count
    )
    summary.append(
        "PASS mapcontainer init: %d runtime cases / %d runtime checks / %d static checks"
        % (len(cases), total_checks, static_count)
    )
    runtime_log = "\n".join(summary) + "\n"
    (output / "runtime.log").write_text(runtime_log, encoding="utf-8")
    print(summary[-2])
    print(summary[-1])

    source_data = json.loads((output / "source.json").read_text(encoding="utf-8"))
    readme = f"""# MapContainer::init focused validation

This `[TEMP]` fixture compiles and executes the exact `AIPathfinding::MapContainer::init` body
extracted from `{source_data['source']}`. The production source SHA-256 is
`{source_data['source_sha256']}` and the extracted body SHA-256 is
`{source_data['body_sha256']}`.

## Result

```text
{runtime_log.rstrip()}
```

`build.log` records a successful MSVC x86 compile and strict `/MACHINE:X86` link with no `/FORCE`;
`binary_identity.json` independently reads the produced PE header as `IMAGE_FILE_MACHINE_I386`.
The runtime cases cover ordered translation and call order, duplicate-ID first-wins behavior with
counter advance, the ordinary `-1` key, growth and complete overwrite, shrink and prefix overwrite,
repeat after name reorder, and empty/empty while still calling `getClusterCostTypes` once.

`static_checks.json` records the missing-key contract without invoking it: the extracted production
body dereferences `find(...)->second` without an `end()` check or fallback insertion. Executing a
missing local key would enter retail undefined behavior, so this fixture deliberately omits it.

## Limits

The `IAISettings` observer is typed and all unexercised slots terminate visibly. Fixture-only
constructors initialize the real `MapContainer` type's STL members and leave its unrelated pointers
null; this does not test the full MapContainer/QuadTree lifetime, a server boot, a bot frame, or the
original retail STL binary. Defined behavior is exercised against the current production body with
the repository's required MSVC x86 C++14 toolchain.

Run from the repository root with:

```powershell
cmd /c Code\\Tests\\aidll\\mapcontainer_init_runtime.cmd
```
"""
    (output / "README.md").write_text(readme, encoding="utf-8")


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "generate":
        generate(Path(sys.argv[2]).resolve())
    elif len(sys.argv) == 4 and sys.argv[1] == "run":
        run(Path(sys.argv[2]).resolve(), Path(sys.argv[3]).resolve())
    else:
        raise SystemExit("generate OUT | run EXE OUT")
