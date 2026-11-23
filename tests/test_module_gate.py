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

def recipe_claims(recipe):
    """Claims plus recorded code matches, as validation compiles a group."""
    return [{"target_id": c["target_id"], "public": c["public"]}
            for c in recipe["claims"] + recipe.get("code_matches", [])]


def exact_except_matches(report, recipe):
    matches = {c["target_id"] for c in recipe.get("code_matches", [])}
    return all(c["exact"] for c in report["claims"] if c["target_id"] not in matches)

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

    def test_near_call_without_a_witnessed_frame_is_only_frame_invariant(self):
        # No reviewed far entry names the caller's CS, but the only frame the image stores
        # (0000) reaches the callee: accepted as the recorded weaker class, not as witnessed.
        obj, oracle, inventory = self.near_fixture([(2, 1, True)])
        for row in inventory["functions"]:
            row["call_sites"] = []
        report = self.check(obj, oracle, inventory, [("caller", "CALLER"), ("callee", "CALLEE")])
        self.assertEqual(report["claims"][0]["state"], "CODE_EXACT", report["claims"][0])
        self.assertEqual(report["claims"][0]["cs_frame"], "frame-invariant near calls")

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
                                             # The site paragraph 0000 is a candidate CS whose window
                                             # holds the caller but wraps the call to 0000:0000.
                                             relocations=[{"image_offset": 19, "segment": 0}])
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

    def test_indirect_call_and_jump_through_memory_carry_a_data_address(self):
        from binding import operand_field
        # call far [1fe8], call [1fe8], jmp [1fe8]: the disp16 names the pointer's storage.
        for code in ("ff1ee81f", "ff16e81f", "ff26e81f"):
            self.assertEqual(operand_field(self.rows(code)[0], 2), "displacement", code)
        # Register-indirect calls and direct far calls have no data address field.
        self.assertIsNone(operand_field(self.rows("ffd3")[0], 1))
        self.assertIsNone(operand_field(self.rows("9a34120000")[0], 1))

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


class ExclusiveSelectorTests(unittest.TestCase):
    """A CONST piece of far-data selector words is placed by who loads them."""

    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        cls.oracle = Oracle.load()
        cls.home = cls.oracle.spaces["overlay-17"]

    def place(self, p, length, name, space):
        from module_gate import _exclusive_selectors
        r = common.read_json(ROOT / f"recipes/{name}.json")
        content = self.home.data[0x2940 + p:0x2940 + p + length]
        return _exclusive_selectors(self.oracle, self.home, 0x2940, p, content, self.oracle.spaces[space],
                                    (r["origin"], r["origin"] + r["emitted_size"]))

    def test_const_start_is_the_end_of_the_termination_table(self):
        import module_gate
        root = self.oracle.spaces["root"]
        self.assertEqual(root.data[0x200C0:0x200C6], bytes.fromhex("be3228bf3628"))
        self.assertEqual(module_gate.CONST_START, 0x2836)

    def test_original_files_own_their_selector_words(self):
        self.assertTrue(self.place(0x2842, 2, "auto/auto", "overlay-3"))
        self.assertTrue(self.place(0x2844, 2, "drawruin/drawruin", "overlay-8"))
        self.assertTrue(self.place(0x2846, 6, "final/final", "overlay-10"))

    def test_another_files_word_or_a_wrong_object_is_refused(self):
        self.assertFalse(self.place(0x2848, 2, "auto/auto", "overlay-3"))      # FINAL loads it
        self.assertFalse(self.place(0x2842, 2, "final/final", "overlay-10"))   # AUTO loads it
        self.assertFalse(self.place(0x2822, 2, "auto/auto", "overlay-3"))      # before CONST, never loaded


class FarConsensusTests(unittest.TestCase):
    """A far data address without an outside witness needs two equal exact claimed sites."""

    def run_rule(self, fields, conditional=()):
        symbols = {"T": {"kind": "far-data", "offset": 8, "members": [{"offset": 1, "consensus": True}]}}
        states = {p: {"far_fields": [{"key": ("T", k), "insn": at, "equal": eq} for k, at, eq in rows]}
                  for p, rows in fields.items()}
        reports = [{"public": p, "exact": True} for p in fields]
        from module_gate import _far_consensus
        _far_consensus(symbols, states, reports, set(conditional))
        return {r["public"]: r["exact"] for r in reports}

    def test_two_sites_support_each_other_and_one_does_not(self):
        self.assertEqual(self.run_rule({"A": [(1, 10, True)], "B": [(1, 20, True)]}), {"A": True, "B": True})
        self.assertEqual(self.run_rule({"A": [(1, 10, True)]}), {"A": False})
        self.assertEqual(self.run_rule({"A": [(1, 10, True), (1, 10, True)]}), {"A": False})   # one instruction
        self.assertEqual(self.run_rule({"A": [(0, 10, True)]}), {"A": True})                   # witnessed base
        self.assertEqual(self.run_rule({"A": [(1, 10, True)], "B": [(1, 20, True)]}, ["B"])["A"], False)

    def test_far_callee_called_only_by_claims(self):
        from module_gate import _far_consensus

        def run(rows):
            states = {p: {"far_fields": [{"key": ("F", 0), "insn": at, "equal": eq} for at, eq in v]}
                      for p, v in rows.items()}
            reports = [{"public": p, "exact": True} for p in rows]
            _far_consensus({"F": {"kind": "far-code-entry", "consensus": True}}, states, reports, set())
            return {r["public"]: r["exact"] for r in reports}
        self.assertEqual(run({"A": [(10, True)], "B": [(20, True)]}), {"A": True, "B": True})
        self.assertEqual(run({"A": [(10, True)]}), {"A": False})
        self.assertEqual(run({"A": [(10, True), (20, False)]}), {"A": False})

    def test_far_callee_consensus_is_a_group_rule_for_a_reviewed_entry(self):
        from far_binding import ground_symbol
        from oracle import Oracle
        oracle = Oracle.load()
        rows = common.read_json(ROOT / "evidence/targets.json")["functions"]
        callee = next(t for t in rows if t["id"] == "root:1fe6:1beb")
        symbol = {"kind": "far-code-entry", "space": "root", "segment": 0x1fe6, "offset": 0x1beb,
                  "entry_target_id": callee["id"], "entry_sha256": callee["sha256"], "witnesses": [], "consensus": True}
        target = next(t for t in rows if t["id"] == "root:052d:0518")
        self.assertEqual(ground_symbol(symbol, target, oracle, claimed=[])["grounded_by"], "internal consensus")
        with self.assertRaisesRegex(ValueError, "group rule"):
            ground_symbol(symbol, target, oracle)
        with self.assertRaisesRegex(ValueError, "without CALL witnesses"):
            ground_symbol({**symbol, "witnesses": [{}]}, target, oracle, claimed=[])


