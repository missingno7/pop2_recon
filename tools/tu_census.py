"""Research census of code frames as candidate source-file (TU) boundaries.

In the resident image, relocated far calls witness candidate MSC code frames.
For the code overlays in the locked Prince image, the relocation records'
site segments witness frames even when no far call can name the overlay.
Resident RTLink vectors identify their destination by descriptor index.
Direct cross-overlay calls use the existing load state and can remain ambiguous.
Output is for boundary research and grants no ownership.
"""
import argparse
import bisect
import collections
import struct

from common import ROOT, project_path, read_json, require, write_json
from oracle import Oracle

DEFAULT_OUTPUT = "build/workers/tu_census/census.json"
MANAGER_INTERCEPT = 0x0558
VECTOR_SIZE = 10
# Reviewed internal transitions in the locked image. Each window is four
# bytes before and four after the boundary (far-return tail or pad -> entry).
OVERLAY_BOUNDARIES = {
    "overlay-2": {0x046c: "8be55dcb558bec83", 0x55dc: "330111cb558bec57", 0x6466: "ca020090558bec83"},
    "overlay-3": {0x2434: "ca040090558bec83", 0x296e: "0000cb90c706205e", 0x4bee: "cc02cb90558bec83"},
    "overlay-6": {0x0c4a: "aa5fcb90558bec83"},
    "overlay-7": {0x046c: "5dca0400558bec83"},
    "overlay-8": {0x026c: "0100cb90558bec83"},
    "overlay-9": {0x0fd6: "2dcbcb90558bec83"},
}
# These two callers use the same address with different callee-pop ABIs.
# Only these reviewed edges use this filter. See frames.md for both paths.
REVIEWED_ABI_CALLS = {
    0x453a: ("overlay-8", 0x453a, "9a00004e2a5e5f8be55dcb"),
    0xc13b: ("overlay-9", 0xc132, "569ac40bcf29eb06569a00004e2a5e8be55dca0200"),
}


def far_call_sites(space):
    """Immediate CALL FAR sites whose segment word is an original relocation."""
    for r in space.relocations:
        at = r["image_offset"] - 3
        if at >= 0 and space.data[at] == 0x9a:
            offset, segment = struct.unpack_from("<HH", space.data, at + 1)
            yield {"site": at, "segment": segment, "offset": offset,
                   "relocation_index": r["index"], "source_relocation_segment": r["segment"]}


def held_in(space, segment, offset):
    return 0 <= (segment - space.link_segment) * 16 + offset < space.size


def overlay_vectors(oracle, code_overlays):
    """Decode the format checked by original manager code at 2203:064c."""
    root = oracle.spaces["root"]
    cs = oracle.mz.header["initial_cs"]
    base = cs * 16
    descriptors = [s.descriptor for s in oracle.spaces.values() if s.descriptor]
    by_index = {s.descriptor["table_index"]: s for s in code_overlays.values()}
    # 05e3..05f7 takes max(filename pointer) + 80h and walks ten-byte records.
    start = max(d["f2"] for d in descriptors) + 0x80
    relocations = {r["image_offset"]: r for r in root.relocations}
    vectors = {}
    for ip in range(start, root.size - base - VECTOR_SIZE + 1, VECTOR_SIZE):
        at = base + ip
        call, disp, jump, off, seg, index = struct.unpack_from("<BhBHHH", root.data, at)
        if (call, (ip + 3 + disp) & 0xffff, jump) != (0xe8, MANAGER_INTERCEPT, 0xea):
            break
        require(index in by_index and at + 6 in relocations, f"Invalid vector at {cs:04x}:{ip:04x}")
        target = by_index[index]
        require(held_in(target, seg, off), f"Vector target outside {target.name}")
        vectors[(cs, ip)] = {
            "segment": cs, "offset": ip, "position": at, "file_offset": root.file_offset + at,
            "bytes_hex": root.data[at:at + VECTOR_SIZE].hex(),
            "jump_segment_relocation_index": relocations[at + 6]["index"],
            "descriptor_index": index, "overlay_id": target.descriptor["overlay_id"],
            "target_space": target.name, "target_segment": seg, "target_offset": off,
            "target_position": target.position(seg, off), "callers": [], "other_relocated_references": [],
        }
    require(vectors, "Original manager vector table is missing")
    return vectors


