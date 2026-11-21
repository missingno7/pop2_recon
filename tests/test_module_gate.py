"""Same-source group gate: one placement, per-claim judgement, outside witnesses, safe migration."""
import copy
import struct
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import common
import publication
from compiler import compile_c, load_lock

ROOT = Path(__file__).resolve().parents[1]
FLAGS = ["/c", "/AM", "/Oe", "/Gs", "/Zl", "/Gc"]
CALLER, CALLEE = "root:0000:0e78", "root:0000:0e8e"
DECLS = """extern unsigned char STATE_BYTE;
extern unsigned char STATE_BASE;
extern unsigned char STATE_DELTA;
char far pascal UPDATE_STEP_DELTA(void);
"""
CALLER_BODY = """
char far pascal INCREMENT_STATE_AND_UPDATE(void)
{
    int next = (signed char)STATE_BYTE + 1;
    STATE_BYTE = (next > 12) ? 12 : next;
    return UPDATE_STEP_DELTA();
}
"""
CALLEE_BODY = """
char far pascal UPDATE_STEP_DELTA(void)
{
    STATE_DELTA = STATE_BYTE - STATE_BASE;
    return STATE_DELTA;
}
"""
BINDING = ROOT / "tests/fixtures/state_step_pair.binding.json"  # frozen pair binding (witnesses outside 0e78..0e9a)


@unittest.skipUnless(Path(load_lock()["runner"]["path"]).is_file(), "Pinned historical runner unavailable")
class GroupGateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        cls.oracle = Oracle.load()
        cls.objects = {}
        for label, text in (("forward", DECLS + CALLER_BODY + CALLEE_BODY),
                            ("backward", DECLS + CALLEE_BODY + CALLER_BODY)):
            result = compile_c(text, "msc600", FLAGS, workdir=ROOT / "build/workers/tests/group" / label)
            assert result.ok, result.log
            cls.objects[label] = result.obj_bytes

    def check(self, claims, *, label="forward", binding=None, group_id="group:state_step_pair"):
        from module_gate import check_group
        from binding import declarations, fixes
        from omf import OmfReader
        binding = binding if binding is not None else common.read_json(BINDING)
        # The test sources keep their own identifiers; witnesses come from the canonical binding.
        module = OmfReader().read(self.objects[label])
        binding = dict(binding, target_id="group:state_step_pair",
                       declarations=declarations(module), fixups=fixes(module))
        return check_group(self.objects[label], [{"target_id": t, "public": p} for t, p in claims],
                           self.oracle, binding=binding, group_id=group_id)

    def test_pair_is_exact_with_near_and_data_equations(self):
        report = self.check([(CALLER, "INCREMENT_STATE_AND_UPDATE"), (CALLEE, "UPDATE_STEP_DELTA")])
        self.assertTrue(report["exact"])
        self.assertEqual(report["origin"], 0x0e78)
        near = [e for e in report["claims"][0]["equations"] if e["kind"] == "same-object near16"]
        self.assertEqual([(e["offset"], e["linked_value"], e["original_value"]) for e in near], [(18, 2, 2)])

    def test_permuted_definitions_cannot_share_the_original_placement(self):
        with self.assertRaisesRegex(ValueError, "one object placement"):
            self.check([(CALLER, "INCREMENT_STATE_AND_UPDATE"), (CALLEE, "UPDATE_STEP_DELTA")],
                       label="backward")

    def test_swapped_claim_publics_are_refused(self):
        with self.assertRaisesRegex(ValueError, "one object placement"):
            self.check([(CALLER, "UPDATE_STEP_DELTA"), (CALLEE, "INCREMENT_STATE_AND_UPDATE")])

    def test_claim_extent_must_equal_the_reviewed_target(self):
        # The 12-byte callee public claimed for the 22-byte caller target.
        report = self.check([(CALLER, "UPDATE_STEP_DELTA")])
        self.assertFalse(report["exact"])
        self.assertIn("trimming forbidden", report["claims"][0]["reason"])

    def test_data_after_a_call_into_an_unclaimed_draft_is_refused(self):
        # The draft could change DS before the caller's DGROUP accesses.
        binding = common.read_json(BINDING)
        for name in ("STATE_BASE", "STATE_DELTA"):
            del binding["symbols"][name]
        report = self.check([(CALLER, "INCREMENT_STATE_AND_UPDATE")], binding=binding)
        self.assertEqual(report["claims"][0]["state"], "REFUSED")
        self.assertIn("segment context", report["claims"][0]["reason"])
        self.assertEqual(report["drafts"], [{"public": "UPDATE_STEP_DELTA", "object_offset": 22}])

    def test_unused_grounded_symbol_is_refused(self):
        root = self.oracle.spaces["root"]
        used = {0x5c12, 0x5c13, 0x5c14}
        at = next(i for i in range(0x1000, root.size - 3) if root.data[i] == 0xa0 and
                  int.from_bytes(root.data[i+1:i+3], "little") not in used)
        binding = common.read_json(BINDING)
        binding["symbols"]["EXTRA"] = {
            "group": "DGROUP", "kind": "near-data-alias", "width": 1,
            "offset": int.from_bytes(root.data[at+1:at+3], "little"),
            "witnesses": [{"bytes_hex": root.data[at:at+3].hex(), "image_offset": at,
                           "operand_offset": 1, "space": "root", "context": "test-only extra alias"}]}
        with self.assertRaisesRegex(ValueError, "unused grounded data symbol"):
            self.check([(CALLER, "INCREMENT_STATE_AND_UPDATE"), (CALLEE, "UPDATE_STEP_DELTA")],
                       binding=binding)

    def test_data_witness_inside_the_group_object_is_refused(self):
        binding = common.read_json(BINDING)
        binding["symbols"]["STATE_BYTE"]["witnesses"].append(
            {"bytes_hex": "a0135c", "image_offset": 0x0e8e, "operand_offset": 1, "space": "root",
             "context": "inside the callee of this group"})
        with self.assertRaisesRegex(ValueError, "overlaps candidate"):
            self.check([(CALLER, "INCREMENT_STATE_AND_UPDATE"), (CALLEE, "UPDATE_STEP_DELTA")],
                       binding=binding)

    def test_binding_is_pinned_to_its_group(self):
        with self.assertRaisesRegex(ValueError, "another group"):
            self.check([(CALLER, "INCREMENT_STATE_AND_UPDATE"), (CALLEE, "UPDATE_STEP_DELTA")],
                       group_id="group:other")