class UnwitnessedExtentTests(unittest.TestCase):
    """In-extent members of an aggregate without a witness stand only on its base's own consensus."""

    def run_rule(self, fields, witnesses=()):
        from module_gate import _internal_support
        symbols = {"A": {"kind": "dgroup-offset", "offset": 8, "witnesses": list(witnesses), "extent": {"length": 13},
                         "members": [{"offset": 5, "witnesses": []}]}}
        states = {p: {"v4": [{"key": ("A", k), "insn": at, "equal": True, "offset": at} for k, at in rows],
                      "equations": []} for p, rows in fields.items()}
        reports = [{"public": p, "exact": True} for p in fields]
        _internal_support(symbols, states, reports, set())
        return {r["public"]: r["exact"] for r in reports}

    def test_member_needs_two_base_sites_unless_the_base_is_witnessed(self):
        self.assertEqual(self.run_rule({"F": [(0, 1), (0, 2)], "G": [(5, 3)]}), {"F": True, "G": True})
        self.assertEqual(self.run_rule({"F": [(0, 1)], "G": [(5, 3)]}), {"F": False, "G": False})
        self.assertEqual(self.run_rule({"G": [(5, 3)]}), {"G": False})
        self.assertEqual(self.run_rule({"G": [(5, 3)]}, witnesses=[{}]), {"G": True})


