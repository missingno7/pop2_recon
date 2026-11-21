"""Original source language evidence: only genuine original assembly may be matched as ASM.

The FM Towns build's debug data names each module's source file (`XLATTXT4.ASM`,
`PTOOL.C`, ...). evidence/modules.json pairs DOS entries with FM functions of those
modules. An ASM claim whose target is paired with an FM `.C` module is a MASM
transcription of compiled C and is refused, except for owners listed as recorded
debt in evidence/asm-origin-audit.json (to be replaced by C reconstructions).
"""
from common import ROOT, read_json, require

EXTRA_REVIEW = {}  # verify-only: a worker's own asm-review.json entries, never used for publication


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
    """Accept an ASM claim only with evidence that the DOS original was assembly:
    - an FM Towns `.ASM` pair; or
    - an FM `.C` pair recorded in evidence/asm-origin-audit.json `dos_native_asm` (the DOS bytes
      carry non-MSC markers: the DOS build hand-assembled it) or listed as C-origin replacement debt;
    - no FM pair: listed with verdict `asm` in evidence/asm-origin-review.json (byte-level review).
    Anything else must be reconstructed as C."""
    pairs = _pairs()
    audit = read_json(ROOT / "evidence/asm-origin-audit.json")
    debt, native = set(audit["c_origin_debt"]), set(audit.get("dos_native_asm", {}))
    review = dict(read_json(ROOT / "evidence/asm-origin-review.json").get("targets", {}), **EXTRA_REVIEW)
    for tid in target_ids:
        module, source = pairs.get(tid, (None, ""))
        if source.upper().endswith(".ASM"):
            continue
        if source.upper().endswith(".C"):
            require(tid in native or tid in debt,
                    f"{tid} belongs to original C module {source}: reconstruct it as C, not MASM")
            continue
        require(review.get(tid, {}).get("verdict") == "asm",
                f"{tid} has no reviewed evidence that its original source is assembly "
                "(evidence/asm-origin-review.json): reconstruct it as C or record the review")
