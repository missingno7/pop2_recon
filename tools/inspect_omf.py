"""Print a strict structural report for one 16-bit OMF object."""
from __future__ import annotations
import argparse
import json
from pathlib import Path
from omf import OmfReader

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("object", type=Path)
    parser.add_argument("--include-record-bytes", action="store_true")
    args = parser.parse_args()
    module = OmfReader().read(args.object.read_bytes(), args.object.name)
    report = {
        "name": module.name,
        "segments": module.segment_defs,
        "initialized_ranges": module.initialized_ranges,
        "segment_bytes_hex": {name: data.hex() for name, data in module.segments.items()},
        "publics": module.publics,
        "externals": module.externals,
        "groups": module.groups,
        "fixups": module.fixups,
        "record_types": [record["type"] for record in module.records],
    }
    if args.include_record_bytes:
        report["records"] = module.records
    print(json.dumps(report, indent=2))

if __name__ == "__main__":
    main()