class PlacementTests(unittest.TestCase):
    """A file's own DGROUP segment is placed only when over-determined."""

    def check(self, segments, uses, placements, image=b"", data_fixes=(), far_symbols=None, relocs=(),
              data_symbols=None, code=None):
        from module_gate import _check_placements
        from oracle import Space
        module = SimpleNamespace(
            segment_defs=[{"name": n, "length": len(c) if c is not None else size} for n, c, size in segments],
            segments={n: c for n, c, _ in segments if c is not None},
            initialized_ranges={n: [(0, len(c))] for n, c, _ in segments if c is not None})
        space = Space("root", 0, 0, bytes(0x100) + image + bytes((0x10, 0x00)),
                      ({"index": 0, "image_offset": 0x100 + len(image)},) +
                      tuple({"index": i + 1, "image_offset": 0x100 + r} for i, r in enumerate(relocs)))
        oracle = SimpleNamespace(spaces={"root": space})
        binding = {"frame": {"segment": 0x10},
                   "dgroup_segment": {"segment": 0x10, "witnesses": [
                       {"space": "root", "image_offset": 0x100 + len(image), "relocation_index": 0}]}}
        return _check_placements(module, placements, uses, binding, space, oracle, data_fixes, far_symbols,
                                 data_symbols, None, code)

    @staticmethod
    def pointer(at, frame=None):
        return {"segment": "_DATA", "segment_offset": at, "loc_type": 1, "field_width": 2, "self_relative": False,
                "frame": frame or {"kind": "group", "index": 1, "name": "DGROUP"},
                "target": {"kind": "segment", "index": 2, "name": "_DATA", "method": 4}, "displacement": None}

    def test_initialized_near_pointer_takes_its_linked_value(self):
        # _DATA = {&_DATA[2], 0xBBAA} placed at 8: the linker writes 8 + 2 = 0x000a.
        content = b"\x02\x00\xaa\xbb"
        image = bytes(8) + b"\x0a\x00\xaa\xbb"
        report = self.check([("_DATA", content, 4)], {}, {"_DATA": 8}, image, [self.pointer(0)])
        self.assertTrue(report["_DATA"]["unique"])
        self.assertEqual(report["_DATA"]["pointer_fields"], 1)
        with self.assertRaisesRegex(ValueError, "differs"):  # the unlinked object bytes are not the original
            self.check([("_DATA", content, 4)], {}, {"_DATA": 8}, bytes(8) + content, [self.pointer(0)])

    def test_data_fixup_outside_the_dgroup_near_pointer_form_is_refused(self):
        image = bytes(8) + b"\x0a\x00\xaa\xbb"
        frame = {"kind": "segment", "index": 2, "name": "_DATA"}
        with self.assertRaisesRegex(ValueError, "not a DGROUP near pointer"):
            self.check([("_DATA", b"\x02\x00\xaa\xbb", 4)], {}, {"_DATA": 8}, image, [self.pointer(0, frame)])

    def test_near_pointer_to_another_files_grounded_data(self):
        # _DATA = {&TABLE[2], 0xBBAA}; TABLE grounded at DS:0x40 elsewhere: the linker writes 0x0042.
        target = {"kind": "external", "index": 3, "name": "TABLE", "method": 6}
        field = dict(self.pointer(0), target=target)
        image = bytes(8) + b"\x42\x00\xaa\xbb"
        args = ([("_DATA", b"\x02\x00\xaa\xbb", 4)], {}, {"_DATA": 8}, image, [field])
        self.assertTrue(self.check(*args, data_symbols={"TABLE": {"offset": 0x40}})["_DATA"]["unique"])
        f5 = dict(field, frame={"kind": "target_frame", "target": target})   # MSC extern: F5/T6
        self.assertTrue(self.check(*args[:4], [f5], data_symbols={"TABLE": {"offset": 0x40}})["_DATA"]["unique"])
        for symbols in (None, {"TABLE": {"offset": 0x40, "unproven": "single site"}}):  # ungrounded: refused
            with self.assertRaisesRegex(ValueError, "independently grounded"):
                self.check(*args, data_symbols=symbols)

    @staticmethod
    def far_pointer(at):
        target = {"kind": "external", "index": 3, "name": "CALLBACK", "method": 6}
        return {"segment": "_DATA", "segment_offset": at, "loc_type": 3, "field_width": 4, "self_relative": False,
                "frame": {"kind": "target_frame", "target": target}, "target": target, "displacement": None}

    def test_far_pointer_into_own_code_uses_the_witnessed_frame(self):
        # _DATA = {own function at CODE+0x10, 0xBBAA}; CODE lies at 2a93:0040.
        target = {"kind": "segment", "index": 1, "name": "UNIT_TEXT", "method": 4}
        field = {"segment": "_DATA", "segment_offset": 0, "loc_type": 3, "field_width": 4, "self_relative": False,
                 "frame": {"kind": "target_frame", "target": target}, "target": target, "displacement": None}
        content, image = bytes.fromhex("10000000aabb"), bytes(8) + bytes.fromhex("5000932aaabb")
        args = ([("_DATA", content, 6)], {}, {"_DATA": 8}, image, [field])
        code = ("UNIT_TEXT", 0x40, 0x100, 0x2A93, {"OWN": 0x10})
        self.assertTrue(self.check(*args, relocs=[10], code=code)["_DATA"]["unique"])
        with self.assertRaisesRegex(ValueError, "witnessed frame"):
            self.check(*args, relocs=[10])
        with self.assertRaisesRegex(ValueError, "relocated words"):
            self.check(*args, code=code)
        with self.assertRaisesRegex(ValueError, "differs"):
            self.check(*args, relocs=[10], code=("UNIT_TEXT", 0x40, 0x100, 0x2A94, {}))
        with self.assertRaisesRegex(ValueError, "leaves the object's CODE"):
            self.check(*args, relocs=[10], code=("UNIT_TEXT", 0x40, 0x10, 0x2A93, {}))
        # The same pointer through an EXTDEF of an own public (MSC), zero stored offset.
        ext = {"kind": "external", "index": 4, "name": "OWN", "method": 6}
        field = dict(field, target=ext, frame={"kind": "target_frame", "target": ext})
        args = ([("_DATA", bytes.fromhex("00000000aabb"), 6)], {}, {"_DATA": 8}, image, [field])
        self.assertTrue(self.check(*args, relocs=[10], code=code)["_DATA"]["unique"])
        with self.assertRaisesRegex(ValueError, "grounded far code symbol"):   # not a public of this object
            self.check(*args, relocs=[10], code=code[:4] + ({},))

    def test_far_code_pointer_takes_the_grounded_symbol_and_needs_its_relocation(self):
        # _DATA = {CALLBACK (far), 0xBBAA} at 8; CALLBACK grounded at 1234:0056.
        far = {"CALLBACK": {"kind": "far-code-entry", "segment": 0x1234, "offset": 0x56}}
        content = bytes(4) + b"\xaa\xbb"
        image = bytes(8) + b"\x56\x00\x34\x12\xaa\xbb"
        report = self.check([("_DATA", content, 6)], {}, {"_DATA": 8}, image, [self.far_pointer(0)], far, relocs=(10,))
        self.assertEqual(report["_DATA"]["pointer_fields"], 1)
        with self.assertRaisesRegex(ValueError, "relocated words"):  # the original must relocate the segment word
            self.check([("_DATA", content, 6)], {}, {"_DATA": 8}, image, [self.far_pointer(0)], far)
        with self.assertRaisesRegex(ValueError, "grounded far code symbol"):  # an ungrounded target
            self.check([("_DATA", content, 6)], {}, {"_DATA": 8}, image, [self.far_pointer(0)], {}, relocs=(10,))

    @staticmethod
    def segment_word(at):
        target = {"kind": "external", "index": 4, "name": "FAR_TABLE", "method": 6}
        return {"segment": "CONST", "segment_offset": at, "loc_type": 2, "field_width": 2, "self_relative": False,
                "frame": {"kind": "target_frame", "target": target}, "target": target, "displacement": None}

    def test_far_segment_word_takes_the_grounded_segment_and_needs_its_relocation(self):
        # CONST = {SEG FAR_TABLE, 0xBBAA} at 8; FAR_TABLE grounded in segment 2c3e.
        far = {"FAR_TABLE": {"kind": "far-data", "segment": 0x2c3e, "offset": 0}}
        content = bytes(2) + b"\xaa\xbb"
        image = bytes(8) + b"\x3e\x2c\xaa\xbb"
        report = self.check([("CONST", content, 4)], {}, {"CONST": 8}, image, [self.segment_word(0)], far, relocs=(8,))
        self.assertEqual(report["CONST"]["pointer_fields"], 1)
        with self.assertRaisesRegex(ValueError, "relocated words"):
            self.check([("CONST", content, 4)], {}, {"CONST": 8}, image, [self.segment_word(0)], far)
        code = {"FAR_TABLE": {"kind": "far-code-entry", "segment": 0x2c3e, "offset": 0}}
        with self.assertRaisesRegex(ValueError, "grounded far segment symbol"):  # a code entry is not a far segment
            self.check([("CONST", content, 4)], {}, {"CONST": 8}, image, [self.segment_word(0)], code, relocs=(8,))

    def test_uninitialized_segment_is_never_self_proving(self):
        # Distinct addends do not support each other: the source chooses the layout.
        self.assertFalse(self.check([("_BSS", None, 4)], {"_BSS": {0, 2}}, {"_BSS": 8})["_BSS"]["unique"])

    def test_initialized_segment_must_equal_the_original_bytes(self):
        image = bytes(8) + b"\x12\x34"
        self.assertTrue(self.check([("_DATA", b"\x12\x34", 2)], {}, {"_DATA": 8}, image)["_DATA"]["unique"])
        with self.assertRaisesRegex(ValueError, "differs"):
            self.check([("_DATA", b"\x12\x35", 2)], {}, {"_DATA": 8}, image)

    def test_single_byte_alignment_holes_are_linker_zeros(self):
        from module_gate import _initialized_with_alignment_holes as holes
        self.assertTrue(holes([(0, 39), (40, 57), (58, 87)], 87))
        self.assertFalse(holes([(0, 39), (41, 87)], 87))   # a two-byte gap is not alignment
        self.assertFalse(holes([(1, 87)], 87))
        self.assertFalse(holes([(0, 86)], 87))

    def test_repeated_content_fixes_no_placement(self):
        # Zero words occur everywhere in DGROUP; matching one proves nothing about P.
        image = bytes(8) + b"\x12\x34"
        self.assertFalse(self.check([("_DATA", b"\x00\x00", 2)], {}, {"_DATA": 2}, image)["_DATA"]["unique"])


