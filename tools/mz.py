"""Bounds-checked 16-bit DOS MZ parser, without overlay-format assumptions."""
from dataclasses import dataclass
import struct

from common import require, sha

FIELDS = ("magic", "last_page_bytes", "pages", "relocation_count", "header_paragraphs",
          "min_extra_paragraphs", "max_extra_paragraphs", "initial_ss", "initial_sp",
          "checksum", "initial_ip", "initial_cs", "relocation_table_offset", "overlay_number")


@dataclass(frozen=True)
class MZ:
    header: dict
    header_size: int
    file_image_size: int
    relocations: tuple

    @classmethod
    def parse(cls, data):
        require(len(data) >= 28, "Truncated MZ header")
        h = dict(zip(FIELDS, struct.unpack_from("<14H", data)))
        require(h["magic"] == 0x5a4d, "Not an MZ executable")
        require(h["pages"] > 0 and h["last_page_bytes"] < 512, "Invalid MZ page count")
        size = h["pages"] * 512
        if h["last_page_bytes"]:
            size -= 512 - h["last_page_bytes"]
        header_size = h["header_paragraphs"] * 16
        require(28 <= header_size <= size <= len(data), "MZ image/header outside file")
        rt = h["relocation_table_offset"]
        require(rt >= 28 and rt + 4 * h["relocation_count"] <= header_size,
                "MZ relocation table outside header")
        rows = []
        for i in range(h["relocation_count"]):
            offset, segment = struct.unpack_from("<HH", data, rt + i * 4)
            linear = segment * 16 + offset
            require(linear + 2 <= size - header_size, "MZ relocation outside load image")
            rows.append({"index": i, "segment": segment, "offset": offset,
                         "image_offset": linear,
                         "stored_word": struct.unpack_from("<H", data, header_size + linear)[0]})
        return cls(h, header_size, size, tuple(rows))

    def load_image(self, data):
        return data[self.header_size:self.file_image_size]

    def describe(self, data):
        image = self.load_image(data)
        return {"header": self.header, "header_size": self.header_size,
                "file_image_size": self.file_image_size, "load_image_size": len(image),
                "load_image_sha256": sha(image), "appended_size": len(data)-self.file_image_size,
                "entry": {"space": "root", "segment": self.header["initial_cs"],
                          "offset": self.header["initial_ip"],
                          "unwrapped_image_offset": self.header["initial_cs"]*16+self.header["initial_ip"]},
                "relocations": list(self.relocations)}
