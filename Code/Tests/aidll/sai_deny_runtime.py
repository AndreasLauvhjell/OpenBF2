"""[TEMP] Strict isolated sweep: exact production body, real headers, chosen lifetimes.

This does not exercise retail SAI construction, the engine commander bodies, or
the unsafe nonempty cancellation branch. No production statements are rewritten.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
SAI = "AI/AIDLL/AI/AIStrategic/SAI"


def methods(name, relative):
    source = (CODE / "BF2" / relative).read_text(encoding="utf-8")
    found = re.findall(r"^class " + name + r"(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};", source, re.M | re.S)
    if len(found) != 1:
        raise RuntimeError("Expected one declaration of " + name)
    body = re.sub(r"//[^\n]*", "", found[0])
    for text in re.findall(r"virtual\s+([^;]+);", body):
        text = re.sub(r"\s*=\s*0$", "", " ".join(text.split()))
        method = re.search(r"(~?\w+)\s*\(", text).group(1)
        if not method.startswith("~"):
            yield method, text


def generate(out):
    source_path = CODE / "BF2" / (SAI + ".cpp")
    source = source_path.read_text(encoding="utf-8")
    bodies = re.findall(r"^void SAI::updateRequestDenyTimers\(\)\s*\{[\s\S]*?^}", source, re.M)
    if len(bodies) != 1 or "BF2_AI_FRONTIER" in bodies[0]:
        raise RuntimeError("Expected one recovered production deny-sweep body")
    out.mkdir(parents=True, exist_ok=True)
    shared = Path(__file__).with_suffix(".h").as_posix()
    (out / "sai_deny_actual.cpp").write_text(
        '// [TEMP] Verbatim current production helper; no statement substitution.\n'
        '#include "' + shared + '"\n'
        'namespace dice { namespace hfe { namespace ai {\n' + bodies[0] + '\n} } }\n', encoding="utf-8")
    (out / "sai_deny_source.json").write_text(json.dumps({
        "source": source_path.as_posix(),
        "source_sha256": hashlib.sha256(source.encode()).hexdigest(),
        "body_sha256": hashlib.sha256(bodies[0].encode()).hexdigest(),
        "body": bodies[0],
    }, indent=2), encoding="utf-8")
    generated = ['// [TEMP] Typed observers; every unused virtual aborts.',
                 'namespace dice { namespace hfe { namespace ai {']
    specs = [
        ("IBFTimer", "AI/AIInterface/BFKnowledge/BFTimer.h", "ObservedTimer", '',
         {"getGameTime": 'return observeTime();'}),
        ("IBFCommander", "AI/AIInterface/BFKnowledge/BFCommander.h", "ObservedCommander", 'int identity = 0;',
         {name: 'return observeCommand(identity, ' + str(tag) + ', a, b);'
          for tag, name in enumerate(('denySupplyRequest', 'denyArtilleryRequest',
                                     'ArtilleryUnavailable', 'denyUavRequest', 'denyVehicleDropRequest'))})]
    for base, relative, name, fields, impl in specs:
        generated.append('class ' + name + ' : public ' + base + ' { public: ' + fields)
        found = set()
        for method, declaration in methods(base, relative):
            if method in impl:
                found.add(method)
            body = impl.get(method, 'unexpected("' + base + '::' + method + '");')
            generated.append('    ' + declaration + ' override { ' + body + ' }')
        if found != set(impl):
            raise RuntimeError("Missing observed methods in " + base)
        generated.append('};')
    for method, declaration in methods("SAI", SAI + ".h"):
        if method == "updateRequestDenyTimers":
            continue
        signature = re.sub(r"\b" + method + r"(?=\s*\()", 'SAI::' + method, declaration, count=1)
        generated.append(signature + ' { unexpected("SAI::' + method + '"); }')
    generated.append('} } }')
    (out / "sai_deny_mocks.inl").write_text('\n'.join(generated), encoding="utf-8")


def run(exe):
    modes = ['empty_false', 'empty_true', 'expiry_boundaries', 'nan_clock',
             'all_tags', 'unknown_tags', 'fresh_commander', 'payload_reload']
    for mode in modes:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding="utf-8")
        if result.returncode != 0 or ('PASS ' + mode + '\n') not in output or 'UNEXPECTED' in output or '[FRONTIER]' in output:
            raise RuntimeError((mode, result.returncode, output))
        print('PASS', mode)
    print('PASS deny sweep: 8 strict x86 cases; exact extracted body, chosen collaborators')


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: sai_deny_runtime.py generate OUTPUT_DIR | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