class SolvedFrameTests(unittest.TestCase):
    """Root code entered only through CS-relative words: the original words fix the frame."""

    def solve(self, table_words, frames=(0x100, 0x180), states=None, proven=()):
        from module_gate import _solved_frame
        space = SimpleNamespace(name="root", link_segment=0)
        states = states if states is not None else {"ENTRY": {"own": [], "position": 0x1400, "piece": b"\0" * 8,
                                                             "expected": b"\0" * 8}}
        with patch("module_gate._candidate_frames", return_value=set(frames)):
            # object origin 0x1300; ENTRY at object offset 0x100 (linear 0x1400)
            return _solved_frame(states, set(proven), table_words, space, SimpleNamespace(), 0x1300,
                                 {"ENTRY": 0x100})

    def test_agreeing_table_words_solve_one_frame_and_enter_their_target(self):
        # Frame 0x100 (base 0x1000): a word naming object offset A holds 0x1300 + A - 0x1000.
        words = [(0x100, 0x10, 0x0400), (0x104, 0x12, 0x0404)]
        base, solved, entered, count = self.solve(words)
        self.assertEqual((base, entered, count), (0x1000, {"ENTRY"}, 2))

    def test_one_word_or_disagreeing_words_prove_nothing(self):
        self.assertIsNone(self.solve([(0x100, 0x10, 0x0400)]))
        self.assertIsNone(self.solve([(0x100, 0x10, 0x0400), (0x104, 0x12, 0x0405)]))

    def test_frame_must_be_one_the_original_stores(self):
        self.assertIsNone(self.solve([(0x100, 0x10, 0x0400), (0x104, 0x12, 0x0404)], frames=(0x101,)))


class PlacedSupportTests(unittest.TestCase):
    """Fields into a placed segment without unique content need two equal claimed sites."""

    def run_support(self, sites, debt=()):
        import module_gate
        states = {f"P{i}": {"placed": [{"key": ("_BSS", k), "equal": True, "insn": 0x100 * i + 4, "offset": 5}],
                            "equations": []} for i, k in enumerate(sites)}
        reports = [{"public": f"P{i}", "target_id": f"t{i}", "exact": True, "state": "CODE_EXACT"}
                   for i in range(len(sites))]
        with patch.object(module_gate, "PLACEMENT_DEBT", set(debt)):
            module_gate._placed_support({"_BSS": {"unique": False}}, states, reports)
        return [r["state"] for r in reports], reports

    def test_single_site_refused(self):
        self.assertEqual(self.run_support([0])[0], ["REFUSED"])

    def test_distinct_addends_do_not_support_each_other(self):
        self.assertEqual(self.run_support([0, 2])[0], ["REFUSED", "REFUSED"])

    def test_same_key_at_two_sites_is_consensus(self):
        self.assertEqual(self.run_support([0, 0])[0], ["CODE_EXACT", "CODE_EXACT"])

    def test_listed_debt_keeps_claim_and_reports_it(self):
        states, reports = self.run_support([0], debt={"t0"})
        self.assertEqual(states, ["CODE_EXACT"])
        self.assertEqual(reports[0]["placement_debt"], ["_BSS+0"])


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
        cls.claims = recipe_claims(recipe)
        cls.recipe = recipe
        cls.binding = common.read_json(ROOT / recipe["binding"])
        cls.group = recipe["group_id"]
        baseline = check_group(cls.obj, cls.claims, cls.oracle, binding=cls.binding, group_id=cls.group)
        assert exact_except_matches(baseline, recipe)
        sites = {}
        for claim in baseline["claims"]:
            for e in claim["equations"]:
                if e["kind"] == "DGROUP offset16" and e["encoded_addend"] == 0:
                    sites.setdefault(e["symbol"], []).append(claim["object_offset"] + e["offset"])
        plain = {n for n, s in cls.binding["symbols"].items()
                 if s.get("kind") == "dgroup-offset" and not s.get("members") and not s.get("unproven")}
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
        self.assertTrue(exact_except_matches(report, self.recipe))
        support = {e.get("support") for c in report["claims"] for e in c["equations"]
                   if e.get("symbol") == self.multi and e["kind"] == "DGROUP offset16"}
        self.assertEqual(support, {"internal consensus"})

    def test_a_single_claimed_site_is_refused(self):
        report = self.check(self.stripped(self.single))
        refused = [c for c in report["claims"] if c["state"] == "REFUSED"]
        self.assertTrue(refused)
        self.assertIn("second equal claimed site", refused[0]["reason"])

    def test_an_unproven_symbol_turns_only_its_users_into_code_matches(self):
        binding = self.stripped(self.single)
        binding["symbols"][self.single]["unproven"] = "single original site (test)"
        report = self.check(binding)
        self.assertFalse(report["exact"])
        recorded = {c["target_id"] for c in self.recipe.get("code_matches", [])}
        claims = [c for c in report["claims"] if c["target_id"] not in recorded]
        matches = [c for c in claims if c["state"] == "CODE_MATCH"]
        self.assertTrue(matches)
        self.assertTrue(all(c["unproven"] == [self.single] for c in matches))
        others = [c for c in claims if c not in matches]
        self.assertTrue(others and all(c["state"] == "CODE_EXACT" for c in others))

    def test_unproven_far_symbol_must_name_its_reviewed_entry(self):
        binding = copy.deepcopy(self.binding)
        far = next((n for n, s in binding["symbols"].items() if s.get("kind") == "far-code-entry"), None)
        if far is None:
            self.skipTest("CTRL binding has no far symbol")
        binding["symbols"][far].update(unproven="test", offset=binding["symbols"][far]["offset"] + 1)
        with self.assertRaisesRegex(ValueError, "does not name its reviewed entry"):
            self.check(binding)

    def test_a_provable_far_symbol_cannot_be_recorded_unproven(self):
        binding = copy.deepcopy(self.binding)
        far = next((n for n, s in binding["symbols"].items()
                    if s.get("kind") == "far-code-entry" and s.get("witnesses")), None)
        if far is None:
            self.skipTest("CTRL binding has no witnessed far symbol")
        binding["symbols"][far]["unproven"] = "test"
        with self.assertRaisesRegex(ValueError, "is provable"):
            self.check(binding)

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


