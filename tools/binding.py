"""Narrow external DGROUP offset proof. Never edits objects or oracle bytes.

Every field is checked by a linker equation and every other byte by equality.
This is component proof only: symbol aliases, not original PUBLIC names; no
natural placement, TU identity, DS runtime trace, or complete link is claimed.
"""
import struct

from common import require, sha
from disasm import _capstone


def instructions(raw):
    cs, _ = _capstone()
    engine = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_16)
    engine.detail = True
    rows = list(engine.disasm(raw, 0))
    require(sum(i.size for i in rows) == len(raw), "Binding code contains undecoded bytes")
    return rows


def direct_operand(insn):
    """One unprefixed, direct default-DS memory operand with a 16-bit offset."""
    _, xc = _capstone()
    memory = [o for o in insn.operands if o.type == xc.X86_OP_MEM]
    require(len(memory) == 1 and not any(insn.prefix) and insn.disp_size == 2,
            "Binding needs an unprefixed direct DS operand")
    operand = memory[0]
    require(operand.mem.segment == operand.mem.base == operand.mem.index == 0,
            "Binding memory operand is not direct default-DS")
    return operand


def declarations(module):
    return {"segments": module.segment_defs, "groups": module.groups,
            "publics": module.publics, "externals": module.externals}


def fixes(module):
    return [e for r in module.fixups for e in r["resolved"] if e["kind"] == "fixup"]


def ground(binding, target, oracle):
    """Recheck frozen original witnesses outside the contribution being judged."""
    require(binding["schema"] == 1 and binding["mode"] == "external-dgroup-offset16-v1",
            "Unsupported binding proof mode")
    require(binding["target_id"] == target["id"] and
            binding["target_sha256"] == sha(oracle.data), "Binding belongs to another target/oracle")
    require(target["space"] == "root", "Overlay DGROUP binding not proven")
    space = oracle.spaces["root"]
    start = space.position(target["segment"], target["offset"])
    end = start + target["size"]

    def witness(spec):
        require(spec["space"] == "root", "Unsupported cross-space data witness")
        at, raw = spec["image_offset"], bytes.fromhex(spec["bytes_hex"])
        require(raw and 0 <= at and at+len(raw) <= space.size and
                not (at < end and start < at+len(raw)), "Binding witness overlaps candidate or escapes root")
        require(space.data[at:at+len(raw)] == raw, "Original binding witness changed")
        return at, raw

    frame = binding["frame"]
    at, raw = witness(frame["witness"])
    # Only MOV r16,paragraph / MOV DS,r16 is supported.
    require(len(raw) == 5 and 0xb8 <= raw[0] <= 0xbf and
            raw[3:] == bytes((0x8e, 0xd8+raw[0]-0xb8)) and
            struct.unpack_from("<H", raw, 1)[0] == frame["segment"] and
            sum(r["image_offset"] == at+1 for r in space.relocations) == 1,
            "DGROUP frame lacks a relocated startup DS load")
    require(0 <= frame["segment"] <= 0xffff and
            frame["segment"]*16 < space.size, "DGROUP frame outside root")
    entry_at, entry_raw = witness(frame["entry_path"])
    entry = space.position(oracle.mz.header["initial_cs"], oracle.mz.header["initial_ip"])
    entry_rows = instructions(entry_raw)
    require(entry_at == entry and entry_at+len(entry_raw) == at+len(raw) and
            entry_raw.endswith(raw) and
            len(entry_rows) >= 2 and entry_rows[-2].address == at-entry_at and
            bytes(entry_rows[-2].bytes) == raw[:3] and bytes(entry_rows[-1].bytes) == raw[3:] and
            all(not set(i.groups).intersection((1, 2, 3, 4, 5, 7)) for i in entry_rows),
            "DS load is not reached by the frozen straight-line MZ entry prefix")
    symbols = binding["symbols"]
    spans = set()
    for name, symbol in symbols.items():
        offset, width = symbol["offset"], symbol["width"]
        require(symbol["kind"] == "near-data-alias" and symbol["group"] == "DGROUP" and
                type(offset) is int and type(width) is int and width in (1, 2) and
                0 <= offset and offset+width <= 65536, "Unsupported grounded data object")
        span = set(range(offset, offset+width))
        require(not spans.intersection(span), "Grounded data aliases overlap")
        spans.update(span)
        require(symbol["witnesses"], "Data symbol has no independent witness")
        for spec in symbol["witnesses"]:
            _, raw = witness(spec)
            rows = instructions(raw)
            require(len(rows) == 1, "Data witness must be one complete instruction")
            operand = direct_operand(rows[0])
            require(rows[0].disp_offset == spec["operand_offset"] and
                    operand.size == width and (operand.mem.disp & 0xffff) == offset,
                    "Data witness does not ground this object address/width")
            # A witness grounds DS-relative offset/width only. Its annotation
            # cannot prove that runtime DS still equals the startup paragraph.
    return symbols


