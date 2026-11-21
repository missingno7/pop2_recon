"""Original source language evidence: only genuine original assembly may be matched as ASM.

The FM Towns build's debug data names each module's source file (`XLATTXT4.ASM`,
`PTOOL.C`, ...). evidence/modules.json pairs DOS entries with FM functions of those
modules. An ASM claim whose target is paired with an FM `.C` module is a MASM
transcription of compiled C and is refused, except for owners listed as recorded
debt in evidence/asm-origin-audit.json (to be replaced by C reconstructions).
"""
from common import ROOT, read_json, require


def _pairs():
    fm = read_json(ROOT / "evidence/fmtowns/symbols.json")
    source = {m["name"]: m.get("source_file") or "" for m in fm["modules"]}
    mods = read_json(ROOT / "evidence/modules.json")
    pairs = {}

    def walk(node):
        for value in node.values():
            if isinstance(value, dict) and "function_pairs" in value:
                for p in value["function_pairs"]:
                    pairs[p["dos_address"]] = (p.get("fmtowns_module"), source.get(p.get("fmtowns_module"), ""))
            elif isinstance(value, dict):
                walk(value)
    walk(mods)
    return pairs


def require_genuine_asm(target_ids):
    """Refuse ASM claims whose original module is C, unless recorded as replacement debt."""
    pairs = _pairs()
    debt = set(read_json(ROOT / "evidence/asm-origin-audit.json")["c_origin_debt"])
    for tid in target_ids:
        module, source = pairs.get(tid, (None, ""))
        require(not source.upper().endswith(".C") or tid in debt,
                f"{tid} belongs to original C module {source}: reconstruct it as C, not MASM")