def _name(text):
    return bytes([len(text)]) + text.encode("ascii")


def _record(kind, body):
    raw = bytes([kind]) + struct.pack("<H", len(body) + 1) + body
    return raw + bytes([-sum(raw) % 256])


def synthetic_object(code, publics, externals=(), site_fixes=(), *, dgroup=False):
    """One-LEDATA object. Each fix is (field offset, EXTDEF index, self-relative): LOC1, F5/T6, no D."""
    records = [_record(0x80, _name("fixture")),
               _record(0x96, b"".join(map(_name, ("", "FIX_TEXT", "CODE", "_DATA", "DATA", "DGROUP")))),
               _record(0x98, b"\x48" + struct.pack("<H", len(code)) + b"\x02\x03\x01")]
    if dgroup:
        records += [_record(0x98, b"\x48\x00\x00\x04\x05\x01"), _record(0x9a, b"\x06\xff\x02")]
    if externals:
        records.append(_record(0x8c, b"".join(_name(e) + b"\x00" for e in externals)))
    records.append(_record(0x90, b"\x00\x01" + b"".join(_name(p) + struct.pack("<H", o) + b"\x00"
                                                       for p, o in publics)))
    records.append(_record(0xa0, b"\x01\x00\x00" + code))
    if site_fixes:
        records.append(_record(0x9c, b"".join(struct.pack(">HBB", (0x8400 if rel else 0xc400) | at, 0x56, index)
                                              for at, index, rel in site_fixes)))
    records.append(_record(0x8a, b"\x00"))
    return b"".join(records)


def far_call(at, segment, offset):
    """Bytes and reviewed call-site row of a relocated CALL FAR at root+at."""
    return (struct.pack("<BHH", 0x9a, offset, segment),
            {"kind": "far_call", "space": "root", "image_offset": at,
             "relocated_segment_site": at + 3, "file_offset": at})


