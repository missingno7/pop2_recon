"""Independent pinned-library member acceptance tests."""
import tempfile
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from common import sha
from runtime import verify_member


def omf_record(kind, body=b""):
    length = len(body) + 1
    prefix = bytes((kind,)) + length.to_bytes(2, "little") + body
    return prefix + bytes(((-sum(prefix)) & 0xFF,))


def runtime_omf(code=b"\xCB", *, code_length=None, data_chunks=None,
               extra_segments=(), extra_data_chunks=(), externals=(), actual_fixup=False):
    """Build one checksummed MSC-style OMF contribution for this test module."""
    segments = [("_TEXT", "CODE", len(code) if code_length is None else code_length)]
    segments.extend(extra_segments)
    names = []
    for name, cls, _ in segments:
        for value in (name, cls):
            if value not in names:
                names.append(value)
    name_index = {name: i + 1 for i, name in enumerate(names)}
    lnames = b"".join(bytes((len(name),)) + name.encode("ascii") for name in names)
    records = [omf_record(0x80, b"\x04test"), omf_record(0x96, lnames)]
    for name, cls, length in segments:
        body = (bytes((0x28,)) + length.to_bytes(2, "little") +
                bytes((name_index[name], name_index[cls], 0)))
        records.append(omf_record(0x98, body))
    if externals:
        body = b"".join(bytes((len(name),)) + name.encode("ascii") + b"\x00"
                         for name in externals)
        records.append(omf_record(0x8C, body))
    public = b"\x07_target\x00\x00\x00"
    records.append(omf_record(0x90, b"\x00\x01" + public))
    chunks = [(0, code)] if data_chunks is None else data_chunks
    for offset, payload in chunks:
        records.append(omf_record(0xA0, b"\x01" + offset.to_bytes(2, "little") + payload))
    for segment_index, offset, payload in extra_data_chunks:
        records.append(omf_record(0xA0, bytes((segment_index,)) +
                                  offset.to_bytes(2, "little") + payload))
    if actual_fixup:
        # Valid one-byte relocation at the start of the one-byte CODE extent.
        locat = 0x8000
        records.append(omf_record(0x9C, bytes((locat >> 8, locat & 0xFF, 0x44, 0x01))))
    records.append(omf_record(0x8A))
    return b"".join(records)


class RuntimeSpace:
    name = "root"

    def __init__(self, data=b"\xCB", relocations=()):
        self.data = data
        self.relocations = tuple(relocations)

    def position(self, segment, offset):
        if segment != 1 or not 0 <= offset < len(self.data):
            raise ValueError("address outside runtime fixture")
        return offset

    def extent(self, segment, offset, size):
        start = self.position(segment, offset)
        if start + size > len(self.data):
            raise ValueError("extent outside runtime fixture")
        return self.data[start:start + size]


class RuntimeMemberTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.oracle = SimpleNamespace(spaces={"root": RuntimeSpace()})
        self._counter = 0

    def tearDown(self):
        self.temp.cleanup()

    def fixture(self, obj=None, *, target=b"\xCB", relocations=(), recipe_changes=None):
        obj = runtime_omf() if obj is None else obj
        self.oracle.spaces["root"] = RuntimeSpace(target, relocations)
        prefix, suffix = b"LIBHDR", b"LIBTAIL"
        library = prefix + obj + suffix
        path = self.root / f"synthetic-{self._counter}.lib"
        self._counter += 1
        path.write_bytes(library)
        recipe = {
            "name": "runtime_test",
            "library_path": str(path),
            "library_sha256": sha(library),
            "archive_offset": len(prefix),
            "member_extent": len(obj),
            "member_sha256": sha(obj),
            "object_segment": "_TEXT",
            "size": len(target),
            "publics": ["_target"],
            "space": "root",
            "segment": 1,
            "offset": 0,
            "target_sha256": sha(target),
        }
        if recipe_changes:
            recipe.update(recipe_changes)
        return recipe, path, library, obj

    def test_complete_public_code_extent_matches_independent_archive_member(self):
        recipe, _, _, _ = self.fixture()
        result = verify_member(recipe, self.oracle)
        self.assertEqual(result["kind"], "PINNED_RUNTIME")
        self.assertEqual(result["state"], "PINNED_RUNTIME")
        self.assertEqual(result["size"], 1)
        self.assertEqual(result["sha256"], sha(b"\xCB"))
        self.assertEqual([p["name"] for p in result["publics"]], ["_target"])

    def test_library_and_member_identity_changes_are_rejected(self):
        recipe, path, library, obj = self.fixture()
        path.write_bytes(library[:-1] + bytes((library[-1] ^ 1,)))
        with self.assertRaisesRegex(ValueError, "Pinned runtime library identity changed"):
            verify_member(recipe, self.oracle)

        recipe, path, library, obj = self.fixture()
        changed_member = bytearray(obj)
        changed_member[-1] ^= 1
        mutated_library = library[:recipe["archive_offset"]] + bytes(changed_member) + library[
            recipe["archive_offset"] + len(obj):]
        path.write_bytes(mutated_library)
        # The library digest is updated here to isolate the independent member pin.
        recipe["library_sha256"] = sha(mutated_library)
        with self.assertRaisesRegex(ValueError, "Pinned runtime member identity changed"):
            verify_member(recipe, self.oracle)

    def test_original_relocation_is_not_satisfied_by_matching_literal_bytes(self):
        recipe, _, _, _ = self.fixture(relocations=({"image_offset": 0},))
        with self.assertRaisesRegex(ValueError, "Original runtime extent carries ungenerated relocation"):
            verify_member(recipe, self.oracle)

    def test_nonempty_secondary_storage_is_rejected(self):
        obj = runtime_omf(extra_segments=(("_DATA", "DATA", 1),),
                          extra_data_chunks=((2, 0, b"\x44"),))
        recipe, _, _, _ = self.fixture(obj)
        with self.assertRaisesRegex(ValueError, "unowned secondary storage"):
            verify_member(recipe, self.oracle)

    def test_sparse_data_hole_cannot_use_reader_zero_fill(self):
        target = b"\xCB\x00"
        obj = runtime_omf(code=target, code_length=2, data_chunks=[(0, b"\xCB")])
        recipe, _, _, _ = self.fixture(obj, target=target)
        with self.assertRaisesRegex(ValueError, "extent is incomplete or trimmed"):
            verify_member(recipe, self.oracle)

    def test_owned_extent_cannot_be_trimmed_or_extended(self):
        recipe, _, _, _ = self.fixture(runtime_omf(code=b"\xCB\x90"))
        with self.assertRaisesRegex(ValueError, "extent is incomplete or trimmed"):
            verify_member(recipe, self.oracle)

        recipe, _, _, _ = self.fixture(runtime_omf(code=b"\xCB"), target=b"\xCB\x90")
        with self.assertRaisesRegex(ValueError, "extent is incomplete or trimmed"):
            verify_member(recipe, self.oracle)

    def test_actual_fixup_subrecord_is_rejected(self):
        obj = runtime_omf(externals=("_helper",), actual_fixup=True)
        recipe, _, _, _ = self.fixture(obj)
        with self.assertRaisesRegex(ValueError, "unsupported symbolic fixup proof"):
            verify_member(recipe, self.oracle)


if __name__ == "__main__":
    unittest.main()