def fixup(at, *, relative, frame, frame_datum=None, target, target_datum, displacement=None):
    """One explicit FIXUPP subrecord: LOC1 at `at`, frame method F0..F5, target method T0..T6."""
    low = target - 4 if target >= 4 else target
    fixdat = (frame << 4) | ((1 if target >= 4 else 0) << 2) | low
    body = struct.pack(">HB", (0x8000 if relative else 0xc000) | (1 << 10) | at, fixdat)
    body += bytes([frame_datum]) if frame <= 2 else b""
    body += bytes([target_datum])
    return body + (struct.pack("<H", displacement) if target < 4 else b"")


def masm_object(code, publics, externals=(), subrecords=(), *, group="DGROUP"):
    """MASM-shaped object: CODE segment 1, empty _DATA segment 2 in GRPDEF `group` (index 1)."""
    names = ("", "FIX_TEXT", "CODE", "_DATA", "DATA", group)
    records = [_record(0x80, _name("fixture")), _record(0x96, b"".join(map(_name, names))),
               _record(0x98, b"\x48" + struct.pack("<H", len(code)) + b"\x02\x03\x01"),
               _record(0x98, b"\x48\x00\x00\x04\x05\x01"), _record(0x9a, b"\x06\xff\x02")]
    if externals:
        records.append(_record(0x8c, b"".join(_name(e) + b"\x00" for e in externals)))
    records.append(_record(0x90, b"\x00\x01" + b"".join(_name(p) + struct.pack("<H", o) + b"\x00"
                                                       for p, o in publics)))
    records.append(_record(0xa0, b"\x01\x00\x00" + code))
    if subrecords:
        records.append(_record(0x9c, b"".join(subrecords)))
    records.append(_record(0x8a, b"\x00"))
    return b"".join(records)


def own_branch(at, destination):
    """MASM's own-segment rel16: F0 own CODE segment, T0 own CODE segment + displacement."""
    return fixup(at, relative=True, frame=0, frame_datum=1, target=0, target_datum=1, displacement=destination)


def near_external(at, index):
    """MASM's external near CALL: F0 own CODE segment, T6 EXTDEF."""
    return fixup(at, relative=True, frame=0, frame_datum=1, target=6, target_datum=index)