def synthetic_oracle(image, targets, relocations=(), source=None):
    from oracle import Space
    space = Space("root", 0, 8192 if source else 0, bytes(image), tuple(relocations))
    oracle = SimpleNamespace(spaces={"root": space}, data=source.data if source else bytes(image),
                             mz=source.mz if source else None)
    rows = [{"id": ident, "segment": segment, "offset": offset, "size": size, "space": "root",
             "sha256": common.sha(space.extent(segment, offset, size)), "boundary_status": "REVIEWED",
             "boundary_evidence": ["synthetic"], "call_sites": calls}
            for ident, segment, offset, size, calls in targets]
    return oracle, {"target_sha256": common.sha(oracle.data), "functions": rows}


class SyntheticGroupTests(unittest.TestCase):
    """Regression cases from the 2026-11-21 review; each was accepted before its fix."""

    def check(self, obj, oracle, inventory, claims, **kw):
        import match
        from module_gate import check_group
        with patch.object(match, "read_json", return_value=inventory):
            return check_group(obj, [{"target_id": t, "public": p} for t, p in claims], oracle, **kw)

    def near_fixture(self, fixes, publics=(("CALLER", 0), ("CALLEE", 6))):
        base = 32
        call, site = far_call(16, 0, base)
        image = bytearray(base) + bytes.fromhex("0ee80200cb90cb")
        image[16:21] = call
        oracle, inventory = synthetic_oracle(image, [("caller", 0, base, 6, [site]), ("callee", 0, base + 6, 1, [])],
                                             relocations=[{"image_offset": 19}])
        obj = synthetic_object(bytes.fromhex("0ee80000cb90cb"), publics, ["CALLEE"], fixes)
        return obj, oracle, inventory

    def test_near_call_control_with_witnessed_frame_is_exact(self):
        obj, oracle, inventory = self.near_fixture([(2, 1, True)])
        report = self.check(obj, oracle, inventory, [("caller", "CALLER"), ("callee", "CALLEE")])
        self.assertTrue(report["exact"], report)
        self.assertEqual(report["frame"], 0)

    def test_duplicate_fixup_fields_are_refused(self):
        obj, oracle, inventory = self.near_fixture([(2, 1, True), (2, 1, True)])
        with self.assertRaisesRegex(ValueError, "Fixup fields overlap"):
            self.check(obj, oracle, inventory, [("caller", "CALLER"), ("callee", "CALLEE")])

    def test_public_outside_the_emitted_code_is_refused(self):
        obj, oracle, inventory = self.near_fixture([(2, 1, True)], (("CALLER", 0), ("DRAFT", 6), ("CALLEE", 8)))
        with self.assertRaisesRegex(ValueError, "outside the initialized CODE"):
            self.check(obj, oracle, inventory, [("caller", "CALLER")])

    def test_code_before_the_first_public_is_refused(self):
        oracle, inventory = synthetic_oracle(bytes(32) + b"\x90\xcb", [("entry", 0, 33, 1, [])])
        with self.assertRaisesRegex(ValueError, "before the first public"):
            self.check(synthetic_object(b"\xcc\xcb", [("ENTRY", 1)]), oracle, inventory, [("entry", "ENTRY")])

    def test_near_call_without_a_witnessed_frame_is_refused(self):
        obj, oracle, inventory = self.near_fixture([(2, 1, True)])
        for row in inventory["functions"]:
            row["call_sites"] = []
        report = self.check(obj, oracle, inventory, [("caller", "CALLER"), ("callee", "CALLEE")])
        self.assertIn("witnessed original frame", report["claims"][0]["reason"])

    def test_claims_entered_through_different_frames_are_refused(self):
        # Linear placement agrees, but 0000:fff0's near call wraps to 0000:0000, not 1000:0000.
        base = 0xfff0
        first, site1 = far_call(16, 0, base)
        second, site2 = far_call(21, 0x1000, 0)
        image = bytearray(base) + bytes.fromhex("0ee80c00cb") + b"\x90" * 11 + b"\xcb"
        image[16:21], image[21:26] = first, second
        oracle, inventory = synthetic_oracle(image, [("caller", 0, base, 16, [site1]), ("callee", 0x1000, 0, 1, [site2])],
                                             relocations=[{"image_offset": 19}, {"image_offset": 24}])
        obj = synthetic_object(bytes.fromhex("0ee80000cb") + b"\x90" * 11 + b"\xcb",
                               [("CALLER", 0), ("CALLEE", 16)], ["CALLEE"], [(2, 1, True)])
        with self.assertRaisesRegex(ValueError, "different original frames"):
            self.check(obj, oracle, inventory, [("caller", "CALLER"), ("callee", "CALLEE")])
        report = self.check(obj, oracle, dict(inventory, functions=inventory["functions"][:1]), [("caller", "CALLER")])
        self.assertIn("CS window", report["claims"][0]["reason"])

    def test_caller_cannot_borrow_another_claims_frame(self):
        # The caller is reached only through an outside near call; the callee's own
        # far entry names a different CS (0fff), so the caller's CS is unproven.
        base = 0xfff0
        callee_call, callee_site = far_call(16, 0x0fff, 0x0010)
        image = bytearray(base) + bytes.fromhex("0ee80c00cb") + b"\x90" * 11 + b"\xcb"
        image[16:21] = callee_call
        oracle, inventory = synthetic_oracle(image, [("caller", 0, base, 16, []), ("callee", 0x0fff, 0x0010, 1,
                                                                                     [callee_site])],
                                             relocations=[{"image_offset": 19}])
        obj = synthetic_object(bytes.fromhex("0ee80000cb") + b"\x90" * 11 + b"\xcb",
                               [("CALLER", 0), ("CALLEE", 16)], ["CALLEE"], [(2, 1, True)])
        report = self.check(obj, oracle, inventory, [("caller", "CALLER"), ("callee", "CALLEE")])
        self.assertIn("independently witnessed original frame", report["claims"][0]["reason"])

    def test_caller_below_the_callees_frame_is_refused(self):
        # Only the callee is far-entered (frame 0002, base 32); the pair at 30..33 straddles the base.
        callee_call, callee_site = far_call(8, 2, 4)
        image = bytearray(30) + bytes.fromhex("0ee80200cb90cb")
        image[8:13] = callee_call
        oracle, inventory = synthetic_oracle(image, [("caller", 0, 30, 6, []), ("callee", 2, 4, 1, [callee_site])],
                                             relocations=[{"image_offset": 11}])
        obj = synthetic_object(bytes.fromhex("0ee80000cb90cb"), [("CALLER", 0), ("CALLEE", 6)],
                               ["CALLEE"], [(2, 1, True)])
        report = self.check(obj, oracle, inventory, [("caller", "CALLER"), ("callee", "CALLEE")])
        self.assertEqual(report["claims"][0]["state"], "REFUSED")

    def test_tail_jump_into_a_draft_is_not_segment_safe(self):
        from binding import declarations, fixes
        from omf import OmfReader
        from oracle import Oracle
        import match
        real = Oracle.load()
        caller, root, _ = match.get_target(CALLER, real)
        base = 0x0e78
        # A calls B then reads STATE_BYTE; B ends with JMP into unclaimed draft C.
        raw, linked = bytes.fromhex("0ee80000a00000cbeb008ed8cb"), bytes.fromhex("0ee80400a0135ccbeb009090cb")
        image = root.data[:base] + linked + root.data[base + len(linked):]
        oracle, inventory = synthetic_oracle(image, [("A", 0, base, 8, caller["call_sites"]), ("B", 0, base + 8, 2, [])],
                                             relocations=root.relocations, source=real)
        obj = synthetic_object(raw, [("A", 0), ("B", 8), ("C", 10)], ["B", "STATE_BYTE"],
                               [(2, 1, True), (5, 2, False)], dgroup=True)
        module = OmfReader().read(obj)
        binding = common.read_json(BINDING)
        binding.update(target_id="group:fixture", declarations=declarations(module), fixups=fixes(module))
        binding["symbols"] = {"STATE_BYTE": binding["symbols"]["STATE_BYTE"]}
        report = self.check(obj, oracle, inventory, [("A", "A"), ("B", "B")], binding=binding,
                            group_id="group:fixture")
        self.assertIn("segment context", report["claims"][0]["reason"])

    def test_data_fixup_naming_a_code_public_is_refused(self):
        from binding import declarations, fixes
        from oracle import Oracle
        from omf import OmfReader
        oracle = Oracle.load()
        obj = synthetic_object(bytes.fromhex("a000002a060000a20000cb90cb"),
                               [("UPDATE_STEP_DELTA", 0), ("STATE_BYTE", 12)],
                               ["STATE_BYTE", "STATE_BASE", "STATE_DELTA"],
                               [(1, 1, False), (5, 2, False), (8, 3, False)], dgroup=True)
        module = OmfReader().read(obj)
        binding = common.read_json(BINDING)
        binding.update(target_id="group:fixture", declarations=declarations(module), fixups=fixes(module))
        from module_gate import check_group
        report = check_group(obj, [{"target_id": CALLEE, "public": "UPDATE_STEP_DELTA"}], oracle,
                             binding=binding, group_id="group:fixture")
        self.assertEqual(report["claims"][0]["state"], "REFUSED")
        self.assertIn("CODE public", report["claims"][0]["reason"])


class RetiredPublicationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.manifest = self.root / "layout/manifest.json"
        common.write_json(self.manifest, {"owners": ["old"]})
        self.old = [self.root / "src/old.c", self.root / "recipes/old.json", self.root / "evidence/bindings/old.json"]
        for path in self.old:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b"old\n")
        self.new = {self.root / "src/group.c": b"new\n", self.root / "recipes/group.json": b"{}\n"}

    def publish(self, check=lambda: None):
        before = self.manifest.read_bytes()
        publication.publish(self.root, self.new, before, {"owners": ["group"]}, check, retire=self.old)

    def test_retired_files_are_removed_only_after_the_manifest_commit(self):
        self.publish()
        self.assertEqual(common.read_json(self.manifest), {"owners": ["group"]})
        self.assertFalse(any(path.exists() for path in self.old))
        self.assertTrue(all(path.exists() for path in self.new))

    def test_failed_gate_keeps_retired_files_and_old_manifest(self):
        def fail():
            raise ValueError("gate refused")
        with self.assertRaises(ValueError):
            self.publish(fail)
        self.assertEqual(common.read_json(self.manifest), {"owners": ["old"]})
        self.assertTrue(all(path.exists() for path in self.old))
        self.assertFalse(any(path.exists() for path in self.new))

    def test_retired_file_edited_during_publication_is_kept_and_rolled_back(self):
        def edit():
            self.old[0].write_bytes(b"edited after the journal pin\n")
        with self.assertRaisesRegex(ValueError, "changed during publication"):
            self.publish(edit)
        self.assertEqual(common.read_json(self.manifest), {"owners": ["old"]})
        self.assertEqual(self.old[0].read_bytes(), b"edited after the journal pin\n")
        self.assertFalse(publication.journal_path(self.root).exists())

    def test_recovery_after_commit_finishes_retirement(self):
        before = self.manifest.read_bytes()
        after = common.json_bytes({"owners": ["group"]})
        record = {"schema": 1, "before_sha256": common.sha(before), "after_sha256": common.sha(after),
                  "files": [{"path": p.relative_to(self.root).as_posix(), "sha256": common.sha(d)}
                            for p, d in self.new.items()],
                  "retire": [{"path": p.relative_to(self.root).as_posix(), "sha256": common.sha(b"old\n")}
                             for p in self.old]}
        for path, data in self.new.items():
            path.write_bytes(data)
        self.manifest.write_bytes(after)
        self.old[0].unlink()  # crash after deleting one retired file
        common.write_json(publication.journal_path(self.root), record)
        self.assertEqual(publication.recover(self.root), "Completed committed publication")
        self.assertFalse(any(path.exists() for path in self.old))

    def test_retiring_outside_canonical_directories_is_refused(self):
        stray = self.root / "docs/note.md"
        stray.parent.mkdir(parents=True)
        stray.write_bytes(b"x")
        self.old.append(stray)
        with self.assertRaisesRegex(ValueError, "must be canonical"):
            self.publish()
        self.assertTrue(stray.exists())


