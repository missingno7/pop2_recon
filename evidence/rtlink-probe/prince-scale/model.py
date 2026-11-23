"""o11_relorder: RTLink 6.10 relocation-order model applied to a whole staged link.

Registration (symbol ids) and per-final-segment group hashing follow o9_linktools/rtlink_order.py:
  key = (flags, 0, id, id), flags 048Bh base16 / 0C8Bh pointer32; bucket = sum ROR16(byte_j, j) mod 101,
  new keys at the head of their bucket chain, buckets walked 0..100, sites in FIXUPP order per key.
This module adds whole-link registration over objects + library members and keys for segment/group
targets (hypotheses selected by the candidate-link check, see notes.md)."""
from __future__ import annotations
import struct
from collections import defaultdict

FLAGS = {2: 0x048B, 3: 0x0C8B}


def ror16(x, n):
    n &= 15
    return ((x >> n) | (x << ((16 - n) & 15))) & 0xFFFF


def bucket(key: bytes) -> int:
    return (sum(ror16(b, j) for j, b in enumerate(key)) & 0xFFFF) % 101


def hkey(flags, a, b, c):
    return struct.pack("<4H", flags & 0xFFFF, a & 0xFFFF, b & 0xFFFF, c & 0xFFFF)


def hb(flags, sid):
    return bucket(hkey(flags, 0, sid, sid))


def _pubdef_names(body: bytes):
    at = 0

    def idx(at):
        b = body[at]
        return (((b & 0x7F) << 8) | body[at + 1], at + 2) if b & 0x80 else (b, at + 1)
    g, at = idx(at)
    s, at = idx(at)
    if g == 0 and s == 0:
        at += 2
    out = []
    while at < len(body):
        n = body[at]
        out.append(body[at + 1:at + 1 + n].decode("latin1"))
        at += 1 + n + 2
        _, at = idx(at)
    return out


def _comdef_names(body: bytes):
    at, out = 0, []
    while at < len(body):
        n = body[at]
        out.append(body[at + 1:at + 1 + n].decode("latin1"))
        at += 1 + n
        at += 1 if body[at] < 0x80 else 2
        dtype = body[at]
        at += 1
        for _ in range(2 if dtype == 0x61 else 1):
            lead = body[at]
            at += 1 + {0x81: 2, 0x84: 3, 0x88: 4}.get(lead, 0)
    return out


class Registry:
    """Case-insensitive symbol table in registration order."""

    def __init__(self, prefix=10, ignore_case=True):
        self.ignore_case = ignore_case
        self.ids = {}
        self.order = []
        self.prefix = prefix

    def reg(self, name, why=None):
        k = name.upper() if self.ignore_case else name
        if k not in self.ids:
            self.ids[k] = len(self.ids) + 1 + self.prefix
            self.order.append((k, why))
        return self.ids[k]

    def id(self, name):
        return self.ids[name.upper() if self.ignore_case else name]


def _fixupp_externals(body, externals, threads):
    """External names a FIXUPP record references, in subrecord order (frame before target).
    `threads` persists across the module's FIXUPP records: ('F'|'T', n) -> (method, datum)."""
    from omf import OmfReader
    out = []
    for row in OmfReader._decode_fixup_record(body):
        if row["kind"] == "thread":
            threads[("F" if row["frame"] else "T", row["thread"])] = (row["method"], row["datum"])
            continue
        if row["frame_thread"]:
            fm, fd = threads.get(("F", row["frame_method"] & 3), (None, None))
        else:
            fm, fd = row["frame_method"], row["frame_index"]
        if fm == 2 and fd:
            out.append(externals[fd - 1])
        if row["target_thread"]:
            tm, td = threads.get(("T", row["target_thread_index"]), (None, None))
            tm = tm & 3 if tm is not None else None
        else:
            tm, td = row["target_method"] & 3, row["target_index"]
        if tm == 2 and td:
            out.append(externals[td - 1])
    return out


def module_events(m, local_mode="global"):
    """Registration events of one module in record order: [(name, why)]."""
    ev = []
    threads = {}
    locals_ = getattr(m, "local_externals", set())

    def nm(name, local):
        return f"{name}@{m.name}" if (local and local_mode == "scoped") else name
    for rec in m.records:
        t = rec["type"]
        body = bytes.fromhex(rec["body_hex"])
        if t in (0x90, 0xB6):
            for n in _pubdef_names(body):
                ev.append((nm(n, t == 0xB6), "pub"))
        elif t == 0xB0:
            for n in _comdef_names(body):
                ev.append((n, "comdef"))
        elif t == 0x9C:
            for n in _fixupp_externals(body, m.externals, threads):
                ev.append((nm(n, n in locals_), "fix"))
        elif t == 0x8A:
            for n in m.externals:
                ev.append((nm(n, n in locals_), "end"))
    return ev


def register(seq, prefix=10, local_mode="global", main_module=None, extra_before=None, hooks=None):
    """Registry over the processing sequence [(sec, label, module)].  `extra_before` maps a seq index to
    names registered before that module; `main_module` is the seq index whose MODEND registers $$MAIN."""
    R = Registry(prefix)
    for i, (sec, label, m) in enumerate(seq):
        for n in (extra_before or {}).get(i, []):
            R.reg(n, ("pre", label))
        for n, why in module_events(m, local_mode):
            R.reg(n, (why, label))
        if main_module is not None and i == main_module:
            R.reg("$$MAIN", ("main", label))
    return R


def target_key(f, R, segkey=None):
    """Hash key for one relocating fixup, or None when unmodelled."""
    t = f["target"]
    flags = FLAGS[f["loc"]]
    if t.get("kind") == "external":
        sid = R.ids.get(t["name"].upper())
        if sid is None:
            return None
        return hkey(flags, 0, sid, sid)
    if segkey is not None:
        return segkey(f, flags)
    return None


def group_order(descs):
    """descs: [(key, payload)] in first-FIXUPP order -> keys in emission order."""
    buckets = [[] for _ in range(101)]
    seen = set()
    for k, _ in descs:
        if k not in seen:
            seen.add(k)
            buckets[bucket(k)].insert(0, k)
    return [k for b in buckets for k in b]


def predict(fixups, R, seg_order, segkey=None):
    """fixups: reloc_fixups rows in processing order.  seg_order: {section: [segment names in table
    order]}.  Returns {section: [site, ...]} (None entries for unmodelled sites, appended at the end
    of their segment) and the per-segment group lists."""
    per = defaultdict(list)
    for f in fixups:
        per[(f["sec"], f["seg"])].append(f)
    out, groups = {}, {}
    for sec, names in seg_order.items():
        lst = []
        for sn in names:
            fl = per.get((sec, sn), [])
            descs, sites, unk = [], defaultdict(list), []
            for f in fl:
                k = target_key(f, R, segkey)
                if k is None:
                    unk.append(f["site"])
                    continue
                descs.append((k, f))
                sites[k].append(f["site"])
            order = group_order(descs)
            groups[(sec, sn)] = [(k, sites[k]) for k in order]
            lst += [s for k in order for s in sites[k]] + unk
        out[sec] = lst
    return out, groups
