"""Same-source groups owning RTLink overlay code: CS frame evidence, far calls with the
overlay's own relocation table, and v4 DGROUP data (2026-11-21)."""
import copy
import struct
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import common

ROOT = Path(__file__).resolve().parents[1]
SELF_REL, OFFSET16, POINTER32 = 0x84, 0xc4, 0xcc  # FIXUPP locat high bytes (LOC1 rel, LOC1, LOC3)


def _name(text):
    return bytes([len(text)]) + text.encode("ascii")


def _record(kind, body):
    raw = bytes([kind]) + struct.pack("<H", len(body) + 1) + body
    return raw + bytes([-sum(raw) % 256])


GROUP = bytes([6, 0xff, 2])  # GRPDEF body: DGROUP = {_DATA}


def omf(code, publics, externals=(), fixes=(), group=GROUP):
    """One CODE LEDATA, an empty _DATA in DGROUP; each fix is (offset, EXTDEF index, locat), F5/T6, no D."""
    records = [_record(0x80, _name("fixture")),
               _record(0x96, b"".join(map(_name, ("", "FIX_TEXT", "CODE", "_DATA", "DATA", "DGROUP")))),
               _record(0x98, b"\x48" + struct.pack("<H", len(code)) + b"\x02\x03\x01"),
               _record(0x98, b"\x48\x00\x00\x04\x05\x01"), _record(0x9a, group)]
    if externals:
        records.append(_record(0x8c, b"".join(_name(e) + b"\x00" for e in externals)))
    records.append(_record(0x90, b"\x00\x01" + b"".join(_name(p) + struct.pack("<H", o) + b"\x00"
                                                       for p, o in publics)))
    records.append(_record(0xa0, b"\x01\x00\x00" + code))
    if fixes:
        records.append(_record(0x9c, b"".join(struct.pack(">HBB", (locat << 8) | at, 0x56, index)
                                              for at, index, locat in fixes)))
    records.append(_record(0x8a, b"\x00"))
    return b"".join(records)


def check(obj, oracle, claims, *, inventory=None, **kw):
    import binding
    import match
    from module_gate import check_group
    binding._REACH.clear()
    claims = [{"target_id": t, "public": p} for t, p in claims]
    if inventory is None:
        return check_group(obj, claims, oracle, **kw)
    with patch.object(match, "read_json_shared", return_value=inventory):
        return check_group(obj, claims, oracle, **kw)


def no_site_frames():
    import module_gate
    return patch.object(module_gate, "_site_frame", return_value=None)