class SelfRelativeTests(unittest.TestCase):
    """MASM self-relative rel16 shapes: own-segment branches, plain and external near calls."""

    def setUp(self):
        import binding
        binding._REACH.clear()  # synthetic oracles may reuse an id

    def check(self, obj, oracle, inventory, claims, *, owners=(), **kw):
        import match
        from module_gate import check_group

        def read(path):
            return {"owners": list(owners)} if "manifest" in str(path) else inventory
        with patch.object(match, "read_json", return_value=inventory), patch.object(common, "read_json", read):
            return check_group(obj, [{"target_id": t, "public": p} for t, p in claims], oracle, **kw)

    def binding(self, obj, symbols, group_id="group:fixture"):
        from binding import OPERAND_MODE, declarations, fixes
        from omf import OmfReader
        module = OmfReader().read(obj)
        return {"schema": 1, "mode": OPERAND_MODE, "target_id": group_id, "symbols": symbols,
                "fixups": fixes(module), "declarations": declarations(module)}

    def test_own_segment_branch_is_exact_without_a_frame(self):
        # jmp done (E9, F0/T0 displacement 4); nop; done: retf. Linked rel16 = 4 - 3.
        oracle, inventory = synthetic_oracle(bytes(32) + bytes.fromhex("e9010090cb"), [("f", 2, 0, 5, [])])
        obj = masm_object(bytes.fromhex("e9000090cb"), [("F", 0)], (), [own_branch(1, 4)])
        report = self.check(obj, oracle, inventory, [("f", "F")])
        claim = report["claims"][0]
        self.assertEqual(claim["state"], "CODE_EXACT", claim)
        self.assertEqual([(e["kind"], e["linked_value"], e["destination"]) for e in claim["equations"]],
                         [("own-segment rel16", 1, 4)])

    def test_self_relative_field_that_is_not_a_branch_operand_is_refused(self):
        # The same fixup on MOV AX, imm16 would let a branch equation stand in for an address.
        oracle, inventory = synthetic_oracle(bytes(32) + bytes.fromhex("b80100cb"), [("f", 2, 0, 4, [])])
        obj = masm_object(bytes.fromhex("b80000cb"), [("F", 0)], (), [own_branch(1, 3)])
        report = self.check(obj, oracle, inventory, [("f", "F")])
        self.assertIn("not the rel16 operand", report["claims"][0]["reason"])

    def test_own_segment_branch_into_another_claim_is_refused(self):
        oracle, inventory = synthetic_oracle(bytes(32) + bytes.fromhex("e90100cbcb"),
                                             [("f", 2, 0, 4, []), ("g", 2, 4, 1, [])])
        obj = masm_object(bytes.fromhex("e90000cbcb"), [("F", 0), ("G", 4)], (), [own_branch(1, 4)])
        report = self.check(obj, oracle, inventory, [("f", "F"), ("g", "G")])
        self.assertIn("leaves its claim", report["claims"][0]["reason"])

    def plain_call(self, sites=True):
        # FIRST: call HELPER (plain E8, F0/T0 displacement 4); retf. HELPER: ret.
        call, site = far_call(16, 2, 0)
        image = bytearray(32) + bytes.fromhex("e80100cbc3")
        image[16:21] = call
        oracle, inventory = synthetic_oracle(image, [("first", 2, 0, 4, [site] if sites else []),
                                                     ("helper", 2, 4, 1, [])], relocations=[{"image_offset": 19}])
        obj = masm_object(bytes.fromhex("e80000cbc3"), [("FIRST", 0), ("HELPER", 4)], (), [own_branch(1, 4)])
        return self.check(obj, oracle, inventory, [("first", "FIRST"), ("helper", "HELPER")])

    def test_plain_near_call_to_an_object_public_needs_the_witnessed_window(self):
        claim = self.plain_call()["claims"][0]
        self.assertEqual(claim["state"], "CODE_EXACT", claim)
        self.assertEqual([(e["kind"], e["push_cs"], e["symbol"]) for e in claim["equations"]],
                         [("same-object near16", False, "HELPER")])
        unwitnessed = self.plain_call(sites=False)["claims"][0]
        self.assertEqual(unwitnessed.get("cs_frame"), "frame-invariant near calls", unwitnessed)

    def external(self, witnesses=None, *, callee=0x60, frame_segment=4, accepted=False):
        """Candidate at 0x40 (PUSH CS; CALL OUTSIDE; RETF), entered by CALL FAR frame_segment:off.
        Callee RETF at `callee`; an independent function at 0x80 calls it with a reached E8."""
        image = bytearray(0x90)
        entry, entry_site = far_call(0x10, frame_segment, 0x40 - frame_segment * 16)
        other, _ = far_call(0x20, 0, 0x80)
        image[0x10:0x15], image[0x20:0x25] = entry, other
        image[0x40:0x45] = bytes.fromhex("0ee8") + struct.pack("<H", (callee - 0x44) & 0xFFFF) + b"\xcb"
        image[callee] = 0xcb
        image[0x80:0x84] = b"\xe8" + struct.pack("<H", (callee - 0x83) & 0xFFFF) + b"\xcb"
        ident = "root:0000:%04x" % callee
        oracle, inventory = synthetic_oracle(image, [("caller", frame_segment, 0x40 - frame_segment * 16, 5,
                                                      [entry_site]), (ident, 0, callee, 1, [])],
                                             relocations=[{"image_offset": 0x13}, {"image_offset": 0x23}])
        inventory["functions"][1]["entry_kind"] = "direct-code"
        obj = masm_object(bytes.fromhex("0ee80000cb"), [("CALLER", 0)], ["OUTSIDE"], [near_external(2, 1)])
        if witnesses is None:
            witnesses = [{"space": "root", "image_offset": 0x80, "bytes_hex": image[0x80:0x83].hex(),
                          "entry": "root:0000:0080"}]
        symbol = {"kind": "near-code-entry", "space": "root", "segment": 0, "offset": callee,
                  "entry_target_id": ident, "entry_sha256": inventory["functions"][1]["sha256"],
                  "witnesses": witnesses}
        owners = [{"target_id": ident, "state": "ASM_EXACT"}] if accepted else []
        return self.check(obj, oracle, inventory, [("caller", "CALLER")], owners=owners,
                          binding=self.binding(obj, {"OUTSIDE": symbol}), group_id="group:fixture")

    def test_external_near_callee_grounded_by_an_outside_near_call_is_exact(self):
        claim = self.external()["claims"][0]
        self.assertEqual(claim["state"], "CODE_EXACT", claim)
        self.assertEqual([(e["kind"], e["linked_value"], e["original_value"]) for e in claim["equations"]],
                         [("external near16", 0x1c, 0x1c)])
        self.assertEqual(self.external([], accepted=True)["claims"][0]["state"], "CODE_EXACT")

    def test_external_near_callee_with_only_the_candidates_own_call_is_refused(self):
        own = [{"space": "root", "image_offset": 0x40, "bytes_hex": "0ee81c00", "entry": "root:0000:0040"}]
        with self.assertRaisesRegex(ValueError, "lies inside the object"):
            self.external(own)
        with self.assertRaisesRegex(ValueError, "no independent original near CALL"):
            self.external([])

    def test_external_near_callee_outside_the_cs_window_is_refused(self):
        # CS = 0004 (base 0x40): the callee at 0x30 lies below the window, so IP arithmetic
        # would wrap to 0x10030 although the linear equation agrees.
        claim = self.external(callee=0x30)["claims"][0]
        self.assertEqual(claim["state"], "REFUSED")
        self.assertIn("CS window", claim["reason"])
        self.assertEqual(self.external(callee=0x30, frame_segment=0)["claims"][0]["state"], "CODE_EXACT")


class MasmDgroupFrameTests(unittest.TestCase):
    """v4 accepts MASM's F1 DGROUP frame (ASSUME DS:DGROUP) only when the group is DGROUP."""

    def equations(self, group):
        from binding import OPERAND_MODE, data_equations, fixes, instructions
        from omf import OmfReader
        code = bytes.fromhex("a10000cb")  # mov ax, [SYM]; retf
        sub = fixup(1, relative=False, frame=1, frame_datum=1, target=6, target_datum=1)
        module = OmfReader().read(masm_object(code, [("F", 0)], ["SYM"], [sub], group=group))
        return data_equations(fixes(module), {"SYM": {"offset": 0x25c0}}, "FIX_TEXT", code,
                              bytes.fromhex("a1c025cb"), instructions(code), OPERAND_MODE, set())

    def test_dgroup_frame_is_the_grounded_offset_equation(self):
        found, unequal = self.equations("DGROUP")
        self.assertEqual((found[0]["linked_value"], unequal), (0x25c0, []))

    def test_frame_naming_another_group_is_refused(self):
        with self.assertRaisesRegex(ValueError, "Unsupported DGROUP fixup equation"):
            self.equations("OTHER")