if __name__ == "__main__":
    unittest.main()


@unittest.skipUnless(Path(load_lock()["runner"]["path"]).is_file(), "Pinned historical runner unavailable")
class GroupFarCallTests(unittest.TestCase):
    """The group gate reproduces the single-function far gate on accepted SetFont."""

    def run_group(self, mutate=None):
        from module_gate import compile_group
        recipe = common.read_json(ROOT / "recipes/resc/set_font.json")
        binding = dict(common.read_json(ROOT / recipe["binding"]), target_id="group:far_probe")
        if mutate:
            mutate(binding)
        source = (ROOT / recipe["source"]).read_text()
        return compile_group(source, recipe["profile"], recipe["flags"],
                             [{"target_id": recipe["target_id"], "public": recipe["public"]}],
                             ROOT / "build/workers/tests/group_far", binding=binding, group_id="group:far_probe")[1]

    def test_far_calls_and_mz_relocations_are_proven_per_claim(self):
        claim = self.run_group()["claims"][0]
        self.assertEqual(claim["state"], "CODE_EXACT")
        self.assertEqual(claim["covered_relocation_indices"], [1453, 1654])
        self.assertEqual(claim["relocation_order"], "GROUPED")
        self.assertEqual(sorted(e["kind"] for e in claim["equations"]), ["far-call", "far-call"])

    def test_far_witness_inside_the_object_is_refused(self):
        recipe = common.read_json(ROOT / "recipes/resc/set_font.json")
        from match import get_target
        from oracle import Oracle
        target, space, raw = get_target(recipe["target_id"], Oracle.load())
        at = space.position(target["segment"], target["offset"]) + 6  # the 9A inside the claim

        def mutate(binding):
            name = next(iter(binding["symbols"]))
            binding["symbols"][name]["witnesses"].append(
                {"space": "root", "image_offset": at, "bytes_hex": space.data[at:at+5].hex(),
                 "relocation_index": 0, "file_offset": space.file_offset + at})
        with self.assertRaises(ValueError):
            self.run_group(mutate)