class RealOverlayTests(unittest.TestCase):
    """Synthetic OMF objects exercise v1.1 targets in the expanded overlays."""

    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        from match import get_target
        cls.oracle = Oracle.load()
        cls.ov3 = cls.oracle.spaces["overlay-3"]
        cls.inventory = common.read_json(ROOT / "evidence/targets.json")
        cls.get_target = staticmethod(get_target)

    # -- near call and CS frame: AUTO 251b:08ac (draft) and 251b:08e8 (claim) ------------------
    def near_object(self):
        lo, hi = 0x8ac, 0x8e8 + 318
        code = bytearray(self.ov3.data[lo:hi])
        field = 0x927 - lo  # 0E E8 rel16 at 251b:0925 calls 251b:08ac
        self.assertEqual(code[field - 2:field], b"\x0e\xe8")
        code[field:field + 2] = bytes(2)
        return omf(bytes(code), [("AUTO_08AC", 0), ("AUTO_08E8", 0x3c)], ["AUTO_08AC"], [(field, 1, SELF_REL)])

    def test_same_overlay_call_witnesses_the_claims_frame(self):
        report = check(self.near_object(), self.oracle, [("overlay-3:251b:08e8", "AUTO_08E8")])
        claim = report["claims"][0]
        self.assertEqual(claim["state"], "CODE_EXACT", claim)
        self.assertEqual((report["space"], report["frame"]), ("overlay-3", 0x251b))
        self.assertEqual([(e["kind"], e["frame"]) for e in claim["frame_evidence"]],
                         [("same-overlay CALL FAR", 0x251b), ("overlay site frame", 0x251b)])
        self.assertEqual(claim["frame_evidence"][0]["image_offset"], 0x3065)
        self.assertEqual([e["kind"] for e in claim["equations"]], ["same-object near16"])

    def without_calls(self):
        inventory = copy.deepcopy(self.inventory)
        next(t for t in inventory["functions"] if t["id"] == "overlay-3:251b:08e8")["call_sites"] = []
        return inventory

    def test_overlay_site_frames_witness_a_claim_without_far_entries(self):
        # Overlay-3 records 52 (site 0x07ad) and 69 (site 0x0ae1) bracket the claim
        # with site frame 251b after its direct callers are removed.
        report = check(self.near_object(), self.oracle, [("overlay-3:251b:08e8", "AUTO_08E8")],
                       inventory=self.without_calls())
        claim = report["claims"][0]
        self.assertEqual(claim["state"], "CODE_EXACT", claim)
        self.assertEqual([(e["kind"], e["frame"], e["bracket_relocation_indices"])
                          for e in claim["frame_evidence"]], [("overlay site frame", 0x251b, [52, 69])])
        with no_site_frames():
            claim = check(self.near_object(), self.oracle, [("overlay-3:251b:08e8", "AUTO_08E8")],
                          inventory=self.without_calls())["claims"][0]
        self.assertEqual((claim["state"], claim["frame_evidence"]), ("REFUSED", []))
        self.assertIn("witnessed original frame", claim["reason"])

    def test_site_frames_on_reviewed_overlay_targets(self):
        import module_gate

        def evidence(ident):
            target, space, _ = self.get_target(ident, self.oracle)
            at = space.position(target["segment"], target["offset"])
            return module_gate._frame_evidence(target, space, self.oracle, (at, at + target["size"]))
        # ACT GenCtrl/Falling/HitFloor and COLLIDE 000e: root calls are shared with overlay 2.
        for ident, frame in [("overlay-3:27bc:048c", 0x27bc), ("overlay-3:27bc:0096", 0x27bc),
                             ("overlay-3:27bc:01f0", 0x27bc), ("overlay-3:29ef:000c", 0x29ef)]:
            found, refused = evidence(ident)
            self.assertEqual([(e["kind"], e["frame"]) for e in found], [("overlay site frame", frame)], ident)
            self.assertTrue(all("overlay-2" in r["reason"] for r in refused), ident)
        # 27bc:0006 precedes every 27bc record and follows 2763's: no bracket (its vector still names it).
        found, refused = evidence("overlay-3:27bc:0006")
        self.assertEqual(([e["kind"] for e in found],
                          [r["reason"] for r in refused if r.get("kind") == "overlay site frame"]),
                         (["rtlink-vector"], ["no relocation records of one site frame bracket the claim"]))
        # The 2bda:0b0e link address lies in overlay 9's 2c59 frame, but overlays
        # holding that address bracket it with site frame 2bda: the linker resolved it there.
        self.assertTrue(module_gate._other_overlay_frame(self.oracle, self.oracle.spaces["overlay-9"],
                                                         0x2bda, 0x0b0e))
        found, refused = evidence("overlay-9:2c59:0368")
        self.assertEqual([(e["kind"], e["frame"]) for e in found], [("overlay site frame", 0x2c59)])
        # Root 255a:000c is NIS's first entry in overlay 2, before its first 255a record: the next
        # record names 255a and earlier ones smaller frames, so it resolves into overlay 2.
        self.assertTrue(module_gate._other_overlay_frame(self.oracle, self.oracle.spaces["overlay-3"],
                                                         0x255a, 0x000c))
        # A frame no other overlay brackets at that address stays an alias.
        self.assertFalse(module_gate._other_overlay_frame(self.oracle, self.oracle.spaces["overlay-9"],
                                                          0x2c5a, 0x0a70))

    def test_root_site_frames_bracket_a_gamelib_claim(self):
        # ITEMDLG's root 1129:a19d has no far caller; root MZ records before and after it name 1129.
        import module_gate
        target, space, _ = self.get_target("root:1129:a19d", self.oracle)
        at = space.position(target["segment"], target["offset"])
        verdict, row = module_gate._site_frame(target, space, self.oracle, (at, at + target["size"]))
        self.assertEqual((verdict, row["kind"], row["frame"]), ("evidence", "root site frame", 0x1129))
        found, _ = module_gate._frame_evidence(target, space, self.oracle, (at, at + target["size"]))
        self.assertEqual([(e["kind"], e["frame"]) for e in found], [("root site frame", 0x1129)])

    def test_section_start_brackets_final_dispatcher(self):
        # FINAL 2bda:0000 precedes overlay 10's first record (index 171 at 0x74), which names the
        # link segment 2bda: the segment holding position 0 has that floor, so the claim does.
        import module_gate
        target, space, _ = self.get_target("overlay-10:2bda:0000", self.oracle)
        found, refused = module_gate._frame_evidence(target, space, self.oracle, (0, target["size"]))
        self.assertEqual([(e["kind"], e["frame"], e["bracket_relocation_indices"]) for e in found],
                         [("overlay site frame", 0x2bda, ["section-start", 171])])
        # 27bc:0006 has 2763 records before it: the section start never replaces a real record.
        target, space, _ = self.get_target("overlay-3:27bc:0006", self.oracle)
        at = space.position(target["segment"], target["offset"])
        self.assertEqual(module_gate._site_frame(target, space, self.oracle, (at, at + target["size"]))[0],
                         "refused")

    def test_foreign_frames_from_the_overlays_own_records(self):
        import module_gate
        ov2, ov9 = self.oracle.spaces["overlay-2"], self.oracle.spaces["overlay-9"]
        # Overlay 2's 255a records cross the floor of 29ef. That frame cannot
        # belong to overlay 2's ANIM segment (2ab1).
        self.assertTrue(module_gate._foreign_frame(ov2, 0x2ab1, 0x29ef))
        self.assertTrue(module_gate._foreign_frame(ov2, 0x2ab1, 0x2b96))  # above the claim's floor
        self.assertFalse(module_gate._foreign_frame(ov2, 0x2ab1, 0x255a))  # a real overlay-2 floor
        # Root+0x4509 stores 2c9e:136c inside DRAWTEMP (frame 2d56).
        self.assertTrue(module_gate._foreign_frame(ov9, 0x2d56, 0x2c9e))
        self.assertFalse(module_gate._foreign_frame(ov9, 0x2d56, 0x2c58))  # below the section
        target, space, _ = self.get_target("overlay-9:2d56:0e1a", self.oracle)
        self.assertEqual(module_gate._site_frame(target, space, self.oracle, (0x102a, 0x1ef4))[1]["frame"],
                         0x2d56)
        with patch.object(module_gate, "_foreign_frame", return_value=False):
            verdict, row = module_gate._site_frame(target, space, self.oracle, (0x102a, 0x1ef4))
        self.assertEqual(verdict, "refused")
        self.assertIn("enters the object with frame 2c9e", row["reason"])

    def far_pointer_row(self, ident, holder, at):
        import match
        target, space, _ = self.get_target(ident, self.oracle)
        row = copy.deepcopy(target)
        row["call_sites"] = [{"kind": "far_pointer", "space": holder, "image_offset": at,
                              "relocated_segment_site": at + 2,
                              "file_offset": self.oracle.spaces[holder].file_offset + at}]
        with patch.object(match, "EXTRA_TARGETS", [row]):
            return self.get_target(ident, self.oracle)[:2]  # match validates the pointer

    def test_far_pointer_to_an_unshared_overlay_address_witnesses_the_frame(self):
        # overlay-17+0x4326 stores 2d56:0ee6; no other space holds that link address.
        import module_gate
        target, space = self.far_pointer_row("overlay-9:2d56:0ee6", "overlay-17", 0x4326)
        at = space.position(target["segment"], target["offset"])
        found, refused = module_gate._frame_evidence(target, space, self.oracle, (at, at + target["size"]))
        self.assertEqual([(e["kind"], e["frame"], e["space"], e["image_offset"]) for e in found],
                         [("relocated far pointer (unshared link address)", 0x2d56, "overlay-17", 0x4326)])
        self.assertEqual([r["reason"] for r in refused],
                         ["no relocation records of one site frame bracket the claim"])

    def test_far_pointer_to_a_shared_overlay_address_is_not_frame_evidence(self):
        # overlay-17+0x4418 stores 2bda:0000, an address overlays 4-7 also hold.
        import module_gate
        target, space = self.far_pointer_row("overlay-10:2bda:0000", "overlay-17", 0x4418)
        with no_site_frames():
            found, refused = module_gate._frame_evidence(target, space, self.oracle, (0, target["size"]))
        self.assertEqual(found, [])
        self.assertEqual([r["reason"] for r in refused],
                         ["far pointer link address also held by overlay-4, overlay-5, overlay-6, overlay-7"])

    def test_vector_witnesses_only_its_own_destination(self):
        import module_gate
        real = module_gate._code_vectors(self.oracle)
        vector = dict(next(v for v in real.values() if v["target_space"] == "overlay-3"))

        def run(position):
            fake = {(0x220e, 0x0d0d): dict(vector, target_segment=0x251b, target_offset=position,
                                           target_position=position)}
            with patch.object(module_gate, "_code_vectors", return_value=fake), \
                    no_site_frames():
                return check(self.near_object(), self.oracle, [("overlay-3:251b:08e8", "AUTO_08E8")],
                             inventory=self.without_calls())["claims"][0]
        claim = run(0x8e8)
        self.assertEqual(claim["state"], "CODE_EXACT", claim)
        self.assertEqual(claim["frame_evidence"][0]["kind"], "rtlink-vector")
        self.assertIn("witnessed original frame", run(0x8ac)["reason"])

    def test_cross_space_call_to_a_shared_link_address_is_not_frame_evidence(self):
        # Root 0xd8dd calls 251b:08ac; overlay 2 shares link segment 251b and holds that address.
        inventory = self.without_calls()
        raw = self.ov3.data[0x8ac:0x8e8]
        # The test's own row with exactly this root caller (replacing any canonical review).
        inventory["functions"] = [f for f in inventory["functions"] if f["id"] != "overlay-3:251b:08ac"]
        inventory["functions"].append({
            "id": "overlay-3:251b:08ac", "space": "overlay-3", "segment": 0x251b, "offset": 0x8ac,
            "size": 0x3c, "sha256": common.sha(raw), "boundary_status": "REVIEWED",
            "boundary_evidence": ["test"], "call_sites": [
                {"kind": "far_call", "space": "root", "image_offset": 0xd8dd,
                 "relocated_segment_site": 0xd8e0, "file_offset": self.oracle.spaces["root"].file_offset + 0xd8dd}]})
        with no_site_frames():  # this test isolates the cross-space call rule
            report = check(self.near_object(), self.oracle, [("overlay-3:251b:08ac", "AUTO_08AC"),
                                                             ("overlay-3:251b:08e8", "AUTO_08E8")], inventory=inventory)
        first, second = report["claims"]
        self.assertEqual(first["state"], "CODE_EXACT")  # no near call: needs no frame
        self.assertIn("overlay-2", first["frame_evidence_refused"][0]["reason"])
        self.assertIn("witnessed original frame", second["reason"])

    # FINAL MoveFireball retains its three relocated resident calls and DGROUP accesses in v1.1.
    FAR = {0x65: "root:1129:83d2", 0x86: "root:0dee:01c6", 0xA7: "root:0790:2b1c"}
    DATA = {0x03: 0x6667, 0x0D: 0x666D, 0x14: 0x666D, 0x20: 0x6667,
            0x2C: 0x6669, 0x33: 0x6669, 0x3A: 0x6669, 0x42: 0x6669,
            0x49: 0x6669, 0x4D: 0x6669, 0x51: 0x6662, 0x55: 0x6666,
            0x5C: 0x6667, 0x72: 0x666D, 0x8C: 0x6667, 0x93: 0x5B48,
            0xB0: 0x666D, 0xB5: 0x666D}
    # Independent resident accesses, with their reviewed entries and encoded disp16 offsets.
    DATA_WITNESSES = {0x6667: (52475, "a36766", 1, "root:0b52:0006"),
                      0x666D: (52487, "c7066d660000", 2, "root:0b52:0006"),
                      0x6669: (52465, "c70669660000", 2, "root:0b52:0006"),
                      0x6662: (52436, "a36266", 1, "root:0b52:0006"),
                      0x6666: (52462, "a26666", 1, "root:0b52:0006"),
                      0x5B48: (4137, "a0485b", 1, "root:0000:02be")}

    def fireball_case(self, literal_far=()):
        from binding import OPERAND_MODE, declarations, fixes
        from omf import OmfReader
        _, _, original = self.get_target("overlay-10:2bda:1ed0", self.oracle)
        code = bytearray(original)
        names, fixups, symbols = [], [], {}
        for at, ident in self.FAR.items():
            if ident in literal_far:
                continue
            names.append("F_" + ident[-9:].replace(":", "_"))
            code[at:at + 4] = bytes(4)
            fixups.append((at, len(names), POINTER32))
            callee, _, _ = self.get_target(ident, self.oracle)
            calls = [c for c in callee["call_sites"] if c["kind"] == "far_call" and c["space"] == "root"]
            root = self.oracle.spaces["root"]
            symbols[names[-1]] = {
                "kind": "far-code-entry", "space": "root", "segment": callee["segment"],
                "offset": callee["offset"], "entry_target_id": ident, "entry_sha256": callee["sha256"],
                "witnesses": [{"space": "root", "image_offset": c["image_offset"],
                               "bytes_hex": root.data[c["image_offset"]:c["image_offset"] + 5].hex(),
                               "relocation_index": next(r["index"] for r in root.relocations
                                                        if r["image_offset"] == c["image_offset"] + 3),
                               "file_offset": c["file_offset"]} for c in calls[:1]]}
        for at, value in self.DATA.items():
            name = "D_%04x" % value
            if name not in names:
                names.append(name)
                pos, raw, operand, entry = self.DATA_WITNESSES[value]
                symbols[name] = {"kind": "dgroup-offset", "group": "DGROUP", "offset": value,
                                 "witnesses": [{"space": "root", "image_offset": pos, "bytes_hex": raw,
                                                "operand_offset": operand, "entry": entry}]}
            code[at:at + 2] = bytes(2)
            fixups.append((at, names.index(name) + 1, OFFSET16))
        obj = omf(bytes(code), [("MOVE_FIREBALL", 0)], names, fixups)
        module = OmfReader().read(obj)
        frame = common.read_json(ROOT / "evidence/bindings/convert_text_handle.json")["frame"]
        binding = {"schema": 1, "mode": OPERAND_MODE, "target_id": "group:overlay_fireball",
                   "target_sha256": common.sha(self.oracle.data), "frame": frame, "symbols": symbols,
                   "fixups": fixes(module), "declarations": declarations(module)}
        return obj, binding

    def test_overlay_far_calls_and_v4_data_are_exact(self):
        obj, binding = self.fireball_case()
        report = check(obj, self.oracle, [("overlay-10:2bda:1ed0", "MOVE_FIREBALL")], binding=binding,
                       group_id="group:overlay_fireball")
        claim = report["claims"][0]
        self.assertEqual(claim["state"], "CODE_EXACT", claim)
        self.assertEqual(claim["covered_relocation_indices"], [29, 109, 155])  # overlay-10's own table
        self.assertEqual(sorted(e["kind"] for e in claim["equations"]).count("far-call"), 3)
        self.assertEqual(report["frame"], 0x2bda)

    def test_overlay_relocation_not_generated_by_a_proven_field_is_refused(self):
        obj, binding = self.fireball_case(literal_far=("root:1129:83d2",))
        claim = check(obj, self.oracle, [("overlay-10:2bda:1ed0", "MOVE_FIREBALL")], binding=binding,
                      group_id="group:overlay_fireball")["claims"][0]
        self.assertEqual(claim["state"], "REFUSED")
        self.assertIn("relocation site set differs", claim["reason"])

    def test_data_witness_inside_the_object_is_refused(self):
        obj, binding = self.fireball_case()
        binding["symbols"]["D_6667"]["witnesses"] = [
            {"space": "overlay-10", "image_offset": 0x1ED1, "bytes_hex": "833e676600",
             "operand_offset": 2, "entry": "overlay-10:2bda:1ed0"}]
        with self.assertRaisesRegex(ValueError, "overlaps candidate"):
            check(obj, self.oracle, [("overlay-10:2bda:1ed0", "MOVE_FIREBALL")], binding=binding,
                  group_id="group:overlay_fireball")

    def test_direct_call_into_an_overlapping_overlay_is_refused(self):
        from far_binding import ground_symbol
        hull = {"id": "group:overlay_coexist", "space": "overlay-3", "segment": 0x27bc, "offset": 0xe, "size": 136}

        def symbol(ident):
            callee, space, _ = self.get_target(ident, self.oracle)
            witnesses = []
            for call in [c for c in callee["call_sites"] if c["kind"] == "far_call"][:1]:
                caller = self.oracle.spaces[call["space"]]
                at = call["image_offset"]
                witnesses.append({"space": caller.name, "image_offset": at,
                                  "bytes_hex": caller.data[at:at + 5].hex(),
                                  "relocation_index": next(r["index"] for r in caller.relocations
                                                           if r["image_offset"] == at + 3),
                                  "file_offset": caller.file_offset + at})
            return {"kind": "overlay-code-entry", "space": space.name, "segment": callee["segment"],
                    "offset": callee["offset"], "entry_target_id": ident, "entry_sha256": callee["sha256"],
                    "witnesses": witnesses}
        # Overlay 4 (link 2bda, accepted owner at 0aa0) lies above overlay 3's allocation
        # at 251b: both can be resident.
        self.assertEqual(ground_symbol(symbol("overlay-4:2bda:0aa0"), hull, self.oracle)["kind"],
                         "overlay-code-entry")
        # Overlay 2 shares link segment 251b with overlay 3 (its reviewed 2ab1:0e22 entry).
        import match
        inventory = copy.deepcopy(self.inventory)
        next(t for t in inventory["functions"] if t["id"] == "overlay-2:2ab1:0e22")["entry_kind"] = "direct-code"
        with patch.object(match, "read_json_shared", return_value=inventory), \
                self.assertRaisesRegex(ValueError, "resident together"):
            ground_symbol(symbol("overlay-2:2ab1:0e22"), hull, self.oracle)

    def test_older_resident_data_modes_still_refuse_overlay_owners(self):
        from binding import ground
        _, binding = self.fireball_case()
        target, _, _ = self.get_target("overlay-10:2bda:1ed0", self.oracle)
        target = dict(target, id="group:overlay_fireball")
        with self.assertRaisesRegex(ValueError, "Unsupported owner space"):
            ground(dict(binding, mode="external-dgroup-offset16-v1", symbols={}), target, self.oracle)


