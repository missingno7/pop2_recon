"""Immutable segmented binary oracle. File, link and runtime addresses stay distinct."""
import argparse
from dataclasses import dataclass
import struct

from common import ROOT, read_json, require, sha, write_json
from inventory import verify_assets
from mz import MZ


@dataclass(frozen=True)
class Space:
    name: str
    link_segment: int
    file_offset: int
    data: bytes
    relocations: tuple
    descriptor: dict | None = None

    @property
    def size(self):
        return len(self.data)

    def position(self, segment, offset):
        """Link address to space-relative position; never picks a different overlay."""
        require(0 <= segment <= 0xffff and 0 <= offset <= 0xffff, "Invalid 16:16 address")
        pos = (segment - self.link_segment) * 16 + offset
        require(0 <= pos < self.size, f"Address outside {self.name}")
        return pos

    def address(self, position):
        require(0 <= position < self.size, f"Position outside {self.name}")
        # Canonical segment at each 64K window; retain explicit aliases in function evidence.
        segment = self.link_segment + (position // 65536) * 4096
        return {"space": self.name, "segment": segment, "offset": position % 65536}

    def extent(self, segment, offset, size):
        pos = self.position(segment, offset)
        require(size > 0 and pos + size <= self.size, "Extent outside space")
        return self.data[pos:pos + size]

    def runtime_address(self, segment, offset, load_segment):
        self.position(segment, offset)
        require(0 <= load_segment <= 0xffff, "Invalid runtime load segment")
        return {"space": self.name, "segment": (segment + load_segment) & 0xffff, "offset": offset,
                "load_segment": load_segment}

    def describe(self):
        return {"name": self.name, "link_segment": self.link_segment, "file_offset": self.file_offset,
                "size": self.size, "sha256": sha(self.data), "relocations": list(self.relocations),
                "descriptor": self.descriptor}


class Oracle:
    def __init__(self, data, profile):
        self.data = data
        self.mz = MZ.parse(data)
        self.profile = profile
        self.spaces = {"root": Space("root", 0, self.mz.header_size,
                                     self.mz.load_image(data), self.mz.relocations)}
        self.gaps = []
        self._parse_overlays()

    @classmethod
    def load(cls):
        lock = verify_assets()
        profile = read_json(ROOT / "layout/oracle-format.json")
        data = (ROOT / lock["target"]["path"]).read_bytes()
        require(sha(data) == lock["target"]["sha256"] == profile["target_sha256"],
                "Oracle/profile target identity mismatch")
        return cls(data, profile)

    def _parse_overlays(self):
        p = self.profile
        table, count = p["descriptor_table_file_offset"], p["descriptor_count"]
        fmt = "<HHHBBHHHHH"
        record_size = struct.calcsize(fmt)
        require(p["record_format"] == fmt, "Unsupported descriptor format")
        require(self.mz.header_size <= table and table + count*record_size <= self.mz.file_image_size,
                "Descriptor table outside resident image")
        fields = ("link_segment", "f2", "position_paragraph_low", "position_paragraph_high", "flags",
                  "f8", "relocation_count", "fc", "overlay_id", "size_paragraphs")
        cursor = self.mz.file_image_size
        for i in range(count):
            descriptor = dict(zip(fields, struct.unpack_from(fmt, self.data, table + i*record_size)))
            descriptor["table_index"] = i
            descriptor["file_offset"] = table + i*record_size
            overlay_id = descriptor["overlay_id"]
            require(overlay_id == p["first_overlay_id"] + i, "Descriptor overlay sequence changed")
            seg = descriptor["link_segment"]
            nrel = descriptor["relocation_count"]
            rpos = (descriptor["position_paragraph_low"] + descriptor["position_paragraph_high"]*65536)*16
            code = rpos + ((nrel + 3)//4)*16
            allocated_size = descriptor["size_paragraphs"]*16
            shortfall = p.get("payload_shortfall_by_overlay", {}).get(str(overlay_id), 0)
            size = allocated_size - shortfall
            require(size > 0 and cursor <= rpos <= code < code+size <= len(self.data),
                    "Overlay payload overlaps/outside file")
            require(shortfall == 0 or (i == count-1 and code+size == len(self.data)),
                    "Known paragraph shortfall is only valid at physical EOF")
            descriptor.update(allocated_size=allocated_size, file_payload_size=size,
                              absent_final_paragraph_bytes=shortfall)
            if cursor < rpos:
                self.gaps.append({"file_offset": cursor, "size": rpos-cursor,
                                  "sha256": sha(self.data[cursor:rpos]), "kind": "unowned_file_gap"})
            rows = []
            for j in range(nrel):
                offset, segment = struct.unpack_from("<HH", self.data, rpos + j*4)
                pos = (segment-seg)*16 + offset
                require(0 <= pos and pos+2 <= size, f"Relocation outside overlay {overlay_id}")
                rows.append({"index": j, "segment": segment, "offset": offset,
                             "image_offset": pos,
                             "stored_word": struct.unpack_from("<H", self.data, code+pos)[0]})
            descriptor.update(relocation_file_offset=rpos, payload_file_offset=code,
                              relocation_padding_size=code-rpos-nrel*4,
                              relocation_padding_sha256=sha(self.data[rpos+nrel*4:code]))
            name = f"overlay-{overlay_id}"
            self.spaces[name] = Space(name, seg, code, self.data[code:code+size], tuple(rows), descriptor)
            cursor = code+size
        if cursor < len(self.data):
            self.gaps.append({"file_offset": cursor, "size": len(self.data)-cursor,
                              "sha256": sha(self.data[cursor:]), "kind": "unowned_file_tail"})

    def describe(self):
        return {"schema": 1, "target_sha256": sha(self.data), "target_size": len(self.data),
                "mz": self.mz.describe(self.data), "format_profile": self.profile,
                "spaces": [space.describe() for space in self.spaces.values()], "gaps": self.gaps,
                "authority": "Only immutable original bytes; no candidate input"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--extract", action="store_true", help="Write unrelocated oracle fixtures under build/oracle")
    args = parser.parse_args()
    oracle = Oracle.load()
    out = ROOT / "build/oracle"
    write_json(out / "oracle.json", oracle.describe())
    if args.extract:
        out.mkdir(parents=True, exist_ok=True)
        for space in oracle.spaces.values():
            (out / (space.name + ".bin")).write_bytes(space.data)
    for space in oracle.spaces.values():
        print(f"{space.name:12} link={space.link_segment:04x} file={space.file_offset:6} "
              f"bytes={space.size:6} relocations={len(space.relocations):4}")
    print(f"Unowned file gaps/tail: {sum(r['size'] for r in oracle.gaps)} bytes")


if __name__ == "__main__":
    main()