class ReplacePublicationTests(unittest.TestCase):
    """A group grown under its own name replaces its files; rollback restores them."""

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.manifest = self.root / "layout/manifest.json"
        common.write_json(self.manifest, {"owners": ["small"]})
        self.recipe = self.root / "recipes/ctrl/ctrl.json"
        self.recipe.parent.mkdir(parents=True)
        self.recipe.write_bytes(b"old recipe\n")

    def publish(self, check=lambda: None):
        before = self.manifest.read_bytes()
        publication.publish(self.root, {self.recipe: b"new recipe\n"}, before, {"owners": ["grown"]},
                            check, replace=[self.recipe])

    def test_commit_replaces_and_clears_backups(self):
        self.publish()
        self.assertEqual(self.recipe.read_bytes(), b"new recipe\n")
        self.assertFalse(any((self.root / "build/publication-backup").glob("*")))

    def test_failed_gate_restores_the_old_file(self):
        def fail():
            raise ValueError("gate refused")
        with self.assertRaises(ValueError):
            self.publish(fail)
        self.assertEqual(self.recipe.read_bytes(), b"old recipe\n")
        self.assertEqual(common.read_json(self.manifest), {"owners": ["small"]})

    def test_unlisted_existing_file_is_still_refused(self):
        before = self.manifest.read_bytes()
        with self.assertRaisesRegex(ValueError, "overwrite"):
            publication.publish(self.root, {self.recipe: b"x\n"}, before, {"owners": []}, lambda: None)


class ReachabilityTests(unittest.TestCase):
    """v4 witnesses must be reached code: LOOP edges followed, prefixed JMP not fallen through."""

    def reach(self, code, entry_at=32):
        import binding
        from oracle import Space
        image = bytearray(16) + struct.pack("<BHH", 0x9a, entry_at, 0) + bytearray(entry_at - 21) + code
        space = Space("root", 0, 0, bytes(image), ({"image_offset": 19, "index": 0},))
        oracle = SimpleNamespace(spaces={"root": space})
        binding._REACH.clear()
        return binding.reachable_from_entry(oracle, "root:0000:%04x" % entry_at)

    def test_loop_target_is_followed(self):
        # 32: loop +2 -> 36 ; 34: ret ; 35: nop ; 36: ret
        self.assertIn(36, self.reach(bytes.fromhex("e202c390c3")))

    def test_near_call_target_is_followed(self):
        # 32: call +1 -> 36 ; 35: retf ; 36: ret
        self.assertIn(36, self.reach(bytes.fromhex("e80100cbc3")))

    def test_prefixed_unconditional_jump_does_not_fall_through(self):
        # 32: cs: jmp word ptr [0x0100] ; 37: bytes after it must not be reached
        self.assertNotIn(37, self.reach(bytes.fromhex("2eff260001b8ffffc3")))