class SyntheticOverlayFrameTests(unittest.TestCase):
    """Overlay `overlay-2` at link segment 0x0100; optional overlay-3 sharing it."""

    def setUp(self):
        import binding
        binding._REACH.clear()

    CODE = "0ee80200cb90cb" + "9a40000001cb"  # caller; callee at +6; draft at +7 calling 0100:0040

    def case(self, *, caller_space=None, caller_at=None, sharer=False, records=(), foreign=None,
             group=GROUP, pointer_at=None):
        """`records`: extra overlay-2 relocation sites (position, site segment) holding zero words;
        `foreign`: (segment, offset) of a relocated root CALL FAR at root+0x20;
        `pointer_at`: root position of a reviewed relocated far pointer 0100:0040."""
        from oracle import Space
        # Overlay object at 0x40: PUSH CS; CALL near +2; RETF; NOP | RETF | draft CALL FAR; RETF.
        data = bytearray(0x80)
        data[0x40:0x4d] = bytes.fromhex(self.CODE)
        root = bytearray(0x40)
        overlay_relocs, root_relocs, sites = [], [], []
        if caller_space:
            holder = data if caller_space == "overlay-2" else root
            holder[caller_at:caller_at + 5] = struct.pack("<BHH", 0x9a, 0x40, 0x0100)
            reloc = {"image_offset": caller_at + 3, "segment": 0x0100 if caller_space != "root" else 0}
            (root_relocs if caller_space == "root" else overlay_relocs).append(reloc)
            file_offset = (0x1000 if caller_space == "overlay-2" else 0) + caller_at
            sites.append({"kind": "far_call", "space": caller_space, "image_offset": caller_at,
                          "relocated_segment_site": caller_at + 3, "file_offset": file_offset})
        overlay_relocs += [{"image_offset": at, "segment": segment} for at, segment in records]
        if foreign:
            root[0x20:0x25] = struct.pack("<BHH", 0x9a, foreign[1], foreign[0])
            root_relocs.append({"image_offset": 0x23, "segment": 0})
        if pointer_at is not None:
            root[pointer_at:pointer_at + 4] = struct.pack("<HH", 0x40, 0x0100)
            root_relocs.append({"image_offset": pointer_at + 2, "segment": 0})
            sites.append({"kind": "far_pointer", "space": "root", "image_offset": pointer_at,
                          "relocated_segment_site": pointer_at + 2, "file_offset": pointer_at})
        for rows, image in ((overlay_relocs, data), (root_relocs, root)):
            rows.sort(key=lambda r: r["image_offset"])
            for i, r in enumerate(rows):
                r.update(index=i, stored_word=struct.unpack_from("<H", image, r["image_offset"])[0])
        descriptor = {"flags": 4, "f8": 8, "link_segment": 0x0100}
        spaces = {"root": Space("root", 0, 0, bytes(root), tuple(root_relocs)),
                  "overlay-2": Space("overlay-2", 0x0100, 0x1000, bytes(data), tuple(overlay_relocs), descriptor)}
        if sharer:
            spaces["overlay-3"] = Space("overlay-3", 0x0100, 0x2000, bytes(0x80), (), dict(descriptor))
        oracle = SimpleNamespace(spaces=spaces, data=bytes(root))
        rows = [{"id": "caller", "space": "overlay-2", "segment": 0x0100, "offset": 0x40, "size": 6,
                 "sha256": common.sha(bytes(data[0x40:0x46])), "boundary_status": "REVIEWED",
                 "boundary_evidence": ["synthetic"], "call_sites": sites}]
        inventory = {"target_sha256": common.sha(oracle.data), "functions": rows}
        obj = omf(bytes.fromhex(self.CODE.replace("0ee80200", "0ee80000", 1)),
                  [("CALLER", 0), ("CALLEE", 6), ("DRAFT", 7)], ["CALLEE"], [(2, 1, SELF_REL)], group)
        import module_gate
        with patch.object(module_gate, "_code_vectors", return_value={}):
            return check(obj, oracle, [("caller", "CALLER")], inventory=inventory)["claims"][0]

    def test_same_overlay_call_outside_the_object_is_exact(self):
        claim = self.case(caller_space="overlay-2", caller_at=0x10)
        self.assertEqual(claim["state"], "CODE_EXACT", claim)

    def test_same_overlay_call_inside_the_object_is_refused(self):
        # The draft's CALL FAR, inside the object. Its own relocation record (site frame 0100)
        # brackets the claim with the section start, so isolate the CALL FAR rule.
        with no_site_frames():
            claim = self.case(caller_space="overlay-2", caller_at=0x47)
        self.assertIn("inside the object", claim["frame_evidence_refused"][0]["reason"])
        self.assertIn("witnessed original frame", claim["reason"])

    def test_cross_space_call_counts_only_for_an_unshared_link_address(self):
        self.assertEqual(self.case(caller_space="root", caller_at=0x10)["state"], "CODE_EXACT")
        claim = self.case(caller_space="root", caller_at=0x10, sharer=True)
        self.assertIn("overlay-3", claim["frame_evidence_refused"][0]["reason"])
        self.assertIn("witnessed original frame", claim["reason"])

    # -- overlay site frames (evidence/rtlink-probe/site-frames/) --------------------------------
    def test_site_frames_bracketing_the_claim_witness_its_frame(self):
        # A sharing overlay holds the address, and no call reaches the claim at all.
        claim = self.case(sharer=True, records=[(0x20, 0x0100), (0x60, 0x0100)])
        self.assertEqual(claim["state"], "CODE_EXACT", claim)
        self.assertEqual([(e["kind"], e["frame"], e["bracket_relocation_indices"])
                          for e in claim["frame_evidence"]], [("overlay site frame", 0x0100, [0, 1])])

    def test_site_frames_straddling_two_frames_are_refused(self):
        # Records of frames 0100 and 0104 around the claim: a relocation-free segment may
        # lie between them (probe file O1Z), so neither frame is the claim's.
        claim = self.case(records=[(0x20, 0x0100), (0x60, 0x0104)])
        self.assertIn("bracket", claim["frame_evidence_refused"][-1]["reason"])
        self.assertIn("witnessed original frame", claim["reason"])
        claim = self.case(records=[(0x20, 0x0100)])  # nothing after the claim
        self.assertIn("bracket", claim["frame_evidence_refused"][-1]["reason"])

    def test_relocated_word_entering_with_another_frame_refuses_site_frames(self):
        # Root CALL FAR 00ff:0050 lands at overlay-2+0x40 with CS 00ff, not the site frame 0100.
        claim = self.case(records=[(0x20, 0x0100), (0x60, 0x0100)], foreign=(0x00ff, 0x0050))
        self.assertIn("enters the object with frame 00ff", claim["frame_evidence_refused"][-1]["reason"])
        self.assertIn("witnessed original frame", claim["reason"])
        claim = self.case(records=[(0x20, 0x0100), (0x60, 0x0100)], foreign=(0x0100, 0x0040))
        self.assertEqual(claim["state"], "CODE_EXACT", claim)

    def test_grouped_code_segment_gets_no_site_frame(self):
        # Probe $$OVLVEC: a group member's records name its floor, its callers the group frame.
        claim = self.case(records=[(0x20, 0x0100), (0x60, 0x0100)], group=GROUP + b"\xff\x01")
        self.assertIn("group member", claim["frame_evidence_refused"][-1]["reason"])
        self.assertIn("witnessed original frame", claim["reason"])

    def test_non_monotone_site_frames_are_refused(self):
        claim = self.case(records=[(0x20, 0x0102), (0x60, 0x0100)])
        self.assertIn("not monotone", claim["frame_evidence_refused"][-1]["reason"])

    # -- section start and foreign frames ---------------------------------------------------------
    def test_section_start_brackets_with_the_link_frame(self):
        # No record before the claim; the first after it names the link segment 0100.
        claim = self.case(sharer=True, records=[(0x60, 0x0100)])
        self.assertEqual(claim["state"], "CODE_EXACT", claim)
        self.assertEqual(claim["frame_evidence"][0]["bracket_relocation_indices"], ["section-start", 0])

    def test_section_start_needs_the_link_frame_after_the_claim(self):
        # First record names 0104: the claim's floor may be anything from 0100 to 0104.
        claim = self.case(records=[(0x60, 0x0104)])
        self.assertIn("bracket", claim["frame_evidence_refused"][-1]["reason"])
        self.assertIn("witnessed original frame", claim["reason"])

    def test_word_of_a_higher_frame_names_another_segment(self):
        # Root CALL FAR 0102:0020 lands on the claim, but its floor exceeds the bracketed 0100.
        claim = self.case(records=[(0x20, 0x0100), (0x60, 0x0100)], foreign=(0x0102, 0x0020))
        self.assertEqual(claim["state"], "CODE_EXACT", claim)

    def test_word_of_a_lower_frame_needs_its_paragraph_covered(self):
        # Root CALL FAR 0101:0030 lands on the claim bracketed in 0102.
        claim = self.case(records=[(0x24, 0x0102), (0x60, 0x0102)], foreign=(0x0101, 0x0030))
        self.assertIn("enters the object with frame 0101", claim["frame_evidence_refused"][-1]["reason"])
        self.assertIn("witnessed original frame", claim["reason"])
        # A 0100 record whose field ends at 0x20 covers paragraph 0101 (0x10-0x1f): no floor 0101.
        claim = self.case(records=[(0x1e, 0x0100), (0x24, 0x0102), (0x60, 0x0102)], foreign=(0x0101, 0x0030))
        self.assertEqual(claim["state"], "CODE_EXACT", claim)
        # Ending at 0x1e, the 0100 segment may stop inside the paragraph: 0101 stays possible.
        claim = self.case(records=[(0x1c, 0x0100), (0x24, 0x0102), (0x60, 0x0102)], foreign=(0x0101, 0x0030))
        self.assertIn("enters the object with frame 0101", claim["frame_evidence_refused"][-1]["reason"])
        self.assertIn("witnessed original frame", claim["reason"])

    def test_word_of_a_frame_below_the_section_stays_refused(self):
        claim = self.case(records=[(0x20, 0x0100), (0x60, 0x0100)], foreign=(0x00ff, 0x0050))
        self.assertIn("enters the object with frame 00ff", claim["frame_evidence_refused"][-1]["reason"])
        self.assertIn("witnessed original frame", claim["reason"])

    # -- reviewed far pointers ----------------------------------------------------------------------
    def test_far_pointer_counts_only_for_an_unshared_link_address(self):
        claim = self.case(pointer_at=0x10)
        self.assertEqual(claim["state"], "CODE_EXACT", claim)
        self.assertEqual([(e["kind"], e["frame"]) for e in claim["frame_evidence"]],
                         [("relocated far pointer (unshared link address)", 0x0100)])
        claim = self.case(pointer_at=0x10, sharer=True)
        self.assertIn("far pointer link address also held by overlay-3",
                      claim["frame_evidence_refused"][0]["reason"])
        self.assertIn("witnessed original frame", claim["reason"])


if __name__ == "__main__":
    unittest.main()