@unittest.skipUnless(Path(load_lock()["runner"]["path"]).is_file(), "Pinned historical runner unavailable")
class AsmGroupTests(unittest.TestCase):
    """A MASM group assembles through compile_group and reports ASM states."""

    SOURCE = """.MODEL MEDIUM
.CODE
PUBLIC FIRST, HELPER
FIRST PROC FAR
    call HELPER
    jmp done
    REPT 130
    nop
    ENDM
done:
    retf
FIRST ENDP
HELPER PROC NEAR
    ret
HELPER ENDP
END
"""

    def test_masm_group_with_own_segment_calls_and_jumps_is_asm_exact(self):
        import match
        from module_gate import compile_group
        # MASM emits F0/T0 for both: call HELPER (137 - 3) and jmp done (136 - 6).
        linked = bytes.fromhex("e88600e98200") + b"\x90" * 130 + b"\xcb\xc3"
        call, site = far_call(16, 2, 0)
        image = bytearray(32) + linked
        image[16:21] = call
        oracle, inventory = synthetic_oracle(image, [("first", 2, 0, 137, [site]), ("helper", 2, 137, 1, [])],
                                             relocations=[{"image_offset": 19}])
        with patch.object(match, "read_json", return_value=inventory), \
                patch("oracle.Oracle.load", return_value=oracle):
            _, report = compile_group(self.SOURCE, "masm510", None,
                                      [{"target_id": "first", "public": "FIRST"},
                                       {"target_id": "helper", "public": "HELPER"}],
                                      ROOT / "build/workers/tests/asm_group", language="asm")
        self.assertEqual([c["state"] for c in report["claims"]], ["ASM_EXACT", "ASM_EXACT"], report["claims"])
        self.assertEqual(sorted(e["kind"] for e in report["claims"][0]["equations"]),
                         ["own-segment rel16", "same-object near16"])


@unittest.skipUnless(Path(load_lock()["runner"]["path"]).is_file(), "Pinned historical runner unavailable")
class DraftRegionFarWitnessTests(unittest.TestCase):
    """In a group, an original relocated CALL FAR in an unclaimed draft region may witness a far
    callee (an original fact the claims are not judged on); one inside a claim may not."""

    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        cls.oracle = Oracle.load()
        cls.root = cls.oracle.spaces["root"]
        # Any resident C group with a draft that calls a grounded far callee (drafts come and go as files complete).
        rows = {r["id"]: r for r in common.read_json(ROOT / "evidence/targets.json")["functions"]}
        for path in sorted((ROOT / "recipes").rglob("*.json")):
            recipe = common.read_json(path)
            if not (isinstance(recipe, dict) and recipe.get("kind") == "same_source_group" and
                    recipe.get("space") == "root" and recipe["source"].endswith(".c")):
                continue
            judged = [rows[c["target_id"]] for c in recipe["claims"] + recipe.get("code_matches", [])]
            spans = [(r["segment"] * 16 + r["offset"], r["segment"] * 16 + r["offset"] + r["size"]) for r in judged]
            binding = common.read_json(ROOT / recipe["binding"])
            callees = {(s["offset"], s["segment"]): n for n, s in binding["symbols"].items()
                       if s.get("kind") == "far-code-entry" and s.get("witnesses")}
            for r in cls.root.relocations:
                at = r["image_offset"] - 3
                if recipe["origin"] <= at < recipe["origin"] + recipe["emitted_size"] and cls.root.data[at] == 0x9a and \
                        not any(lo - 5 < at < hi for lo, hi in spans):
                    name = callees.get(struct.unpack_from("<HH", cls.root.data, at + 1))
                    if name:
                        result = compile_c((ROOT / recipe["source"]).read_text(encoding="utf-8"), recipe["profile"],
                                           recipe["flags"], workdir=ROOT / "build/workers/tests/far_draft")
                        assert result.ok, result.log
                        cls.obj, cls.recipe, cls.binding, cls.draft_call, cls.callee = \
                            result.obj_bytes, recipe, binding, at, name
                        return
        raise unittest.SkipTest("no resident C group has an open draft calling a grounded far callee")

    def witness(self, at):
        relocation = next(r for r in self.root.relocations if r["image_offset"] == at + 3)
        return {"space": "root", "image_offset": at, "bytes_hex": self.root.data[at:at+5].hex(),
                "relocation_index": relocation["index"], "file_offset": self.root.file_offset + at}

    def check(self, at):
        from module_gate import check_group
        binding = copy.deepcopy(self.binding)
        binding["symbols"][self.callee]["witnesses"] = [self.witness(at)]
        return check_group(self.obj, recipe_claims(self.recipe), self.oracle, binding=binding,
                           group_id=self.recipe["group_id"])

    def test_call_in_an_unclaimed_draft_region_witnesses_the_callee(self):
        self.assertTrue(exact_except_matches(self.check(self.draft_call), self.recipe))

    def test_call_inside_a_claim_is_refused(self):
        report = self.check(self.draft_call)
        claimed = next(c for c in report["claims"] for e in c["equations"]
                       if e.get("kind") == "far-call" and e.get("symbol") == self.callee)
        at = report["origin"] + claimed["object_offset"] + next(
            e["offset"] for e in claimed["equations"] if e.get("symbol") == self.callee) - 1
        with self.assertRaisesRegex(ValueError, "overlaps candidate"):
            self.check(at)


class ReachedNearFrameTests(unittest.TestCase):
    """A root claim with no relocated far caller takes CS = F from an original near CALL that
    is reached from an entry a relocated CALL FAR names as F:offset."""

    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        from match import get_target
        cls.oracle = Oracle.load()
        cls.target, cls.space, raw = get_target("root:1101:7364", cls.oracle)
        cls.entry = cls.space.position(cls.target["segment"], cls.target["offset"])
        cls.size = len(raw)

    def frames(self, hull):
        from module_gate import _reached_near_frames
        return _reached_near_frames(self.target, self.space, self.oracle, hull)

    def test_near_call_reached_from_a_far_entry_names_the_frame(self):
        found = self.frames((self.entry, self.entry + self.size))
        self.assertEqual(found[0]["frame"], 0x1101)
        self.assertEqual(found[0]["kind"], "reached near CALL")

    def test_a_near_call_inside_the_object_does_not_count(self):
        at = self.frames((self.entry, self.entry + self.size))[0]["image_offset"]
        lo = min(at, self.entry)
        hi = max(at + 3, self.entry + self.size)
        others = [f for f in self.frames((lo, hi)) if f["image_offset"] == at]
        self.assertEqual(others, [])

    def test_an_overlay_far_caller_names_the_frame_of_a_root_entry(self):
        # root:1101:4ea8 is entered by a relocated CALL FAR in overlay 2 (relocation 29); its near
        # calls reach the cursor routine at 042f, which therefore runs with CS = 1101.
        from match import get_target
        from module_gate import _reached_near_frames
        target, space, raw = get_target("root:1101:042f", self.oracle)
        entry = space.position(target["segment"], target["offset"])
        found = _reached_near_frames(target, space, self.oracle, (entry, entry + len(raw)))
        self.assertEqual((found[0]["frame"], found[0]["entry"]), (0x1101, "root:1101:4ea8"))


