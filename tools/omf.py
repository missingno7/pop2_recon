"""Strict reader for the 16-bit Microsoft OMF records emitted by MSC."""
from __future__ import annotations
from dataclasses import dataclass
import struct

class OmfError(ValueError):
    pass

@dataclass
class ObjectModule:
    name: str
    segments: dict[str, bytes]
    segment_lengths: dict[str, int]
    segment_defs: list[dict]
    publics: list[dict]
    externals: list[str]
    groups: list[dict]
    fixups: list[dict]
    records: list[dict]
    initialized_ranges: dict[str, list[tuple[int, int]]]

    def segment_bytes(self, name: str) -> bytes:
        if name not in self.segments:
            raise OmfError(f"no initialized data for segment {name!r}")
        return self.segments[name]

    def segment_length(self, name: str) -> int | None:
        return self.segment_lengths.get(name)

class OmfReader:
    """Decode 16-bit OMF structure and data; reject unknown records and truncation."""
    _IGNORED = {0x94}
    # Width in bytes of the 16-bit OMF LOCAT field.  These are field widths,
    # not a request to apply the relocation.  6/7/8/10/12/14/15 are reserved.
    _LOC_WIDTH = {0: 1, 1: 2, 2: 2, 3: 4, 4: 1, 5: 2}
    _RECORD_NAMES = {
        0x80: "THEADR", 0x82: "LHEADR", 0x88: "COMENT", 0x8A: "MODEND",
        0x8B: "MODEND32", 0x8C: "EXTDEF", 0x90: "PUBDEF", 0x91: "PUBDEF32",
        0x96: "LNAMES", 0x98: "SEGDEF", 0x99: "SEGDEF32", 0x9A: "GRPDEF",
        0x9C: "FIXUPP", 0x9D: "FIXUPP32", 0xA0: "LEDATA", 0xA1: "LEDATA32",
        0xA2: "LIDATA", 0xA3: "LIDATA32", 0xB0: "COMDEF", 0xB1: "COMDEF32",
    }

    @staticmethod
    def _index(data: bytes, at: int) -> tuple[int, int]:
        if at >= len(data):
            raise OmfError("truncated OMF index")
        first = data[at]
        if first & 0x80:
            if at + 1 >= len(data):
                raise OmfError("truncated two-byte OMF index")
            return ((first & 0x7f) << 8) | data[at + 1], at + 2
        return first, at + 1

    @staticmethod
    def records(data: bytes):
        at = 0
        while at < len(data):
            if at + 3 > len(data):
                raise OmfError(f"truncated OMF record header at file offset {at:#x}")
            kind = data[at]
            length = struct.unpack_from("<H", data, at + 1)[0]
            end = at + 3 + length
            if length < 1 or end > len(data):
                raise OmfError(f"invalid OMF record length at file offset {at:#x}")
            raw = data[at:end]
            if sum(raw) & 0xff:
                raise OmfError(f"bad checksum in OMF record at file offset {at:#x}")
            yield kind, raw[3:-1], at
            at = end

    @classmethod
    def _lidata_block(cls, data: bytes, at: int) -> tuple[bytes, int]:
        if at + 4 > len(data):
            raise OmfError("truncated LIDATA block")
        repeat, children = struct.unpack_from("<HH", data, at)
        at += 4
        if children == 0:
            if at >= len(data):
                raise OmfError("truncated LIDATA leaf")
            n = data[at]
            at += 1
            if at + n > len(data):
                raise OmfError("truncated LIDATA bytes")
            return data[at:at+n] * repeat, at+n
        unit = bytearray()
        for _ in range(children):
            child, at = cls._lidata_block(data, at)
            unit.extend(child)
        return bytes(unit) * repeat, at

    def read(self, data: bytes, label: str = "") -> ObjectModule:
        names, seg_defs, group_defs = [], [], []
        group_names, lengths, contents, ranges = [], {}, {}, {}
        publics, externals, fixups, recs = [], [], [], []
        module_name, last_data, ended = label, None, False
        seen_seg_names = set()
        for kind, body, file_offset in self.records(data):
            if ended:
                raise OmfError("records found after MODEND")
            recs.append({"type": kind, "name": self._RECORD_NAMES.get(kind, f"0x{kind:02x}"),
                         "file_offset": file_offset, "body_hex": body.hex()})
            if kind in (0x91, 0x95, 0x99, 0x9D, 0xA1, 0xA3, 0x8B, 0xB1):
                raise OmfError(f"unsupported 32-bit OMF record {kind:#04x}")
            if kind in (0x80, 0x82):
                if not body or len(body) < 1 + body[0]:
                    raise OmfError("malformed THEADR/LHEADR")
                module_name = body[1:1+body[0]].decode("latin1")
            elif kind == 0x96:
                at = 0
                while at < len(body):
                    n = body[at]; at += 1
                    if at + n > len(body): raise OmfError("truncated LNAMES entry")
                    names.append(body[at:at+n].decode("latin1")); at += n
            elif kind == 0x98:
                if len(body) < 3: raise OmfError("truncated SEGDEF")
                acbp, at = body[0], 1
                alignment, frame, frame_offset = acbp >> 5, None, None
                if alignment == 0:
                    if at + 3 > len(body): raise OmfError("truncated absolute SEGDEF")
                    frame = struct.unpack_from("<H", body, at)[0]
                    frame_offset = body[at+2]; at += 3
                if at + 2 > len(body): raise OmfError("truncated SEGDEF length")
                size = struct.unpack_from("<H", body, at)[0]; at += 2
                name_i, at = self._index(body, at)
                class_i, at = self._index(body, at)
                overlay_i, at = self._index(body, at)
                if at != len(body): raise OmfError("trailing bytes in SEGDEF")
                getname = lambda i: names[i-1] if 0 < i <= len(names) else f"?name{i}"
                name, cls, overlay = getname(name_i), getname(class_i), getname(overlay_i)
                if name in seen_seg_names:
                    raise OmfError(f"duplicate SEGDEF name {name!r} is ambiguous")
                if acbp & 3:
                    raise OmfError(f"32-bit/large SEGDEF flags are unsupported for {name}")
                seen_seg_names.add(name)
                lengths[name] = size
                seg_defs.append({"name": name, "class": cls, "overlay": overlay, "length": size,
                                 "alignment": alignment, "combine": (acbp >> 2) & 7,
                                 "big": bool(acbp & 2), "use32": bool(acbp & 1),
                                 "frame": frame, "frame_offset": frame_offset})
            elif kind == 0x9A:
                if not body: raise OmfError("empty GRPDEF")
                n, at = body[0], 1
                group_name = names[n-1] if 0 < n <= len(names) else f"?name{n}"
                group_names.append(group_name); members = []
                while at < len(body):
                    if body[at] != 0xff: raise OmfError("unknown GRPDEF component")
                    idx, at = self._index(body, at+1); members.append(idx)
                group_defs.append({"name": group_name, "segments": members})
            elif kind == 0x8C:
                at = 0
                while at < len(body):
                    n = body[at]; at += 1
                    if at + n > len(body): raise OmfError("truncated EXTDEF name")
                    externals.append(body[at:at+n].decode("latin1")); at += n
                    _, at = self._index(body, at)
            elif kind == 0x90:
                at = 0
                group_i, at = self._index(body, at)
                segment_i, at = self._index(body, at)
                frame = None
                if group_i == 0 and segment_i == 0:
                    if at+2 > len(body): raise OmfError("truncated PUBDEF frame")
                    frame = struct.unpack_from("<H", body, at)[0]; at += 2
                while at < len(body):
                    n = body[at]; at += 1
                    if at+n+2 > len(body): raise OmfError("truncated PUBDEF entry")
                    name = body[at:at+n].decode("latin1"); at += n
                    offset = struct.unpack_from("<H", body, at)[0]; at += 2
                    type_i, at = self._index(body, at)
                    seg_name = seg_defs[segment_i-1]["name"] if 0 < segment_i <= len(seg_defs) else None
                    publics.append({"name": name, "segment": seg_name, "segment_index": segment_i,
                                    "offset": offset, "group_index": group_i, "frame": frame,
                                    "type_index": type_i})
            elif kind in (0xA0, 0xA2):
                seg_i, at = self._index(body, 0)
                if at + 2 > len(body): raise OmfError("truncated LEDATA/LIDATA offset")
                offset = struct.unpack_from("<H", body, at)[0]; at += 2
                if kind == 0xA0: payload, iterated = body[at:], False
                else:
                    chunks = bytearray()
                    while at < len(body):
                        chunk, at = self._lidata_block(body, at); chunks.extend(chunk)
                    payload, iterated = bytes(chunks), True
                if not seg_i or seg_i > len(seg_defs):
                    raise OmfError(f"data references undeclared segment {seg_i}")
                seg_name, seg_len = seg_defs[seg_i-1]["name"], seg_defs[seg_i-1]["length"]
                if offset + len(payload) > seg_len:
                    raise OmfError(f"data extends past SEGDEF extent for {seg_name}")
                buf = contents.setdefault(seg_i, bytearray(seg_len))
                old_ranges = ranges.setdefault(seg_i, [])
                for lo, hi in old_ranges:
                    if offset < hi and lo < offset + len(payload):
                        raise OmfError(f"overlapping LEDATA/LIDATA for {seg_name}")
                buf[offset:offset+len(payload)] = payload
                old_ranges.append((offset, offset+len(payload)))
                last_data = (seg_i, offset, payload, iterated)
            elif kind == 0x9C:
                anchor = last_data
                fixups.append({"segment_index": anchor[0] if anchor else None,
                               "data_offset": anchor[1] if anchor else None,
                               "data_length": len(anchor[2]) if anchor else None,
                               "iterated": anchor[3] if anchor else None,
                               "anchor_known": anchor is not None, "record_hex": body.hex(),
                               "decoded": self._decode_fixup_record(body)})
            elif kind == 0x8A:
                # An empty body is accepted for legacy fixtures; the usual
                # object form has one module-type byte. Bit 1 announces an
                # entry-point tuple, which this reader does not resolve.
                if len(body) > 1 or (body and body[0] & 0xfe):
                    raise OmfError("MODEND entry-point metadata is unsupported")
                ended = True
            elif kind in self._IGNORED or kind == 0x88:
                pass
            else:
                raise OmfError(f"unsupported OMF record {kind:#04x}")
        if not ended: raise OmfError("OMF module has no MODEND")
        segments = {seg_defs[i-1]["name"]: bytes(buf) for i, buf in contents.items()}
        initialized = {}
        for i, spans in ranges.items():
            merged = []
            for start, end in sorted(spans):
                if merged and start == merged[-1][1]:
                    merged[-1] = (merged[-1][0], end)
                else:
                    merged.append((start, end))
            initialized[seg_defs[i-1]["name"]] = merged
        self._resolve_fixups(fixups, seg_defs, group_defs, externals)
        return ObjectModule(module_name, segments, lengths, seg_defs, publics,
                            externals, group_defs, fixups, recs, initialized)

    @classmethod
    def _resolve_fixups(cls, fixups: list[dict], seg_defs: list[dict],
                        group_defs: list[dict], externals: list[str]) -> None:
        """Attach symbolic FIXUPP descriptions; never writes relocated bytes.

        Thread tables are independent for frame and target threads and persist
        across FIXUPP records.  The source record and its original decoded
        fields remain intact; ``resolved`` parallels the decoded subrecords.
        """
        frame_threads: dict[int, dict] = {}
        target_threads: dict[int, dict] = {}

        def indexed(table, index, label):
            if not isinstance(index, int) or index < 1 or index > len(table):
                raise OmfError(f"FIXUPP {label} index {index!r} is undefined")
            return table[index - 1]

        def binding(frame: bool, method: int, datum: int | None, seg_i: int | None):
            if frame:
                if method == 0:
                    seg = indexed(seg_defs, datum, "frame segment")
                    return {"kind": "segment", "index": datum, "name": seg["name"]}
                if method == 1:
                    group = indexed(group_defs, datum, "frame group")
                    return {"kind": "group", "index": datum, "name": group["name"]}
                if method == 2:
                    return {"kind": "external", "index": datum,
                            "name": indexed(externals, datum, "frame external")}
                if method == 3:
                    raise OmfError("unsupported FIXUPP frame method 3")
                if method == 4:
                    seg = indexed(seg_defs, seg_i, "location segment")
                    return {"kind": "segment", "index": seg_i, "name": seg["name"],
                            "basis": "location"}
                if method == 5:
                    return {"kind": "target_frame"}
                raise OmfError(f"unsupported FIXUPP frame method {method}")
            if method == 0:
                seg = indexed(seg_defs, datum, "target segment")
                return {"kind": "segment", "index": datum, "name": seg["name"], "method": 0}
            if method == 1:
                group = indexed(group_defs, datum, "target group")
                return {"kind": "group", "index": datum, "name": group["name"], "method": 1}
            if method == 2:
                return {"kind": "external", "index": datum,
                        "name": indexed(externals, datum, "target external"), "method": 2}
            if method == 3:
                if datum is None: raise OmfError("FIXUPP absolute target lacks a datum")
                return {"kind": "absolute", "frame": datum, "method": 3}
            if method == 4:
                return {"kind": "segment", "index": datum,
                        "name": indexed(seg_defs, datum, "target segment")["name"],
                        "method": 4, "zero_displacement": True}
            if method == 5:
                return {"kind": "group", "index": datum,
                        "name": indexed(group_defs, datum, "target group")["name"],
                        "method": 5, "zero_displacement": True}
            if method == 6:
                return {"kind": "external", "index": datum,
                        "name": indexed(externals, datum, "target external"),
                        "method": 6, "zero_displacement": True}
            raise OmfError(f"unsupported FIXUPP target method {method}")

        for record in fixups:
            resolved = []
            seg_i, base = record["segment_index"], record["data_offset"]
            data_length, iterated = record["data_length"], record["iterated"]
            for row in record["decoded"]:
                if row["kind"] == "thread":
                    # The TARGET-thread method's high bit is supplied by the
                    # P bit in each referring FIXUP, not by THREAD itself.
                    method = row["method"] if row["frame"] else row["method"] & 3
                    table = frame_threads if row["frame"] else target_threads
                    if row["frame"] and method in (4, 5):
                        value = {"kind": "frame_special", "method": method}
                    else:
                        value = binding(row["frame"], method, row["datum"], seg_i)
                    table[row["thread"]] = value
                    resolved.append({"kind": "thread", "frame": row["frame"],
                                     "thread": row["thread"], "binding": value})
                    continue

                loc, width = row["loc_type"], cls._LOC_WIDTH.get(row["loc_type"])
                if width is None:
                    raise OmfError(f"unsupported FIXUPP location type {loc}")
                if seg_i is None or base is None:
                    raise OmfError("FIXUPP relocation has no preceding LEDATA/LIDATA anchor")
                if iterated:
                    raise OmfError("FIXUPP relocation anchored to unsupported LIDATA")
                if row["location"] + width > data_length:
                    raise OmfError("FIXUPP relocation field extends past its LEDATA record")
                seg = indexed(seg_defs, seg_i, "location segment")
                segment_offset = base + row["location"]
                if segment_offset + width > seg["length"]:
                    raise OmfError("FIXUPP relocation field extends past segment extent")

                frame_field = row["frame_method"]
                if row.get("frame_thread"):
                    thread_no = frame_field & 3
                    if thread_no not in frame_threads:
                        raise OmfError(f"undefined FIXUPP frame thread {thread_no}")
                    frame = frame_threads[thread_no]
                else:
                    frame = binding(True, frame_field, row["frame_index"], seg_i)
                if row["target_thread"]:
                    thread_no = row["target_thread_index"]
                    if thread_no not in target_threads:
                        raise OmfError(f"undefined FIXUPP target thread {thread_no}")
                    base_target = target_threads[thread_no]
                    effective_method = base_target["method"]
                    if row["target_has_displacement"]:
                        target = dict(base_target)
                    else:
                        effective_method += 4
                        target = binding(False, effective_method,
                                         base_target.get("index"), seg_i)
                else:
                    target = binding(False, row["effective_target_method"],
                                     row["target_index"], seg_i)
                # F5 inline frames and frame threads describe a frame derived from the
                # target. Resolve when unambiguous, otherwise keep symbolic.
                if frame.get("kind") == "frame_special":
                    method = frame["method"]
                    if method == 4:
                        frame = {"kind": "segment", "index": seg_i,
                                 "name": seg["name"], "basis": "location"}
                    elif method == 5:
                        frame = {"kind": "target_frame"}
                if frame.get("kind") == "target_frame":
                    if target.get("kind") not in ("segment", "group", "external"):
                        raise OmfError("FIXUPP target-derived frame has a non-address target")
                    frame = {"kind": "target_frame", "target": target}
                resolved.append({"kind": "fixup", "segment_index": seg_i,
                                 "segment": seg["name"], "segment_offset": segment_offset,
                                 "field_width": width, "self_relative": row["self_relative"],
                                 "frame": frame, "target": target,
                                 "displacement": row["displacement"]})
            record["resolved"] = resolved

    @classmethod
    def _decode_fixup_record(cls, body: bytes) -> list[dict]:
        at, out = 0, []
        while at < len(body):
            first = body[at]
            if not first & 0x80:
                at += 1; is_frame = bool(first & 0x40)
                method, thread = (first >> 2) & 7, first & 3
                datum = None
                if is_frame and method == 3:
                    raise OmfError("unsupported FIXUPP frame thread method 3")
                if not (is_frame and method in (4, 5, 6)):
                    if not is_frame and (method & 3) == 3:
                        if at + 2 > len(body): raise OmfError("truncated FIXUPP target frame datum")
                        datum = struct.unpack_from("<H", body, at)[0]; at += 2
                    else:
                        datum, at = cls._index(body, at)
                out.append({"kind": "thread", "frame": is_frame, "method": method,
                            "thread": thread, "datum": datum})
                continue
            if at + 3 > len(body): raise OmfError("truncated FIXUPP subrecord")
            locat = (body[at] << 8) | body[at+1]; at += 2
            fixdat = body[at]; at += 1
            loc, frame_method = (locat >> 10) & 15, (fixdat >> 4) & 7
            target_method, frame_i, target_i = fixdat & 3, None, None
            if not fixdat & 0x80 and frame_method in (0, 1, 2):
                frame_i, at = cls._index(body, at)
            elif not fixdat & 0x80 and frame_method == 3:
                if at+2 > len(body): raise OmfError("truncated FIXUPP frame datum")
                frame_i = struct.unpack_from("<H", body, at)[0]; at += 2
            target_thread = bool(fixdat & 8)
            if not target_thread:
                if target_method == 3:
                    if at + 2 > len(body): raise OmfError("truncated FIXUPP target frame datum")
                    target_i = struct.unpack_from("<H", body, at)[0]; at += 2
                else:
                    target_i, at = cls._index(body, at)
            target_thread_i = (fixdat & 3) if target_thread else None
            displacement = None
            if not fixdat & 4:
                if at+2 > len(body): raise OmfError("truncated FIXUPP displacement")
                displacement = struct.unpack_from("<H", body, at)[0]; at += 2
            out.append({"kind": "fixup", "location": locat & 0x3ff, "loc_type": loc,
                        "self_relative": not bool(locat & 0x4000), "frame_method": frame_method,
                        "frame_index": frame_i, "target_method": target_method,
                        "target_index": target_i, "target_thread": target_thread,
                        "target_thread_index": target_thread_i,
                        "frame_thread": bool(fixdat & 0x80),
                        "target_has_displacement": not bool(fixdat & 4),
                        "effective_target_method": target_method + (4 if fixdat & 4 else 0),
                        "displacement": displacement})
        return out

def read_object(data: bytes, label: str = "") -> ObjectModule:
    return OmfReader().read(data, label)
