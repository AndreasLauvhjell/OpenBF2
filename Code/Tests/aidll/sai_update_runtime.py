"""[TEMP] Test verbatim production SAI bodies with real headers and chosen lifetimes.

The surrounding production translation unit and constructor are not executed.
Unobserved calls fail explicitly; no production statements are substituted.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
AI = "AI/AIDLL/AI/"


def methods(name, relative):
    source = (CODE / "BF2" / relative).read_text(encoding="utf-8")
    pattern = r"^class " + name + r"(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};"
    found = re.findall(pattern, source, re.M | re.S)
    if len(found) != 1:
        raise RuntimeError("Expected one declaration of " + name)
    body = re.sub(r"//[^\n]*", "", found[0])
    for text in re.findall(r"virtual\s+([^;]+);", body):
        text = re.sub(r"\s*=\s*0$", "", " ".join(text.split()))
        method = re.search(r"(~?\w+)\s*\(", text).group(1)
        if not method.startswith("~"):
            yield method, text


def generate(out):
    source_path = CODE / "BF2" / AI / "AIStrategic/SAI.cpp"
    source = source_path.read_text(encoding="utf-8")
    bodies = {}
    for name in ("update", "distributeArtillery"):
        found = re.findall(r"^void SAI::" + name + r"\([^\n]*\)\s*\{[\s\S]*?^}", source, re.M)
        if len(found) != 1 or 'BF2_AI_FRONTIER("dice::hfe::ai::SAI::' + name + '")' in found[0]:
            raise RuntimeError("Expected one recovered production SAI::" + name + " body")
        bodies[name] = found[0]
    out.mkdir(parents=True, exist_ok=True)
    shared = Path(__file__).with_suffix(".h").as_posix()
    (out / "sai_update_actual.cpp").write_text(
        '// [TEMP] Verbatim current production bodies; no statement substitution.\n'
        '#include "' + shared + '"\n'
        'namespace dice { namespace hfe { namespace ai {\n' + '\n\n'.join(bodies.values()) + '\n} } }\n', encoding="utf-8")
    (out / "sai_update_source.json").write_text(json.dumps({
        "source": source_path.as_posix(), "source_sha256": hashlib.sha256(source.encode()).hexdigest(),
        "bodies": {name: {"sha256": hashlib.sha256(body.encode()).hexdigest(), "body": body}
                   for name, body in bodies.items()},
    }, indent=2), encoding="utf-8")
    generated = ['// [TEMP] Typed observers from actual declarations; unused calls abort.',
                 'namespace dice { namespace hfe { namespace ai {']
    specs = [
        ("IBFTimer", "AI/AIInterface/BFKnowledge/BFTimer.h", "ObservedTimer", 'int identity = 0;', {
            "getTimeIncrease": 'return observeIncrease(identity);',
            "getUpdateTime": 'return observeUpdate(identity);',
            "getGameTime": 'return observeGame(identity);'}),
        ("IAISettings", AI + "Main/AISettingsInterfaces.h", "ObservedSettings", 'int identity = 0;',
         {"getSAIUpdateFrequency": 'return observeFrequency(identity);'})]
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
    for method, declaration in methods("SAI", AI + "AIStrategic/SAI.h"):
        if method in bodies:
            continue
        signature = re.sub(r"\b" + method + r"(?=\s*\()", 'SAI::' + method, declaration, count=1)
        generated.append(signature + ' { unexpected("SAI::' + method + '"); }')
    generated.append('} } }')
    (out / "sai_update_mocks.inl").write_text('\n'.join(generated), encoding="utf-8")


def run(exe):
    modes = ['state_' + str(state) for state in (0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 19)]
    modes += ['budget_equal', 'budget_nan', 'elapsed_nan', 'frequency_equal',
              'frequency_nan', 'game_nan', 'strategic_false', 'owned_false',
              'full_normal', 'full_suppressed', 'flag_reread', 'buffer_nonzero',
              'buffer_captured', 'cached_timer', 'settings_captured',
              'timestamp_after_game', 'bot_pointer_reload', 'bot_float_rounding',
              'bot_live_end']
    for mode in modes:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding="utf-8")
        if result.returncode != 0 or 'PASS ' + mode not in output or 'UNEXPECTED' in output or '[FRONTIER]' in output:
            raise RuntimeError((mode, result.returncode, output))
        print('PASS', mode)
    invalid = ['invalid_' + str(state) for state in (5, 16, 17, 18, -1, 20)]
    for mode in invalid:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding="utf-8")
        if result.returncode != 3 or 'EXPECTED DEBUG 6 1044 Unhadled case in SAI::Update().' not in output or 'AFTER INVALID' in output or 'UNEXPECTED' in output:
            raise RuntimeError((mode, result.returncode, output))
        print('PASS', mode, '(expected production Debug abort)')
    print('PASS SAI: ' + str(len(modes) + len(invalid)) + ' isolated strict x86 cases; exact production bodies, chosen collaborators')


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: sai_update_runtime.py generate OUTPUT_DIR | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