@unittest.skipUnless(Path(load_lock()["runner"]["path"]).is_file(), "Pinned historical runner unavailable")
class OwnBaseBucketTests(unittest.TestCase):
    """RTLink buckets relocations by target identity: `mov ax, SEG MarkMove` style LOC2 bases of
    three different publics of this object are three buckets, not one segment bucket."""

    def test_move_mark_mob_redraw_buckets_by_public(self):
        from module_gate import check_group
        from oracle import Oracle
        recipe = common.read_json(ROOT / "recipes/move/move.json")
        result = compile_c((ROOT / recipe["source"]).read_text(encoding="utf-8"), recipe["profile"],
                           recipe["flags"], workdir=ROOT / "build/workers/tests/own_base_buckets")
        self.assertTrue(result.ok, result.log)
        claims = recipe_claims(recipe)
        if not any(c["target_id"] == "root:0b41:2264" for c in claims):
            claims.append({"target_id": "root:0b41:2264", "public": "MARK_MOB_REDRAW"})
        report = check_group(result.obj_bytes, claims, Oracle.load(),
                             binding=common.read_json(ROOT / recipe["binding"]), group_id=recipe["group_id"])
        claim = next(c for c in report["claims"] if c["public"] == "MARK_MOB_REDRAW")
        self.assertEqual(claim["state"], "CODE_EXACT", claim.get("reason"))
        self.assertEqual(sorted(g["symbol"] for g in claim["relocation_groups"]),
                         ["MARK_FLOOR", "MARK_FOREGROUND", "MARK_MOVE"])


class FrameInvariantTests(unittest.TestCase):
    """A claim without its own frame witness: its near calls are proven only when every frame
    the original stores whose window holds the claim also holds each callee."""

    def run_check(self, frames, near, position=0x12000, size=0x40):
        import module_gate
        space = SimpleNamespace(name="root", link_segment=0, data=b"",
                                relocations=[{"segment": f} for f in frames])
        oracle = SimpleNamespace(spaces={"root": space})
        state = {"near": near, "position": position, "piece": bytes(size)}
        with patch.object(module_gate, "_code_vectors", return_value={}), \
                patch.dict(module_gate._CANDIDATE_FRAMES, clear=True):
            return module_gate._frame_invariant(state, space, oracle)

    def call(self, callee_at, equal=True):
        return {"pair_start": 0x12010, "field_end": 0x12014, "callee_at": callee_at, "equal": equal}

    def test_callee_inside_every_window_is_proven(self):
        self.assertTrue(self.run_check([0x1000, 0x1101], [self.call(0x12100)]))

    def test_a_window_that_wraps_away_from_the_callee_refuses(self):
        # Frame 0x1101's window ends at 0x21010; frame 0x0300's window ends at 0x13000, so a
        # callee at 0x13100 is reached under 0x1101 but wraps under 0x0300.
        self.assertFalse(self.run_check([0x1101, 0x0300], [self.call(0x13100)]))

    def test_no_stored_frame_holding_the_claim_refuses(self):
        self.assertFalse(self.run_check([0x3000], [self.call(0x12100)]))

    def test_unequal_near_call_refuses(self):
        self.assertFalse(self.run_check([0x1000], [self.call(0x12100, equal=False)]))


class FarPointerFrameTests(unittest.TestCase):
    """A relocated far pointer naming a root entry (the window callback table in DGROUP at
    overlay-17+0x1320 holds 1101:8738) is CS-frame evidence like a relocated CALL FAR."""

    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        cls.oracle = Oracle.load()
        rows = [f for f in common.read_json(ROOT / "evidence/targets.json")["functions"]
                if f["id"] == "root:1101:8738"]
        if not rows:
            raise unittest.SkipTest("root:1101:8738 is not a reviewed target")
        cls.row = rows[0]
        holder = cls.oracle.spaces["overlay-17"]
        cls.at = 0x1320
        cls.site = {"kind": "far_pointer", "space": "overlay-17", "image_offset": cls.at,
                    "file_offset": holder.file_offset + cls.at, "relocated_segment_site": cls.at + 2}

    def target(self, site):
        import match
        row = dict(self.row, call_sites=[site])
        inventory = dict(common.read_json(ROOT / "evidence/targets.json"))
        inventory["functions"] = [f for f in inventory["functions"] if f["id"] != row["id"]] + [row]
        with patch.object(match, "read_json", return_value=inventory):
            return match.get_target(row["id"], self.oracle)[0]

    def test_relocated_far_pointer_names_the_frame(self):
        from module_gate import _frame_evidence
        target = self.target(self.site)
        space = self.oracle.spaces["root"]
        evidence, _ = _frame_evidence(target, space, self.oracle, (0, 0))
        self.assertIn({"frame": 0x1101, "kind": "relocated far pointer", "space": "overlay-17",
                       "image_offset": self.at}, evidence)

    def test_pointer_without_its_relocation_or_at_the_wrong_word_refuses(self):
        shifted = dict(self.site, image_offset=self.at + 2, file_offset=self.site["file_offset"] + 2,
                       relocated_segment_site=self.at + 4)
        with self.assertRaisesRegex(ValueError, "far pointer"):
            self.target(shifted)
        with self.assertRaisesRegex(ValueError, "provenance"):
            self.target(dict(self.site, relocated_segment_site=self.at + 3))
