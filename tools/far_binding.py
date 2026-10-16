"""Finite root external CALL FAR equations and grouped MZ obligations.

The callee entry and an original relocated CALL outside the owner establish
its alias independently. Raw OMF and oracle bytes are never rewritten.
"""
import struct

from binding import (declarations, fixes, instructions, ground as ground_data,
                     data_context, data_equations)
from common import require, sha
from disasm import _capstone


def ground_symbol(symbol, target, oracle):
    from match import get_target
    require(symbol.get("kind") == "far-code-entry" and symbol.get("space") == "root",
            "Far binding refuses overlay/vector/unknown code aliases")
    require(type(symbol["segment"]) is int and type(symbol["offset"]) is int and
            0 <= symbol["segment"] <= 65535 and 0 <= symbol["offset"] <= 65535,
            "Far code alias must be an exact 16:16 pair")
    callee, space, raw = get_target(symbol["entry_target_id"], oracle)
    require(callee.get("entry_kind") == "direct-code" and callee["space"] == "root" and
            (callee["segment"], callee["offset"]) == (symbol["segment"], symbol["offset"]) and
            callee["sha256"] == symbol["entry_sha256"],
            "Far callee is not the frozen reviewed direct-code entry (vectors refused)")
    start = space.position(target["segment"], target["offset"])
    end = start+target["size"]
    entry = space.position(callee["segment"], callee["offset"])
    require(not (entry < end and start < entry+len(raw)), "Far callee entry overlaps candidate")
    require(symbol.get("witnesses"), "Far callee has no independent original CALL witness")
    receipts = []
    for spec in symbol["witnesses"]:
        require(spec["space"] == "root", "Far entry witness must be resident")
        at, witness = spec["image_offset"], bytes.fromhex(spec["bytes_hex"])
        require(type(at) is int and len(witness) == 5 and witness[0] == 0x9a and
                0 <= at <= space.size-5 and space.data[at:at+5] == witness,
                "Far entry witness is not an original CALL FAR")
        require(not (at < end and start < at+5), "Far entry witness overlaps candidate")
        offset, segment = struct.unpack_from("<HH", witness, 1)
        require((segment, offset) == (symbol["segment"], symbol["offset"]),
                "Independent far CALL does not ground the exact callee alias")
        relocs = [r for r in space.relocations if at-1 <= r["image_offset"] < at+5]
        require(len(relocs) == 1 and relocs[0]["image_offset"] == at+3 and
                relocs[0]["index"] == spec["relocation_index"] and
                spec["file_offset"] == space.file_offset+at,
                "Independent far CALL relocation/file provenance differs")
        receipts.append({"image_offset": at, "relocation_index": relocs[0]["index"]})
    return {"entry_target_id": callee["id"], "segment": symbol["segment"],
            "offset": symbol["offset"], "witnesses": receipts, "callee_owned_bytes": 0}


ORDER_DEBT = ["Between-target-symbol relocation group order and combined output-segment placement require structural linker proof"]


def relocation_order(relocs, fields, frozen_indices):
    """Project RTLink's output-segment/symbol buckets onto one whole CODE owner.

    `fields` retains OMF FIXUPP traversal, never site-sorted. Distinct external
    symbols remain distinct even when their grounded 16:16 addresses coincide.
    No between-symbol bucket permutation is inferred from the local object.
    """
    original_sites = [r["image_offset"] for r in relocs]
    sites = [f["site"] for f in fields]
    require(len(original_sites) == len(sites) == len(set(original_sites)) == len(set(sites)) and
            set(original_sites) == set(sites),
            "Original MZ relocation site set differs from far segment fields")
    indices = [r["index"] for r in relocs]
    require(all(type(i) is int and i >= 0 for i in indices) and len(indices) == len(set(indices)),
            "Duplicate/invalid original relocation index")
    require(indices == frozen_indices, "Frozen original relocation indices changed")
    by_site = {f["site"]: f for f in fields}
    groups = []
    names = list(dict.fromkeys(f["symbol"] for f in fields))
    original_names = [by_site[r["image_offset"]]["symbol"] for r in relocs]
    runs = [n for i, n in enumerate(original_names) if i == 0 or original_names[i-1] != n]
    require(len(runs) == len(set(runs)), "Original MZ target-symbol groups are interleaved")
    for name in names:
        generated = [f for f in fields if f["symbol"] == name]
        observed = [r for r in relocs if by_site[r["image_offset"]]["symbol"] == name]
        require([f["site"] for f in generated] == [r["image_offset"] for r in observed],
                "Original MZ within-symbol FIXUPP order differs: " + name)
        groups.append({"symbol": name,
                       "fixup_operand_offsets": [f["offset"] for f in generated],
                       "segment_word_sites": [f["site"] for f in generated],
                       "original_relocation_indices": [r["index"] for r in observed],
                       "within_group_order": "EXACT"})
    qualification = "EXACT" if original_sites == sites else "GROUPED"
    index_by_site = {r["image_offset"]: r["index"] for r in relocs}
    return {"relocation_order": qualification, "relocation_groups": groups,
            "generated_relocation_indices": [index_by_site[f["site"]] for f in fields],
            "relocation_order_debt": list(ORDER_DEBT) if qualification == "GROUPED" else [],
            "relocation_order_scope": "Exact site bijection and contiguous target-symbol groups within one CODE contribution; exact FIXUPP order inside each group; no natural between-group/output-segment placement proof"}