class V4FieldTests(unittest.TestCase):
    """v4 grounds only address fields and only zero addends (2026-11-21 review)."""

    def rows(self, hexcode):
        import binding
        return binding.instructions(bytes.fromhex(hexcode))

    def test_branch_ret_and_arithmetic_immediates_are_not_address_fields(self):
        from binding import operand_field
        for code, at in (("e83412", 1), ("e93412", 1), ("ca3412", 1), ("053412", 1)):
            self.assertIsNone(operand_field(self.rows(code)[0], at), code)
        self.assertEqual(operand_field(self.rows("b83412")[0], 1), "immediate")
        self.assertEqual(operand_field(self.rows("8b873412")[0], 2), "displacement")
        # A candidate's FIXUPP marks ADD ax, offset X as an address; branches never qualify.
        self.assertEqual(operand_field(self.rows("053412")[0], 1, candidate=True), "immediate")
        self.assertIsNone(operand_field(self.rows("e83412")[0], 1, candidate=True))

    def test_nonzero_addend_is_refused(self):
        from binding import OPERAND_MODE, data_equations
        code = bytes.fromhex("b80900cb")  # mov ax, offset sym+9 ; retf
        fix = {"segment": "T", "segment_offset": 1, "loc_type": 1, "field_width": 2, "self_relative": False,
               "target": {"kind": "external", "method": 6, "name": "SYM"}, "displacement": None}
        fix["frame"] = {"kind": "target_frame", "target": fix["target"]}
        with self.assertRaisesRegex(ValueError, "witnessed members"):
            data_equations([fix], {"SYM": {"offset": 0x6252}}, "T", code, bytes.fromhex("b85b62cb"),
                           self.rows("b80900cb"), OPERAND_MODE, set())


