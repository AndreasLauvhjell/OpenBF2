"""[TEMP] Extract the current production BotInfo constructor verbatim for isolation.

MSVC resolves references in unused BotManager functions before /OPT:REF; this
fixture therefore compiles only the exact constructor text, against its real header.
No production statement or declaration is substituted.
"""
from pathlib import Path
import re
import sys

code = Path(__file__).resolve().parents[2]
source_path = code / "BF2/AI/AIDLL/AI/Main/BotManager.cpp"
header_path = source_path.with_suffix(".h")
source = source_path.read_text(encoding="utf-8")
matches = re.findall(r"^BotManager::BotInfo::BotInfo\(Bot\* bot\)[\s\S]*?^}", source, re.MULTILINE)
if len(matches) != 1:
    raise SystemExit("Expected exactly one production BotInfo constructor")
out = Path(sys.argv[1])
out.write_text(
    '// [TEMP] Generated verbatim from the current production BotManager.cpp.\n'
    '#include "' + header_path.as_posix() + '"\n'
    'namespace dice { namespace hfe { namespace ai {\n'
    + matches[0] + '\n} } }\n', encoding="utf-8"
)