def compare(module, code_name, actual, expected, binding, target, space, oracle):
    require(binding["schema"] == 1 and binding["mode"] == "external-far-call-v1" and
            binding["target_id"] == target["id"] and binding["target_sha256"] == sha(oracle.data),
            "Unsupported/foreign far-call binding")
    require(target["space"] == space.name == "root", "Far-call v1 requires a root owner; overlays refused")
    require(len(actual) == len(expected) == target["size"], "Far component extent differs; trimming forbidden")
    obligations = fixes(module)
    require(obligations and obligations == binding["fixups"], "Ordered far symbolic fixup obligations changed")
    require(declarations(module) == binding["declarations"], "Far object declarations changed")
    symbols = binding["symbols"]
    require(set(symbols) == {f["target"].get("name") for f in obligations},
            "Missing or unused grounded far/data symbol")
    far_symbols = {n: s for n, s in symbols.items() if s.get("kind") == "far-code-entry"}
    data_symbols = {n: s for n, s in symbols.items() if s.get("kind") == "near-data-alias"}
    require(far_symbols and len(far_symbols)+len(data_symbols) == len(symbols),
            "Far binding refuses overlay/vector/unknown code aliases")
    grounding = {n: ground_symbol(s, target, oracle) for n, s in far_symbols.items()}
    if data_symbols:
        data = binding.get("data", {})
        require(data.get("mode") in ("external-dgroup-offset16-v1", "external-dgroup-word-pair-offset16-v2"),
                "Composed far binding needs an explicit resident DGROUP mode")
        ground_data({**binding, **data, "symbols": data_symbols}, target, oracle)
    else:
        require("data" not in binding, "Unused DGROUP proof in far binding")
    rows = instructions(actual)
    cs, _ = _capstone()
    occupied, equations, differences, sites, allowed_calls = set(), [], [], [], set()
    for fix in obligations:
        at, ref = fix["segment_offset"], fix["target"]
        if ref.get("name") in data_symbols:
            continue
        require(fix["segment"] == code_name and fix["loc_type"] == 3 and fix["field_width"] == 4 and
                not fix["self_relative"] and ref["kind"] == "external" and ref["method"] == 6 and
                fix["displacement"] is None and fix["frame"] == {"kind": "target_frame", "target": ref},
                "Unsupported far CALL fixup equation/frame/displacement")
        require(type(at) is int and 1 <= at <= len(actual)-4 and
                not occupied.intersection(range(at, at+4)), "Overlapping/out-of-range far fields")
        require(any(i.address == at-1 and i.size == 5 and bytes(i.bytes) == b"\x9a"+actual[at:at+4]
                    for i in rows), "Far fixup is not a complete unprefixed 9A CALL operand")
        require(actual[at:at+4] == bytes(4), "Only zero encoded far addends are proven")
        symbol = far_symbols[ref["name"]]
        value = (symbol["offset"], symbol["segment"])
        observed = struct.unpack_from("<HH", expected, at)
        if value != observed:
            differences.append(at)
        equations.append({"kind": "far-call", "offset": at, "symbol": ref["name"],
                          "encoded_addend": 0, "explicit_displacement": None,
                          "linked_offset": value[0], "linked_segment": value[1],
                          "original_offset": observed[0], "original_segment": observed[1],
                          "equal": value == observed})
        occupied.update(range(at, at+4))
        allowed_calls.add(at-1)
        sites.append({"site": space.position(target["segment"], target["offset"])+at+2,
                      "offset": at, "symbol": ref["name"]})
    # No literal or indirect call may bypass the supported symbolic obligations.
    for insn in rows:
        require(not set(insn.groups).intersection((cs.CS_GRP_CALL, cs.CS_GRP_INT, cs.CS_GRP_IRET)) or
                insn.address in allowed_calls, "Unproved call/interrupt in far-bound code")
        require(insn.mnemonic not in ("ljmp", "jmpf"), "Unproved far jump/vector transfer")
    if not data_symbols:
        # Reuse the symbolic-address audit even when no DS fixups were emitted.
        data_equations([], {}, code_name, actual, expected, rows,
                       "external-dgroup-offset16-v1", occupied)
    if data_symbols:
        data_context(module, rows, allowed_calls)
        data_fixes = [f for f in obligations if f["target"].get("name") in data_symbols]
        data_rows, data_diff = data_equations(data_fixes, data_symbols, code_name, actual, expected,
                                            rows, binding["data"]["mode"], occupied)
        equations.extend({"kind": "dgroup-offset16", **e} for e in data_rows)
        differences.extend(data_diff)
    start = space.position(target["segment"], target["offset"])
    relocs = [r for r in space.relocations if start-1 <= r["image_offset"] < start+len(expected)]
    order_proof = relocation_order(relocs, sites, binding["relocation_indices"])
    indices = [r["index"] for r in relocs]
    # Retain complete FIXUPP order even when data/far equations were evaluated separately.
    order = {f["segment_offset"]: i for i, f in enumerate(obligations)}
    equations.sort(key=lambda e: order[e["offset"]])
    ordinary = [i for i, (a, b) in enumerate(zip(actual, expected)) if i not in occupied and a != b]
    field_count = len(differences)
    differences.extend(ordinary)
    return {**order_proof, "exact": not differences, "first_difference": min(differences, default=None),
            "mismatch_count": len(differences), "mismatch_basis": "ordinary bytes plus unequal field equations",
            "ordinary_mismatch_count": len(ordinary), "field_mismatch_count": field_count,
            "fixups": len(obligations), "relocations": len(relocs), "equations": equations,
            "covered_relocation_indices": indices if not differences else [],
            "original_relocations": relocs,
            "ordinary_bytes_compared": len(actual)-len(occupied), "symbol_grounding": grounding,
            "proof_scope": "Complete root component bytes, external zero-addend CALL FAR equations to independently witnessed reviewed direct entries, exact original MZ segment-word coverage with within-target-symbol FIXUPP order (between-group order is structural debt); optional resident DGROUP equations hold for every DS; raw OMF preserved, original symbols/TU/runtime DS/natural placement/overlay vectors/whole link unproved"}
