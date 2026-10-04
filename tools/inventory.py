"""Freeze asset identities once; later invocations verify, never refresh them."""
import argparse
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

from common import ROOT, read_json, require, sha, write_json
from mz import MZ

LOCK = ROOT / "layout/oracle.lock.json"


def inventory(directory):
    files = []
    for path in sorted(directory.rglob("*")):
        if not path.is_file():
            continue
        data = path.read_bytes()
        suffix = path.suffix.lower()
        kind = {".dat": "resource_or_configuration", ".drv": "external_sound_driver",
                ".sav": "save_state", ".opt": "configuration"}.get(suffix, "support")
        row = {"path": path.relative_to(directory).as_posix(), "size": len(data),
               "sha256": sha(data), "kind": kind,
               "observed_mtime_utc": datetime.fromtimestamp(path.stat().st_mtime, timezone.utc).isoformat()}
        if data[:2] == b"MZ":
            row["kind"] = "dos_mz_executable"
            mz = MZ.parse(data)
            row["mz"] = {k: v for k, v in mz.describe(data).items() if k != "relocations"}
        files.append(row)
    return {"schema": 1, "files": files, "file_count": len(files),
            "total_bytes": sum(r["size"] for r in files),
            "kinds": dict(Counter(r["kind"] for r in files))}


def verify_assets():
    require(LOCK.is_file(), "Missing frozen oracle lock; run inventory.py --freeze once")
    lock = read_json(LOCK)
    expected = {r["path"]: (r["size"], r["sha256"]) for r in lock["assets"]["files"]}
    actual = inventory(ROOT / "assets")
    observed = {r["path"]: (r["size"], r["sha256"]) for r in actual["files"]}
    require(expected == observed, "Asset identities differ from frozen oracle.lock.json")
    return lock


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--freeze", action="store_true")
    args = parser.parse_args()
    if args.freeze:
        require(not LOCK.exists(), "Refusing to overwrite frozen oracle lock")
        inv = inventory(ROOT / "assets")
        target = next(r for r in inv["files"] if r["path"] == "PRINCE.EXE")
        write_json(LOCK, {"schema": 1, "authority": "Immutable supplied assets; never candidate output",
                          "target": {"path": "assets/PRINCE.EXE", "size": target["size"],
                                     "sha256": target["sha256"]}, "assets": inv})
    lock = verify_assets()
    print(f"Verified {lock['assets']['file_count']} assets, {lock['assets']['total_bytes']} bytes")
    print(f"PRINCE.EXE {lock['target']['sha256']}")


if __name__ == "__main__":
    main()
