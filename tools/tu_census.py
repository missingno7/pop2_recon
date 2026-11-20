"""Research census of code frames as candidate source-file (TU) boundaries.

Under MSC medium model every .C file gets its own NAME_TEXT segment, so a far
call's relocated segment word names the frame of the callee's file. This tool
collects those frames from original relocation records only, attributes every
`PUSH CS; CALL near` site and existing owner to a frame, and reports boundary
fill. Output is a hypothesis for review; it grants no ownership.
"""
import argparse
import bisect
import collections
import json
import struct

from common import ROOT, project_path, read_json, write_json
from oracle import Oracle

DEFAULT_OUTPUT = "build/workers/tu_census/census.json"


def far_call_sites(space):
    """Immediate CALL FAR sites whose segment word is an original relocation."""
    for r in space.relocations:
        at = r["image_offset"] - 3
        if at >= 0 and space.data[at] == 0x9a:
            offset, segment = struct.unpack_from("<HH", space.data, at + 1)
            yield {"site": at, "segment": segment, "offset": offset, "relocation_index": r["index"]}


def resolve(spaces, segment, offset, preferred):
    """Spaces holding segment:offset; the caller's own space wins over aliases."""
    hits = []
    for space in spaces.values():
        pos = (segment - space.link_segment) * 16 + offset
        if 0 <= pos < space.size:
            hits.append(space.name)
    if preferred in hits:
        return [preferred]
    return hits


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


def census(oracle):
    spaces = oracle.spaces
    manifest = read_json(ROOT / "layout/manifest.json")
    targets = read_json(ROOT / "evidence/targets.json")["functions"]
    entries = collections.defaultdict(lambda: collections.defaultdict(set))   # space -> frame -> positions
    callers = collections.defaultdict(lambda: collections.defaultdict(int))   # space -> frame -> incoming far calls
    ambiguous = []
    for space in spaces.values():
        for call in far_call_sites(space):
            hits = resolve(spaces, call["segment"], call["offset"], space.name)
            if len(hits) != 1:
                # Overlays sharing a link segment: the word alone cannot pick the space.
                ambiguous.append({"from": space.name, "site": call["site"], "segment": call["segment"],
                                  "offset": call["offset"], "spaces": hits})
                continue
            name = hits[0]
            pos = (call["segment"] - spaces[name].link_segment) * 16 + call["offset"]
            entries[name][call["segment"]].add(pos)
            callers[name][call["segment"]] += 1
    report = {"schema": 1, "authority": "Original relocation records and bytes only; research hypothesis",
              "ambiguous_far_calls": len(ambiguous), "spaces": []}
    for space in spaces.values():
        frames = sorted(entries[space.name])
        if not frames:
            continue
        bases = [(f - space.link_segment) * 16 for f in frames]
        relocated = {r["image_offset"] + k for r in space.relocations for k in (0, 1)}

        def frame_of(pos):
            i = bisect.bisect_right(bases, pos) - 1
            return frames[i] if i >= 0 else None

        near = collections.defaultdict(lambda: {"internal": 0, "outgoing": 0, "incoming": 0})
        spans = collections.defaultdict(list)
        cross = []
        def base_of(pos):
            i = bisect.bisect_right(bases, pos) - 1
            return bases[i] if i >= 0 else None

        for site, target in near_call_sites(space, relocated, base_of):
            a, b = frame_of(site), frame_of(target)
            if a is None or b is None:
                continue
            if a == b:
                near[a]["internal"] += 1
                spans[a].append((min(site, target), max(site, target)))
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
            end = bases[i + 1] if i + 1 < len(frames) else space.size
            pos = sorted(entries[space.name][frame])
            overrun = [p for p in pos if p >= end]
            first = pos[0]
            # Bytes between the frame paragraph and its first called entry: fill or uncalled code.
            lead = space.data[base:first] if base <= first else b""
            tail = space.data[max(base, end - 4):end]
            # A file is contiguous, so each same-file call interval lies inside one file.
            # Gaps between merged intervals are necessary (not sufficient) file boundaries.
            blocks = []
            for lo, hi in sorted(spans[frame]):
                if blocks and lo <= blocks[-1][1]:
                    blocks[-1][1] = max(blocks[-1][1], hi)
                else:
                    blocks.append([lo, hi])
            rows.append({
                "frame": f"{frame:04x}", "base": base, "end": end, "span": end - base,
                "entries": len(pos), "first_entry": first, "last_entry": pos[-1],
                "entries_beyond_next_frame": len(overrun),
                "incoming_far_calls": callers[space.name][frame],
                "near_calls": near[frame], "near_call_blocks": blocks,
                "lead_hex": lead[:16].hex(), "tail_hex": tail.hex(),
                "owners": owners.get(frame, []), "reviewed_targets": reviewed.get(frame, 0),
                "owned_bytes": sum(o["size"] for o in owners.get(frame, [])),
            })
        report["spaces"].append({"space": space.name, "link_segment": f"{space.link_segment:04x}",
                                 "size": space.size, "frames": rows, "cross_frame_near_calls": cross,
                                 "owners_before_first_frame": owners.get(None, [])})
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