def relocation_frames(space):
    """Group by the original section-relative relocation site frame, not its stored word."""
    groups = collections.defaultdict(list)
    for record in space.relocations:
        groups[record["segment"]].append(record)
    return dict(sorted(groups.items()))


def frame_extents(space, groups, vector_entries):
    """Bound each transition independently by its frame floor and witnessed bytes."""
    start_bounds = {}
    previous_end = 0
    for frame, records in groups.items():
        base = (frame - space.link_segment) * 16
        first = min(r["image_offset"] for r in records)
        earliest_entry = min(vector_entries.get(frame, (first,)))
        lo, hi = max(base, previous_end), min(base + 15, first, earliest_entry)
        require(0 <= lo <= hi < space.size, f"Inconsistent frame bounds at {space.name}:{frame:04x}")
        start_bounds[frame] = lo, hi
        previous_end = max(r["image_offset"] + 2 for r in records)
    ends = list(start_bounds.values())[1:] + [(space.size, space.size)]
    extents = {
        frame: {"start_min": start[0], "start_max": start[1],
                "end_exclusive_min": end[0], "end_exclusive_max": end[1]}
        for (frame, start), end in zip(start_bounds.items(), ends)
    }
    frames = list(extents)
    for pos, window in OVERLAY_BOUNDARIES.get(space.name, {}).items():
        frame = space.link_segment + pos // 16
        require(frame in extents and frame != frames[0], f"Unknown internal boundary in {space.name}")
        extent = extents[frame]
        require(extent["start_min"] <= pos <= extent["start_max"]
                and space.data[pos - 4:pos + 4].hex() == window,
                f"Changed boundary bytes in {space.name}:{pos:04x}")
        extent.update(reviewed_start=pos, boundary_window_hex=window)
        extents[frames[frames.index(frame) - 1]]["reviewed_end_exclusive"] = pos
    return extents


def can_coexist(source, target):
    """Manager at 0249..0275 evicts any resident section overlapping the new allocation."""
    a, b = source.link_segment, target.link_segment
    return a + source.descriptor["f8"] <= b or b + target.descriptor["f8"] <= a


def near_call_sites(space, relocated, base_of):
    """`0E E8 rel16` with no relocation word in the five bytes.

    IP arithmetic wraps at 64K inside the caller's segment, whose base is
    taken from the caller's frame.
    """
    data = space.data
    at = data.find(b"\x0e\xe8")
    while at != -1:
        base = base_of(at)
        if base is not None and at + 4 <= len(data) and not relocated.intersection(range(at - 1, at + 4)):
            ip = (at - base + 4 + struct.unpack_from("<H", data, at + 2)[0]) & 0xffff
            target = base + ip
            if 0 <= target < len(data):
                yield at, target
        at = data.find(b"\x0e\xe8", at + 1)


def overlay_frame_candidates(overlays, extents, segment, offset):
    for name, target in overlays.items():
        extent = extents[name].get(segment)
        if extent and held_in(target, segment, offset):
            pos = target.position(segment, offset)
            start = extent.get("reviewed_start", extent["start_min"])
            end = extent.get("reviewed_end_exclusive", extent["end_exclusive_max"])
            if start <= pos < end:
                yield target


def attribute_overlay_call(source, call, overlays, extents):
    candidates = list(overlay_frame_candidates(overlays, extents, call["segment"], call["offset"]))
    if source in candidates:
        # The direct instruction neither calls the manager nor changes the
        # overlay at the destination. The caller's copy is already loaded.
        return source.name, "same_overlay", [source.name]

    possible = [s.name for s in candidates
                if not source.descriptor or can_coexist(source, s)]
    if len(possible) == 1:
        return possible[0], "unique_compatible_frame", possible
    return None, "unresolved", possible