def compare(module, code_name, actual, expected, binding, target, space, oracle):
    symbols = ground(binding, target, oracle)
    obligations = fixes(module)
    require(obligations and obligations == binding["fixups"], "Ordered symbolic fixup obligations changed")
    require(declarations(module) == binding["declarations"], "Bound object declarations changed")
    require(len(actual) == len(expected), "Bound component extent differs; trimming forbidden")
    require(set(symbols) == {e["target"].get("name") for e in obligations},
            "Missing or unused grounded data symbol")
    groups = [g for g in module.groups if g["name"] == "DGROUP"]
    require(len(groups) == 1 and groups[0]["segments"] and
            all(1 <= i <= len(module.segment_defs) and
                module.segment_defs[i-1]["class"] in ("DATA", "BSS", "CONST") and
                module.segment_defs[i-1]["length"] == 0 for i in groups[0]["segments"]),
            "External data binding needs an empty declared DGROUP")
    cs, xc = _capstone()
    rows = instructions(actual)
    for insn in rows:
        require(not set(insn.groups).intersection((cs.CS_GRP_CALL, cs.CS_GRP_INT, cs.CS_GRP_IRET)) and
                insn.mnemonic not in ("int", "into", "iret", "iretd") and
                not set(insn.regs_access()[1]).intersection((xc.X86_REG_DS, xc.X86_REG_ES,
                                                          xc.X86_REG_SS, xc.X86_REG_CS)),
                "Near-data binding helper changes segment context or calls unknown code")
    occupied, equations, differences = set(), [], []
    for fix in obligations:
        at = fix["segment_offset"]
        target_ref = fix["target"]
        require(fix["segment"] == code_name and fix["loc_type"] == 1 and
                fix["field_width"] == 2 and not fix["self_relative"] and
                target_ref["kind"] == "external" and target_ref["method"] == 6 and
                fix["displacement"] is None and
                fix["frame"] == {"kind": "target_frame", "target": target_ref},
                "Unsupported DGROUP fixup equation")
        require(0 <= at <= len(actual)-2 and not occupied.intersection((at, at+1)),
                "Overlapping/out-of-range bound fields")
        symbol = symbols[target_ref["name"]]
        insns = [i for i in rows if i.address+i.disp_offset == at and i.disp_size == 2]
        require(len(insns) == 1 and direct_operand(insns[0]).size == symbol["width"],
                "FIXUPP is not the grounded DS memory operand")
        addend = struct.unpack_from("<H", actual, at)[0]
        require(addend == 0, "Only zero encoded data addends are proven")
        # F5 selects the external's group frame. For the independently reviewed
        # relative alias S=F+offset, S-F+A+D = offset (A=0,D omitted).
        # No absolute runtime address or natural symbol placement is inferred.
        value = symbol["offset"]+addend
        observed = struct.unpack_from("<H", expected, at)[0]
        if value != observed:
            differences.append(at)
        equations.append({"offset": at, "symbol": target_ref["name"], "encoded_addend": addend,
                          "grounded_ds_relative_offset": symbol["offset"],
                          "linked_value": value, "original_value": observed, "equal": value == observed})
        occupied.update((at, at+1))
    # A literal address may not stand in for a missing external FIXUPP.
    for insn in rows:
        memory = [o for o in insn.operands if o.type == xc.X86_OP_MEM and
                  o.mem.base == o.mem.index == 0 and o.mem.segment == 0]
        if memory:
            direct_operand(insn)
            require({insn.address+insn.disp_offset, insn.address+insn.disp_offset+1} <= occupied,
                    "Absolute DS operand lacks a symbolic candidate fixup")
    start = space.position(target["segment"], target["offset"])
    require(not any(start-1 <= r["image_offset"] < start+len(expected) for r in space.relocations),
            "Original relocation obligations are not generated by offset16 fixups")
    ordinary_differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if i not in occupied and a != b]
    field_mismatches = len(differences)
    differences += ordinary_differences
    return {"exact": not differences, "first_difference": min(differences, default=None),
            "mismatch_count": len(differences),
            "mismatch_basis": "ordinary bytes plus unequal field equations",
            "ordinary_mismatch_count": len(ordinary_differences), "field_mismatch_count": field_mismatches,
            "fixups": len(obligations), "relocations": 0, "equations": equations,
            "ordinary_bytes_compared": len(actual)-len(occupied),
            "proof_scope": "Complete component bytes and external DGROUP offset16 equations to independent DS-relative aliases; raw object preserved, absolute data placement/original symbols/TU/runtime DS association/natural link placement unproved"}
