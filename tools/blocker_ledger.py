#!/usr/bin/env python3
"""Rank what unblocks the most remaining work, from all worker submissions.

Reads every build/workers/*/submission.json "blocked" list, drops entries whose target is
now owned (layout/manifest.json), marks needs that are already satisfied (the needed target
is owned), and ranks the open needs by how many blocked functions name them. Callee-first
matching has repeatedly unlocked whole runs of other files; this makes those links visible
instead of relying on someone noticing them.
"""
import argparse
import collections
import json
from glob import glob
from pathlib import Path

from common import ROOT, read_json


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", help="also write the ledger as JSON to this path")
    parser.add_argument("--top", type=int, default=25)
    args = parser.parse_args()
    owned = {o["target_id"] for o in read_json(ROOT / "layout/manifest.json")["owners"]}
    open_items, by_need, by_kind = [], collections.defaultdict(list), collections.Counter()
    for path in sorted(glob(str(ROOT / "build/workers/*/submission.json"))):
        worker = Path(path).parent.name
        try:
            sub = read_json(path)
        except (OSError, ValueError):
            continue
        for block in sub.get("blocked", []):
            if block.get("target_id") in owned:
                continue  # resolved since
            item = dict(block, worker=worker, need_owned=block.get("needs") in owned)
            open_items.append(item)
            by_kind[block.get("kind")] += 1
            if block.get("needs") and not item["need_owned"]:
                by_need[block["needs"]].append(item)
    print(f"{len(open_items)} open blocked functions by kind: {dict(by_kind)}")
    ready = [i for i in open_items if i["need_owned"]]
    if ready:
        print(f"\n{len(ready)} blocked entries whose need is now owned (retry these first):")
        for i in ready:
            print(f"  {i['target_id']} ({i['worker']}): {i['kind']} -> {i['needs']}")
    print(f"\nTop needs by number of blocked dependents:")
    for need, items in sorted(by_need.items(), key=lambda kv: -len(kv[1]))[:args.top]:
        kinds = collections.Counter(i["kind"] for i in items)
        print(f"  {len(items):3d}  {need}  {dict(kinds)}  e.g. {items[0]['target_id']} ({items[0]['worker']})")
    if args.json:
        Path(args.json).write_text(json.dumps({"open": open_items,
                                               "needs": {k: len(v) for k, v in by_need.items()}}, indent=2) + "\n")


if __name__ == "__main__":
    main()