def overlay_pointer_candidates(spaces, overlays, extents):
    """Relocated records from Prince's main data contribution at overlay-17:30f7.

    A two-word pointer shape witnesses a candidate entry, without proving
    that the program calls it. Some of the contribution contains data pointers.
    """
    source = spaces.get("overlay-17")
    if source is None:
        return []
    rows = []
    for record in source.relocations:
        p = record["image_offset"]
        if record["segment"] != 0x30f7 or p < 2:
            continue
        offset = struct.unpack_from("<H", source.data, p - 2)[0]
        segment = record["stored_word"]
        hits = list(overlay_frame_candidates(overlays, extents, segment, offset))
        if len(hits) == 1:
            target, = hits
            rows.append({"from": source.name, "position": p - 2, "relocation_index": record["index"],
                         "segment": segment, "offset": offset, "target_space": target.name,
                         "target_position": target.position(segment, offset)})
    return sorted(rows, key=lambda row: row["position"])


def census(oracle):
    spaces = oracle.spaces
    root = spaces["root"]
    manifest = read_json(ROOT / "layout/manifest.json")
    targets = read_json(ROOT / "evidence/targets.json")["functions"]
    # On this locked image descriptor 17 has flag 01h and is preloaded data
    # / driver material. Merely bearing a relocation does not make it CODE.
    overlays = {name: s for name, s in spaces.items() if s.descriptor and s.descriptor["flags"] == 4}
    groups = {name: relocation_frames(s) for name, s in overlays.items()}
    entries = collections.defaultdict(lambda: collections.defaultdict(set))
    callers = collections.defaultdict(lambda: collections.defaultdict(collections.Counter))
    call_reasons = collections.defaultdict(lambda: collections.defaultdict(collections.Counter))
    vectors = overlay_vectors(oracle, overlays)
    for v in vectors.values():
        entries[v["target_space"]][v["target_segment"]].add(v["target_position"])
    extents = {name: frame_extents(s, groups[name], entries[name]) for name, s in overlays.items()}
    for v in vectors.values():
        possible = overlay_frame_candidates(overlays, extents, v["target_segment"], v["target_offset"])
        require(overlays[v["target_space"]] in possible, "Vector target does not fit its overlay frame")
    pointer_candidates = overlay_pointer_candidates(spaces, overlays, extents)
    pointer_entries = collections.defaultdict(lambda: collections.defaultdict(set))
    for row in pointer_candidates:
        pointer_entries[row["target_space"]][row["segment"]].add(row["target_position"])

    attributions = []
    ambiguous = []

    def record_entry(source, name, segment, offset, rule):
        entries[name][segment].add(spaces[name].position(segment, offset))
        callers[name][segment][source] += 1
        call_reasons[name][segment][rule] += 1

    for space in spaces.values():
        direct_relocs = set()
        for call in far_call_sites(space):
            segment, offset = call["segment"], call["offset"]
            direct_relocs.add(call["relocation_index"])
            evidence = {"from": space.name, **call}
            if held_in(root, segment, offset):
                record_entry(space.name, "root", segment, offset, "resident")
                v = vectors.get((segment, offset))
                if v:
                    v["callers"].append(evidence)
                    record_entry(space.name, v["target_space"], v["target_segment"], v["target_offset"],
                                 "rtlink_vector")
                continue
            name, rule, possible = attribute_overlay_call(space, call, overlays, extents)
            if space is root and call["site"] in REVIEWED_ABI_CALLS:
                reviewed, start, window = REVIEWED_ABI_CALLS[call["site"]]
                require(name is None and (segment, offset) == (0x2a4e, 0)
                        and set(possible) == {"overlay-8", "overlay-9"}
                        and root.data[start:start + len(window) // 2].hex() == window,
                        "Changed overlay-call ABI witness")
                require(overlays["overlay-8"].data[0x123:0x124] == b"\xcb"
                        and overlays["overlay-9"].data[0x020:0x023] == b"\xca\x02\x00",
                        "Changed overlay callee-pop ABI")
                evidence["compatible_before_abi_review"] = possible
                name, rule, possible = reviewed, "reviewed_callee_pop_abi", [reviewed]
            evidence.update(rule=rule, spaces=possible)
            if name is None:
                evidence["physical_spaces"] = [s.name for s in spaces.values() if held_in(s, segment, offset)]
                evidence["candidate_entry_bytes"] = {}
                for candidate in possible:
                    dest = spaces[candidate]
                    pos = dest.position(segment, offset)
                    evidence["candidate_entry_bytes"][candidate] = dest.data[pos:pos + 8].hex()
                ambiguous.append(evidence)
            else:
                record_entry(space.name, name, segment, offset, rule)
                attributions.append(evidence)

        for r in space.relocations:
            p = r["image_offset"]
            if p < 2 or r["index"] in direct_relocs:
                continue
            offset = struct.unpack_from("<H", space.data, p - 2)[0]
            v = vectors.get((r["stored_word"], offset))
            if v:
                v["other_relocated_references"].append({
                    "from": space.name, "position": p - 2, "relocation_index": r["index"],
                })

    report = {
        "schema": 2, "target_sha256": oracle.profile["target_sha256"],
        "authority": "Original relocation records and bytes only. Frame census is research evidence.",
        "attribution_rules": {
            "rtlink_vector": "Resident near CALL to 2203:0558, far JMP, zero-based descriptor index. "
                             "0558 loads [return IP+5]; 0587 uses 0b1b + 18*index.",
            "same_overlay": "Immediate direct call names a frame in the caller's loaded overlay.",
            "unique_compatible_frame": "Only one witnessed code-overlay frame covers this direct address "
                                       "and can coexist with the caller. This assumes a relocation-bearing "
                                       "callee frame and valid preloading; it is a static attribution.",
            "reviewed_callee_pop_abi": "Two original root sites at 453a and c13b call 2a4e:0000 "
                                       "with zero and one word respectively. Overlay 8 ends in RETF, "
                                       "and overlay 9 ends in RETF 2.",
            "unresolved": "Direct call encodes no descriptor index. Occupancy/address alone does not "
                          "distinguish the remaining compatible frames.",
            "extent": "Overlay-relative bounds. Internal starts are within the frame paragraph, after "
                      "the previous frame's last fixup and before this frame's first fixup or vector entry. "
                      "Reviewed transitions refine internal starts and ends. The final end is the physical "
                      "payload limit and may include fill.",
        },
        "ambiguous_far_calls": len(ambiguous), "spaces": [],
        "overlay_call_attributions": attributions, "overlay_vectors": list(vectors.values()),
        "overlay_data_pointer_candidates": pointer_candidates,
    }
    for space in spaces.values():
        frames = list(groups[space.name]) if space.name in groups else sorted(entries[space.name])
        bases = [(f - space.link_segment) * 16 for f in frames]
        starts = [extents.get(space.name, {}).get(f, {}).get("reviewed_start", base)
                  for f, base in zip(frames, bases)]
        relocated = {r["image_offset"] + k for r in space.relocations for k in (0, 1)}

        def frame_of(pos):
            i = bisect.bisect_right(starts, pos) - 1
            return frames[i] if i >= 0 else None

        near = collections.defaultdict(lambda: {"internal": 0, "outgoing": 0, "incoming": 0})
        spans = collections.defaultdict(list)
        near_entries = collections.defaultdict(set)
        cross = []

        def base_of(pos):
            i = bisect.bisect_right(starts, pos) - 1
            return bases[i] if i >= 0 else None

        for site, target in near_call_sites(space, relocated, base_of):
            a, b = frame_of(site), frame_of(target)
            if a is None or b is None:
                continue
            if a == b:
                near[a]["internal"] += 1
                spans[a].append((min(site, target), max(site, target)))
                near_entries[a].add(target)
            else:
                near[a]["outgoing"] += 1
                near[b]["incoming"] += 1
                cross.append({"site": site, "target": target, "from": a, "to": b})
        owners = collections.defaultdict(list)
        for o in manifest["owners"]:
            if o["space"] == space.name:
                pos = space.position(o["segment"], o["offset"])
                owners[frame_of(pos)].append({"target_id": o["target_id"], "position": pos, "size": o["size"]})
        reviewed = collections.defaultdict(int)
        for t in targets:
            if t["space"] == space.name and t.get("boundary_status") == "REVIEWED":
                reviewed[frame_of(space.position(t["segment"], t["offset"]))] += 1
        rows = []
        for i, frame in enumerate(frames):
            base = bases[i]
            end = starts[i + 1] if i + 1 < len(frames) else space.size
            pos = sorted(entries[space.name][frame])
            overrun = [p for p in pos if p >= end]
            first = pos[0] if pos else None
            # Bytes between the frame paragraph and its first called entry: fill or uncalled code.
            lead = space.data[base:first] if first is not None and base <= first else b""
            tail = space.data[max(base, end - 4):end]
            # A file is contiguous, so each same-file call interval lies inside one file.
            # Gaps between merged intervals are necessary (not sufficient) file boundaries.
            blocks = []
            for lo, hi in sorted(spans[frame]):
                if blocks and lo <= blocks[-1][1]:
                    blocks[-1][1] = max(blocks[-1][1], hi)
                else:
                    blocks.append([lo, hi])
            row = {
                "frame": f"{frame:04x}", "base": base, "end": end, "span": end - base,
                "entries": len(pos), "entry_positions": pos,
                "first_entry": first, "last_entry": pos[-1] if pos else None,
                "entries_beyond_next_frame": len(overrun),
                "incoming_far_calls": sum(callers[space.name][frame].values()),
                "callers_by_space": dict(callers[space.name][frame]),
                "calls_by_attribution_rule": dict(call_reasons[space.name][frame]),
                "near_calls": near[frame], "near_call_blocks": blocks,
                "near_call_entry_positions": sorted(near_entries[frame]),
                "lead_hex": lead[:16].hex(), "tail_hex": tail.hex(),
                "owners": owners.get(frame, []), "reviewed_targets": reviewed.get(frame, 0),
                "owned_bytes": sum(o["size"] for o in owners.get(frame, [])),
            }
            if space.name in groups:
                records = groups[space.name][frame]
                row.update(extent=extents[space.name][frame],
                           source_relocation_count=len(records),
                           first_relocation_position=min(r["image_offset"] for r in records),
                           last_relocation_end=max(r["image_offset"] + 2 for r in records),
                           data_pointer_entry_candidates=sorted(pointer_entries[space.name][frame]))
            rows.append(row)

        space_row = {"space": space.name, "link_segment": f"{space.link_segment:04x}",
                     "size": space.size, "frames": rows, "cross_frame_near_calls": cross,
                     "owners_before_first_frame": owners.get(None, [])}
        if space.descriptor:
            space_row["descriptor"] = space.descriptor
        if space.name != "root" and space.name not in groups:
            space_row["classification"] = "Preloaded data / driver payload; code frames unclassified."
            space_row["unclassified_relocation_site_segments"] = {
                f"{seg:04x}": len(records) for seg, records in relocation_frames(space).items()
            }
        report["spaces"].append(space_row)
    report["ambiguous"] = ambiguous
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    report = census(Oracle.load())
    write_json(project_path(args.output), report)
    for s in report["spaces"]:
        internal = sum(r["near_calls"]["internal"] for r in s["frames"])
        print(f"{s['space']:11} link={s['link_segment']} frames={len(s['frames']):3} "
              f"near internal={internal:5} cross={len(s['cross_frame_near_calls']):4}")
        for r in s["frames"]:
            n = r["near_calls"]
            print(f"  {r['frame']} base={r['base']:06x} span={r['span']:6} entries={r['entries']:3} "
                  f"over={r['entries_beyond_next_frame']:2} near i/o/in={n['internal']}/{n['outgoing']}/"
                  f"{n['incoming']} owned={r['owned_bytes']:4}/{len(r['owners'])} "
                  f"lead={r['lead_hex'][:12]} tail={r['tail_hex']}")
    print(f"Ambiguous far calls: {report['ambiguous_far_calls']}; written {args.output}")


if __name__ == "__main__":
    main()