@unittest.skipUnless(Path(load_lock()["runner"]["path"]).is_file(), "Pinned historical runner unavailable")
class SwitchTableTests(unittest.TestCase):
    """A real MSC jump table: the CS: displacement and table words are own-segment fields."""

    SOURCE = """
int far pascal Sw(int k)
{
    switch (k) {
    case 1: return 5;  case 2: return 9;  case 3: return 12; case 4: return 20;
    case 5: return 1;  case 6: return 3;  case 7: return 8;  case 8: return 2;
    }
    return 0;
}
"""

    def test_dense_switch_is_exact_and_the_jump_is_not_cut(self):
        from module_gate import check_group
        obj = compile_c(self.SOURCE, "msc600", FLAGS, workdir=ROOT / "build/workers/tests/switch").obj_bytes
        from omf import OmfReader
        module = OmfReader().read(obj)
        code = next(s for s in module.segment_defs if s["class"] == "CODE")
        raw = module.segments[code["name"]]
        base = 0x100  # file starts on a paragraph: own-segment values equal their addends
        call, site = far_call(0x10, base // 16, 0)
        image = bytearray(base) + raw
        image[0x10:0x15] = call
        oracle, inventory = synthetic_oracle(image, [("sw", base // 16, 0, len(raw), [site])],
                                             relocations=[{"image_offset": 0x13}])
        import match
        with patch.object(match, "read_json", return_value=inventory):
            report = check_group(obj, [{"target_id": "sw", "public": "SW"}], oracle)
        claim = report["claims"][0]
        self.assertEqual(claim["state"], "CODE_EXACT", claim)
        self.assertGreaterEqual(sum(e["kind"] == "own-segment offset16" for e in claim["equations"]), 9)


class PlacementTests(unittest.TestCase):
    """A file's own DGROUP segment is placed only when over-determined."""

    def check(self, segments, uses, placements, image=b""):
        from module_gate import _check_placements
        from oracle import Space
        module = SimpleNamespace(
            segment_defs=[{"name": n, "length": len(c) if c is not None else size} for n, c, size in segments],
            segments={n: c for n, c, _ in segments if c is not None},
            initialized_ranges={n: [(0, len(c))] for n, c, _ in segments if c is not None})
        space = Space("root", 0, 0, bytes(0x100) + image + bytes((0x10, 0x00)), ({"index": 0, "image_offset": 0x100 + len(image)},))
        oracle = SimpleNamespace(spaces={"root": space})
        binding = {"frame": {"segment": 0x10},
                   "dgroup_segment": {"segment": 0x10, "witnesses": [
                       {"space": "root", "image_offset": 0x100 + len(image), "relocation_index": 0}]}}
        return _check_placements(module, placements, uses, binding, space, oracle)

    def test_uninitialized_segment_needs_two_distinct_addends(self):
        with self.assertRaisesRegex(ValueError, "two distinct"):
            self.check([("_BSS", None, 4)], {"_BSS": {0}}, {"_BSS": 8})
        self.assertEqual(self.check([("_BSS", None, 4)], {"_BSS": {0, 2}}, {"_BSS": 8})["_BSS"]["placement"], 8)

    def test_initialized_segment_must_equal_the_original_bytes(self):
        image = bytes(8) + b"\x12\x34"
        self.assertTrue(self.check([("_DATA", b"\x12\x34", 2)], {}, {"_DATA": 8}, image))
        with self.assertRaisesRegex(ValueError, "differs"):
            self.check([("_DATA", b"\x12\x35", 2)], {}, {"_DATA": 8}, image)


@unittest.skipUnless(Path(load_lock()["runner"]["path"]).is_file(), "Pinned historical runner unavailable")
class V4InternalSupportTests(unittest.TestCase):
    """A v4 address without witnesses is proven only by two equal claimed sites (internal
    consensus); draft-region original instructions may witness; aliasing is refused."""

    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        from module_gate import check_group
        cls.oracle = Oracle.load()
        recipe = common.read_json(ROOT / "recipes/ctrl/ctrl.json")
        result = compile_c((ROOT / recipe["source"]).read_text(encoding="utf-8"), recipe["profile"],
                           recipe["flags"], workdir=ROOT / "build/workers/tests/v4_internal")
        assert result.ok, result.log
        cls.obj = result.obj_bytes
        cls.claims = [{"target_id": c["target_id"], "public": c["public"]} for c in recipe["claims"]]
        cls.binding = common.read_json(ROOT / recipe["binding"])
        cls.group = recipe["group_id"]
        baseline = check_group(cls.obj, cls.claims, cls.oracle, binding=cls.binding, group_id=cls.group)
        assert baseline["exact"]
        sites = {}
        for claim in baseline["claims"]:
            for e in claim["equations"]:
                if e["kind"] == "DGROUP offset16" and e["encoded_addend"] == 0:
                    sites.setdefault(e["symbol"], []).append(claim["object_offset"] + e["offset"])
        plain = {n for n, s in cls.binding["symbols"].items() if s.get("kind") == "dgroup-offset" and not s.get("members")}
        cls.multi = next(n for n, at in sites.items() if len(at) >= 2 and n in plain)
        cls.single = next(n for n, at in sites.items() if len(at) == 1 and n in plain)
        cls.single_site = sites[cls.single][0]

    def check(self, binding):
        from module_gate import check_group
        return check_group(self.obj, self.claims, self.oracle, binding=binding, group_id=self.group)

    def stripped(self, name):
        binding = copy.deepcopy(self.binding)
        binding["symbols"][name]["witnesses"] = []
        return binding

    def test_two_equal_claimed_sites_prove_an_unwitnessed_address(self):
        report = self.check(self.stripped(self.multi))
        self.assertTrue(report["exact"])
        support = {e.get("support") for c in report["claims"] for e in c["equations"]
                   if e.get("symbol") == self.multi and e["kind"] == "DGROUP offset16"}
        self.assertEqual(support, {"internal consensus"})

    def test_a_single_claimed_site_is_refused(self):
        report = self.check(self.stripped(self.single))
        refused = [c for c in report["claims"] if c["state"] == "REFUSED"]
        self.assertTrue(refused)
        self.assertIn("second equal claimed site", refused[0]["reason"])

    def test_a_witness_inside_a_claim_is_refused(self):
        binding = copy.deepcopy(self.binding)
        at = self.single_site + self.check(self.binding)["origin"]
        root = self.oracle.spaces["root"]
        binding["symbols"][self.single]["witnesses"] = [
            {"bytes_hex": root.data[at-2:at+2].hex(), "image_offset": at - 2, "operand_offset": 2,
             "space": "root", "context": "inside a claim"}]
        with self.assertRaisesRegex(ValueError, "overlaps candidate"):
            self.check(binding)

    def test_member_aliasing_another_symbol_is_refused(self):
        binding = copy.deepcopy(self.binding)
        other = binding["symbols"][self.single]["offset"]
        mine = binding["symbols"][self.multi]
        mine["members"] = [{"offset": (other - mine["offset"]) & 0xFFFF, "witnesses": []}]
        with self.assertRaisesRegex(ValueError, "share one DGROUP offset"):
            self.check(binding)
