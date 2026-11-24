"""Data publics in placed segments, and externs grounded by an accepted provider's data public."""
import copy
import json
import shutil
import struct
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import common
import module_gate
from oracle import Space

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / "build/workers/tests/data_publics"
TABLE = bytes(range(0x41, 0x41 + 12))  # unique initialized content of the provider's _DATA
P = 0x20                               # its placement (DGROUP offset)


def module(publics, *, groups=({"name": "DGROUP", "segments": [2]},)):
    return SimpleNamespace(
        segment_defs=[{"name": "T_TEXT", "length": 4}, {"name": "_DATA", "length": len(TABLE)},
                      {"name": "_BSS", "length": 4}],
        groups=list(groups),
        publics=[dict({"segment": "_DATA", "group_index": 1, "frame": None}, **p) for p in publics])


class DataPublicDeclarationTests(unittest.TestCase):
    """A provider object may define publics only in a placed DGROUP segment, framed by DGROUP."""

    def check(self, publics, placements=None, **kwargs):
        return module_gate._data_publics(module(publics, **kwargs), placements or {"_DATA": P}, {"_DATA"}, "T_TEXT")

    def test_spans_run_to_the_next_public_or_segment_end(self):
        found = self.check([{"name": "HEIGHTS", "offset": 0}, {"name": "WIDTHS", "offset": 6}])
        self.assertEqual((found["HEIGHTS"]["span"], found["WIDTHS"]["span"]), (6, 6))

    def test_public_in_an_unplaced_segment_is_refused(self):
        with self.assertRaisesRegex(ValueError, "outside the CODE segment"):
            self.check([{"name": "X", "offset": 0}], placements={"_BSS": 0})

    def test_public_not_framed_by_dgroup_is_refused(self):
        # Without the DGROUP group index, F5 would frame the external by _DATA, not DGROUP.
        with self.assertRaisesRegex(ValueError, "outside the CODE segment"):
            self.check([{"name": "X", "offset": 0, "group_index": 0}])

    def test_public_outside_the_segment_is_refused(self):
        with self.assertRaisesRegex(ValueError, "outside the CODE segment"):
            self.check([{"name": "X", "offset": len(TABLE)}])


class MscDataPublicFormTests(unittest.TestCase):
    """MSC 6.00 emits non-static initialized data as a DGROUP-framed PUBDEF in _DATA; statics
    emit none; a consumer's extern is F5/T6 with the member offset as LEDATA addend."""

    def test_msc_public_and_extern_forms(self):
        from compiler import compile_c, load_lock
        from binding import fixes
        from omf import OmfReader
        if not Path(load_lock()["runner"]["path"]).is_file():
            self.skipTest("Pinned historical runner unavailable")
        flags = ["/c", "/AM", "/Oe", "/Gs", "/Zl", "/Gc"]
        provider = compile_c("int tab_a[3] = {1, 2, 3};\nstatic int hidden = 7;\nint tab_b[2] = {4, 5};\n"
                             "int far f(int i) { return tab_a[i] + tab_b[1] + hidden; }\n",
                             "msc600", flags, workdir=WORK / "msc_provider")
        consumer = compile_c("extern int tab_b[];\nint far g(void) { return tab_b[1]; }\n",
                             "msc600", flags, workdir=WORK / "msc_consumer")
        self.assertTrue(provider.ok and consumer.ok)
        m = OmfReader().read(provider.obj_bytes)
        members = {m.segment_defs[i - 1]["name"] for g in m.groups if g["name"] == "DGROUP" for i in g["segments"]}
        found = module_gate._data_publics(m, {"_DATA": 0}, members, "UNIT_TEXT")
        # The static between them extends TAB_A's span: an upper bound on the declared object.
        self.assertEqual({n: (d["offset"], d["span"]) for n, d in found.items()}, {"TAB_A": (0, 8), "TAB_B": (8, 4)})
        c = OmfReader().read(consumer.obj_bytes)
        (fix,) = fixes(c)
        self.assertEqual((fix["target"]["name"], fix["frame"]["kind"], fix["displacement"]),
                         ("TAB_B", "target_frame", None))
        self.assertEqual(struct.unpack_from("<H", c.segments["UNIT_TEXT"], fix["segment_offset"])[0], 2)


class ProviderPublicTests(unittest.TestCase):
    """S = P + o from an accepted provider whose placed content occurs once in DGROUP."""

    def setUp(self):
        shutil.rmtree(WORK, ignore_errors=True)
        WORK.mkdir(parents=True)
        # DGROUP at paragraph 0x10 of root (image 0x100); its paragraph is stored in a relocated word.
        image = bytearray(0x100 + 0x80)
        image[0x100 + P:0x100 + P + len(TABLE)] = TABLE
        image += struct.pack("<H", 0x10)
        self.space = Space("root", 0, 0, bytes(image), ({"index": 0, "image_offset": len(image) - 2},))
        self.oracle = SimpleNamespace(spaces={"root": self.space})
        self.provider_binding = {
            "frame": {"segment": 0x10},
            "dgroup_segment": {"segment": 0x10, "witnesses": [
                {"space": "root", "image_offset": len(image) - 2, "relocation_index": 0}]},
            "placements": {"_DATA": P},
            "declarations": {"segments": [{"name": "_DATA", "length": len(TABLE)}]}}
        self.recipes = [({"kind": "MATCHING_C"}, self.recipe("group:prov", self.provider_binding,
                                                             [{"name": "TABLE", "segment": "_DATA", "offset": 4,
                                                               "span": 8, "address": P + 4, "unique": True}]))]

    def recipe(self, group_id, binding, data_publics, claims=("PROVIDER_FUNC",)):
        path = WORK / (group_id.split(":")[1] + ".json")
        raw = json.dumps(binding).encode()
        path.write_bytes(raw)
        return {"kind": "same_source_group", "group_id": group_id, "name": group_id.split(":")[1], "space": "root",
                "binding": path.relative_to(ROOT).as_posix(), "binding_sha256": common.sha(raw),
                "claims": [{"public": c} for c in claims], "drafts": [], "data_publics": data_publics}

    def ground(self, symbol=None, name="TABLE", group_id="group:consumer", own=()):
        symbol = symbol or {"kind": "dgroup-public", "group": "DGROUP", "provider": "group:prov", "offset": P + 4}
        with patch.object(module_gate, "_accepted_recipes", return_value=self.recipes):
            return module_gate._provider_public(name, symbol, self.oracle, group_id, set(own))

    def test_unique_placed_content_grounds_the_public(self):
        grounded = self.ground()
        self.assertEqual((grounded["offset"], grounded["span"], grounded["range"]), (P + 4, 8, (P, P + len(TABLE))))

    def test_repeated_content_or_bss_never_provides(self):
        self.recipes[0][1]["data_publics"][0]["unique"] = False
        with self.assertRaisesRegex(ValueError, "unique initialized content"):
            self.ground()

    def test_original_window_is_rechecked_for_uniqueness(self):
        # The recipe says unique, but the original DGROUP holds the same bytes twice.
        data = bytearray(self.space.data)
        data[0x100 + 0x60:0x100 + 0x60 + len(TABLE)] = TABLE
        self.oracle.spaces["root"] = Space("root", 0, 0, bytes(data), self.space.relocations)
        with self.assertRaisesRegex(ValueError, "unique original DGROUP content"):
            self.ground()

    def test_name_defined_twice_among_accepted_components_is_refused(self):
        self.recipes.append(({"kind": "MATCHING_C"}, {"public": "TABLE"}))
        with self.assertRaisesRegex(ValueError, "exactly once"):
            self.ground()

    def test_name_must_be_a_data_public_of_that_provider(self):
        with self.assertRaisesRegex(ValueError, "exactly once"):
            self.ground(name="OTHER")
        self.recipes[0][1]["claims"].append({"public": "OTHER"})
        with self.assertRaisesRegex(ValueError, "not a data public"):
            self.ground(name="OTHER")

    def test_unaccepted_provider_and_self_reference_are_refused(self):
        with self.assertRaisesRegex(ValueError, "not one accepted"):
            self.ground({"kind": "dgroup-public", "group": "DGROUP", "provider": "group:none", "offset": P + 4})
        with self.assertRaisesRegex(ValueError, "another accepted group"):
            self.ground(group_id="group:prov")
        with self.assertRaisesRegex(ValueError, "another accepted group"):
            self.ground(own={"TABLE"})

    def test_stated_address_must_equal_placement_plus_offset(self):
        with self.assertRaisesRegex(ValueError, "disagrees"):
            self.ground({"kind": "dgroup-public", "group": "DGROUP", "provider": "group:prov", "offset": P + 6})

    def test_changed_provider_binding_is_refused(self):
        self.recipes[0][1]["binding_sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "Provider binding changed"):
            self.ground()

    def test_another_groups_placement_over_the_provider_segment_is_refused(self):
        other = copy.deepcopy(self.provider_binding)
        other["placements"] = {"_DATA": P + len(TABLE) - 1}
        self.recipes.append(({"kind": "MATCHING_C"}, self.recipe("group:other", other, [], claims=("OTHER_FUNC",))))
        with self.assertRaisesRegex(ValueError, "overlaps placed _DATA of other"):
            self.ground()

    def test_the_republished_consumers_own_old_placement_is_replaced_not_an_overlap(self):
        # Publishing group:other again replaces its accepted recipe, so that recipe's old placement
        # over the provider is not a conflict; a third group's placement still is.
        other = copy.deepcopy(self.provider_binding)
        other["placements"] = {"_DATA": P + len(TABLE) - 1}
        self.recipes.append(({"kind": "MATCHING_C"}, self.recipe("group:other", other, [], claims=("OTHER_FUNC",))))
        self.assertEqual(self.ground(group_id="group:other")["offset"], P + 4)
        third = copy.deepcopy(other)
        self.recipes.append(({"kind": "MATCHING_C"}, self.recipe("group:third", third, [], claims=("THIRD_FUNC",))))
        with self.assertRaisesRegex(ValueError, "overlaps placed _DATA of third"):
            self.ground(group_id="group:other")
        # ...unless the same publication batch republishes group:third too.
        with patch.object(module_gate, "REPLACING", {"group:third"}):
            self.assertEqual(self.ground(group_id="group:other")["offset"], P + 4)


class ProviderFieldTests(unittest.TestCase):
    """The consumer's addend stays inside the provider public's span; values never alias."""

    def equations(self, code, expected):
        from binding import OPERAND_MODE, data_equations, instructions
        fix = {"segment": "T", "segment_offset": 2, "loc_type": 1, "field_width": 2, "self_relative": False,
               "target": {"kind": "external", "method": 6, "name": "TABLE"}, "displacement": None}
        fix["frame"] = {"kind": "target_frame", "target": fix["target"]}
        symbol = {"kind": "dgroup-public", "offset": P + 4, "span": 8, "provider": "group:prov"}
        return data_equations([fix], {"TABLE": symbol}, "T", bytes.fromhex(code), bytes.fromhex(expected),
                              instructions(bytes.fromhex(code)), OPERAND_MODE, set())

    def test_member_addend_inside_the_span_is_proven(self):
        found, unequal = self.equations("8b060600cb", "8b062a00cb")  # mov ax,[TABLE+6] -> DS:002a
        self.assertEqual((found[0]["linked_value"], found[0]["provider"], unequal), (P + 10, "group:prov", []))

    def test_word_access_crossing_the_span_end_is_refused(self):
        with self.assertRaisesRegex(ValueError, "span"):
            self.equations("8b060700cb", "8b062b00cb")  # [TABLE+7] reads bytes 7..8 of an 8-byte span

    def test_addend_outside_the_span_is_refused(self):
        with self.assertRaisesRegex(ValueError, "span"):
            self.equations("8a060800cb", "8a062c00cb")
        with self.assertRaisesRegex(ValueError, "span"):
            self.equations("8a06feffcb", "8a062200cb")  # a negative addend wraps to 0xfffe

    def test_provider_field_aliasing_a_v4_symbol_is_refused(self):
        symbols = {"TABLE": {"kind": "dgroup-public", "offset": P + 4, "span": 8},
                   "ALIAS": {"kind": "dgroup-offset", "offset": P + 6}}
        states = {"F": {"v4": [{"key": ("TABLE", 2)}]}}
        with self.assertRaisesRegex(ValueError, "aliases v4 symbol ALIAS"):
            module_gate._check_provider_aliases(symbols, states)
        states["F"]["v4"] = [{"key": ("TABLE", 0)}]
        module_gate._check_provider_aliases(symbols, states)

    def test_provider_segment_over_own_placement_is_refused_and_span_aliases_are_debt(self):
        own = SimpleNamespace(segment_defs=[{"name": "_DATA", "length": 2}])
        symbols = {"TABLE": {"kind": "dgroup-public", "offset": P + 4, "span": 8, "range": (P, P + 12)},
                   "ALIAS": {"kind": "dgroup-offset", "offset": P + 6}}
        self.assertEqual(module_gate._check_unplaced(own, {"_DATA": 0x80}, symbols), ["ALIAS@0x0026 in TABLE"])
        with self.assertRaisesRegex(ValueError, "overlaps placed segment _DATA"):
            module_gate._check_unplaced(own, {"_DATA": P + 11}, symbols)


if __name__ == "__main__":
    unittest.main()


class DataOnlyGroupTests(unittest.TestCase):
    """A data-only C object is placed by unique initialized content and owns no code."""
    FLAGS = ["/c", "/AM", "/Oe", "/Gs", "/Zl", "/Gc"]

    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        cls.oracle = Oracle.load()
        binding = common.read_json(ROOT / "evidence/bindings/custcvca/custcvca.json")
        cls.base = {k: binding[k] for k in ("schema", "mode", "target_sha256", "frame", "dgroup_segment")}

    def judge(self, source, placement, name):
        from binding import declarations, fixes
        from compiler import compile_c
        from omf import OmfReader
        r = compile_c(source, "msc600", flags=self.FLAGS, workdir=WORK / name)
        self.assertTrue(r.ok, r.log)
        m = OmfReader().read(r.obj_bytes, "probe")
        binding = dict(self.base, target_id="group:t/" + name, placements={"_DATA": placement},
                       fixups=fixes(m), declarations=declarations(m), symbols={})
        return module_gate.check_data_group(r.obj_bytes, self.oracle, binding=binding, group_id="group:t/" + name)

    def test_unique_string_is_placed_and_published(self):
        # "Pos=%d" occurs once in the original DGROUP, at DS:109e.
        report = self.judge('char pos_format[] = "Pos=%d";\n', 0x109E, "pos")
        self.assertTrue(report["placements"]["_DATA"]["unique"])
        self.assertEqual([(d["name"], d["address"]) for d in report["data_publics"]], [("POS_FORMAT", 0x109E)])
        with self.assertRaisesRegex(ValueError, "differs"):
            self.judge('char pos_format[] = "Pos=%d";\n', 0x10A0, "pos_wrong")

    def test_code_is_refused(self):
        with self.assertRaisesRegex(ValueError, "emits no code"):
            self.judge('char pos_format[] = "Pos=%d";\nint f(void) { return 1; }\n', 0x109E, "pos_code")


class AggregateExtentTests(unittest.TestCase):
    """An original block copy anchored at S proves [S, S+L) is one copied unit (MOVE's 13-byte mob
    records: `lea si,[bx+293e]; ...; mov cx,6; rep movsw; movsb`)."""

    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        cls.root = Oracle.load().spaces["root"]

    def test_source_and_destination_copies_give_the_record_length(self):
        from binding import block_copy_extent
        self.assertEqual(block_copy_extent(self.root.data[0xCF9F:0xCFAB], 0xCF9F, 0x293E), 13)
        self.assertEqual(block_copy_extent(self.root.data[0xCFBF:0xCFD0], 0xCFBF, 0x293E), 13)

    def test_other_base_or_control_transfer_proves_nothing(self):
        from binding import block_copy_extent
        self.assertIsNone(block_copy_extent(self.root.data[0xCF9F:0xCFAB], 0xCF9F, 0x293F))
        # lea si,[bx+2aa4]; jmp short $+2; mov cx,6; rep movsw: a jump inside the window
        with self.assertRaisesRegex(ValueError, "straight-line"):
            block_copy_extent(bytes.fromhex("8db7a42aeb00b90600f3a5"), 0, 0x2AA4)
        # the count register rewritten after `mov cx,n`: no proven length
        self.assertIsNone(block_copy_extent(bytes.fromhex("8db7a42ab9060041f3a5"), 0, 0x2AA4))


class ImmediateWitnessTests(unittest.TestCase):
    """An immediate is an address witness only when original code uses it as an address."""

    def use(self, code):
        from binding import immediate_is_address
        space = Space("root", 0, 0, bytes.fromhex(code) + bytes([0xCB]) * 32, ())
        return immediate_is_address(SimpleNamespace(spaces={"root": space}), "root", 0)

    def test_dereference_string_and_ds_argument_count(self):
        self.assertTrue(self.use("b8005c8bd88a07"))            # mov ax,S; mov bx,ax; mov al,[bx]
        self.assertTrue(self.use("bb005c03d88b4702"))          # mov bx,S; add bx,ax; mov ax,[bx+2]
        self.assertTrue(self.use("be025cb91000f3a5"))          # mov si,S; mov cx,n; rep movsw
        self.assertTrue(self.use("b8005c1e50"))                # mov ax,S; push ds; push ax
        self.assertTrue(self.use("ba8a2ceb018b8bda8a07"))      # through a direct jump

    def test_numbers_do_not(self):
        self.assertFalse(self.use("b85a0050"))                 # mov ax,90; push ax
        self.assertFalse(self.use("c706b0000000"))             # mov word [00b0],0
        self.assertFalse(self.use("b906008bc1f7e1"))           # mov cx,6; mov ax,cx; mul cx
        self.assertFalse(self.use("b8005c33c08bd88a07"))       # overwritten before use
        self.assertFalse(self.use("bb005c268a07"))             # ES: access is not a near DGROUP access

    def test_original_number_and_address(self):
        from oracle import Oracle
        from binding import immediate_is_address
        oracle = Oracle.load()
        self.assertFalse(immediate_is_address(oracle, "root", 0x7684))   # mov ax,5a (a number, DIALOG)
        self.assertTrue(immediate_is_address(oracle, "root", 0x3109))   # mov ax,5cc6 -> di (GetFrameInfo copy)


class SwitchReachTests(unittest.TestCase):
    """Recursive descent follows a bounded MSC switch table and no other indirect jump."""

    def test_original_switch_cases_are_reached(self):
        from oracle import Oracle
        from binding import reachable_from_entry
        reached = reachable_from_entry(Oracle.load(), "root:02dc:02ac")   # GetFrameInfo: 13 cases
        self.assertTrue({0x30B2, 0x30D2, 0x30FE, 0x3109} <= reached)
        self.assertFalse(any(0x3098 <= at < 0x30B2 for at in reached))    # the table itself is data

    def test_only_an_unsigned_bound_directly_before_the_jump_counts(self):
        from binding import _switch_targets
        good = bytes.fromhex("3d0c007603e99f0003c0932effa7c202")
        self.assertEqual(_switch_targets(good, 11, {0}), (0x2C2, 13))
        self.assertEqual(_switch_targets(good, 11, set()), ())                         # bound check not reached
        self.assertEqual(_switch_targets(bytes.fromhex("3d0c007e03e99f0003c0932effa7c202"), 11, {0}), ())  # signed
        self.assertEqual(_switch_targets(bytes.fromhex("3d0c007603e99f009003c0932effa7c202"), 12, {0}), ())
        self.assertEqual(_switch_targets(bytes.fromhex("3d0c007310d1e0932effa71000"), 8, {0}), (0x10, 12))
        self.assertEqual(_switch_targets(bytes.fromhex("3d1e007603e94e01932effa7d201"), 9, {0}), (0x1D2, 16))
        self.assertEqual(_switch_targets(bytes.fromhex("3d1f007603e94e01932effa7d201"), 9, {0}), ())       # odd


class ExtendedCopyTests(unittest.TestCase):
    """Unrolled copies, register bases and DS/ES tracking (2026-11-23)."""

    def extent(self, code, base=0x2AA4):
        from binding import block_copy_extent
        return block_copy_extent(bytes.fromhex(code), 0, base)

    def test_original_frame_record_copy_through_ax(self):
        # root+3109: mov ax,5cc6; ...; push ds; mov di,ax; push ds; pop es; mov ds,cx; movsw x3; movsb
        from oracle import Oracle
        from binding import block_copy_extent
        root = Oracle.load().spaces["root"]
        self.assertEqual(block_copy_extent(root.data[0x3109:0x312E], 0x3109, 0x5CC6), 7)

    def test_unrolled_run_ends_at_the_first_other_instruction(self):
        self.assertEqual(self.extent("bea42a1e07a5a5a4"), 5)
        self.assertEqual(self.extent("bea42a1e07a5a590a5"), 4)      # nop splits the run
        self.assertIsNone(self.extent("bea42a46a5a5"))              # inc si: no longer the base

    def test_a_changed_ds_or_es_proves_nothing(self):
        self.assertIsNone(self.extent("bea42a8ed8b90600f3a5"))      # mov ds,ax before a DS:SI copy
        self.assertIsNone(self.extent("bea42a1fb90600f3a5"))        # pop ds
        self.assertIsNone(self.extent("bfa42ab90600f3a5"))          # ES never loaded from DS
        self.assertEqual(self.extent("bfa42a1e07b90600f3a5"), 12)
        self.assertIsNone(self.extent("bfa42a1e078ec0b90600f3a5"))  # ES replaced afterwards
        self.assertIsNone(self.extent("bea42afdb90600f3a5"))        # std


class PointerExtentTests(unittest.TestCase):
    """One far-pointer access proves a 4-byte object."""

    def test_original_vector_uses(self):
        from oracle import Oracle
        from binding import pointer_extent
        root = Oracle.load().spaces["root"]
        self.assertEqual(pointer_extent(root.data[0x15EB:0x15F0], 0x1D68), 4)     # lds dx,ss:[1d68]
        self.assertEqual(pointer_extent(root.data[0x1630:0x1634], 0x1D6C), 4)     # call far [1d6c]
        self.assertIsNone(pointer_extent(root.data[0x1630:0x1634], 0x1D6E))

    def test_other_accesses_do_not(self):
        from binding import pointer_extent
        for code in ("a1da1e", "ff36da1e", "26c41eda1e", "c45eda", "c41f"):   # word, push, ES, [bp], [bx]
            self.assertIsNone(pointer_extent(bytes.fromhex(code), 0x1EDA), code)

    def test_an_index_witness_must_lie_outside_the_claims(self):
        from oracle import Oracle
        from binding import ground
        oracle = Oracle.load()
        real = common.read_json(ROOT / "evidence/bindings/custcvca/custcvca.json")
        symbol = {"kind": "dgroup-offset", "group": "DGROUP", "offset": 0x1D68, "witnesses": [],
                  "members": [{"offset": 2, "witnesses": []}],
                  "extent": {"kind": "pointer", "length": 4, "witness": {
                      "space": "root", "entry": "root:0000:15e2", "image_offset": 0x15EB, "bytes_hex": "36c516681d"}}}
        binding = {**real, "target_id": "group:t/t", "symbols": {"V": symbol}}
        target = {"id": "group:t/t", "space": "root", "segment": 0, "offset": 0x157E, "size": 0x12F}
        ground(binding, target, oracle, claimed=[(0x15E2, 0x160A)])     # a pointer access may be claimed
        symbol["extent"]["kind"] = "index"
        with self.assertRaisesRegex(ValueError, "overlaps candidate"):
            ground(binding, target, oracle, claimed=[(0x15E2, 0x160A)])


class IndexExtentTests(unittest.TestCase):
    """A bounds-checked byte index proves an array's base and length."""

    def test_keyboard_handler_indexes_59h_bytes(self):
        from oracle import Oracle
        from binding import index_extent
        root = Oracle.load().spaces["root"]
        self.assertEqual(index_extent(root.data[0x160B:0x162D], 0x1D0D), 0x59)
        self.assertIsNone(index_extent(root.data[0x160B:0x162D], 0x1D0E))

    def test_every_part_of_the_bound_is_required(self):
        from binding import index_extent
        good = "8ad82aff80e37f80fb59730681c37b1e8807"
        self.assertEqual(index_extent(bytes.fromhex(good), 0x1E7B), 0x59)
        for code in ("8ad880e37f80fb59730681c37b1e8807",            # BH not cleared
                     "8ad82aff8bd880fb59730681c37b1e8807",          # BX replaced afterwards
                     "8ad82aff80fb597f0681c37b1e8807",              # signed bound
                     "8ad82aff80fb59730681c37b1e268807"):           # ES access
            self.assertIsNone(index_extent(bytes.fromhex(code), 0x1E7B), code)


class RecordExtentTests(unittest.TestCase):
    """Stepping through [r+S] with an index of d times one unknown proves records of d bytes at S."""

    def stride(self, code, base, **kw):
        from binding import record_stride
        return record_stride(bytes.fromhex(code), base, **kw)

    def test_original_tile_and_object_records(self):
        from oracle import Oracle
        from binding import record_stride
        root = Oracle.load().spaces["root"]
        self.assertEqual(record_stride(root.data[0x7D34:0x7D44], 0x61AA), 5)      # shifts and an add
        self.assertEqual(record_stride(root.data[0xD808:0xD81A], 0x5D44), 23)     # imul by a constant register
        self.assertIsNone(record_stride(root.data[0x7D34:0x7D44], 0x61AB))

    def test_the_scale_must_be_visible_and_the_index_one_unknown(self):
        self.assertEqual(self.stride("8bd8d1e3d1e303d88a87aa61", 0x61AA), 5)
        self.assertEqual(self.stride("b016f62ede5c8bd88a87aa61", 0x61AA), 22)     # byte mul by a constant
        for code in ("8bd88a87aa61",                          # no scale
                     "8bd8d1e3d1e303d98a87aa61",              # two unknowns added
                     "8bd8d1e3d1e303d88a1e0e018a87aa61",      # BL replaced afterwards
                     "8bd8d1e3d1e303d88d87aa61",              # LEA is no access
                     "8bd8d1e3d1e303d8268a87aa61",            # ES
                     "8bd8d1e3d1e303d88a80aa61",              # two index registers
                     "bb0500d1e38a87aa61"):                   # a constant index
            self.assertIsNone(self.stride(code, 0x61AA), code)
        with self.assertRaisesRegex(Exception, "straight-line"):
            self.stride("8bd8d1e3740003d88a87aa61", 0x61AA)

    def test_es_records_need_the_es_access(self):
        self.assertEqual(self.stride("b91400f7e98bd8268987f811", 0x11F8, es=True), 20)
        self.assertIsNone(self.stride("b91400f7e98bd88987f811", 0x11F8, es=True))


class FillExtentTests(unittest.TestCase):
    """REP STOS of a constant count at a constant DGROUP address fills one unit."""

    def test_original_redraw_table_clear(self):
        from oracle import Oracle
        from binding import fill_extent
        root = Oracle.load().spaces["root"]
        self.assertEqual(fill_extent(root.data[0xC9A1:0xC9B9], 0x6248), 300)
        self.assertIsNone(fill_extent(root.data[0xC9A1:0xC9B9], 0x64FA))          # an earlier fill of the window

    def test_every_part_is_required(self):
        from binding import fill_extent
        self.assertEqual(fill_extent(bytes.fromhex("1e07bf4862b91000f3ab"), 0x6248), 32)
        for code in ("bf4862b91000f3ab",                      # ES unknown
                     "1e07bf4862f3ab",                        # no count
                     "1e07bf4862b91000b108f3ab",              # count changed
                     "1e07bf486247b91000f3ab",                # DI changed
                     "1e07fdbf4862b91000f3ab",                # backwards
                     "1607bf4862b91000f3ab"):                 # ES = SS
            self.assertIsNone(fill_extent(bytes.fromhex(code), 0x6248), code)


class FarReadExtentTests(unittest.TestCase):
    """A callee that reads L bytes in sequence through a DS:S argument proves [S, S+L) is one unit."""

    def setUp(self):
        from oracle import Oracle
        self.root = Oracle.load().spaces["root"]

    def test_inset_rect_reads_the_screen_rectangle(self):
        from binding import call_arguments, far_read_extent
        arguments = call_arguments(self.root.data[0x5832:0x5841])
        self.assertEqual(arguments, {6: 9, 8: 9, 10: 0x1F2A, 12: "ds"})
        body = self.root.data[0x1629C:0x162BD]
        self.assertEqual(far_read_extent(body, 0x1F2A, arguments), 8)
        self.assertIsNone(far_read_extent(body, 0x1F2C, arguments))
        self.assertIsNone(far_read_extent(body, 0x1F2A, {10: 0x1F2A}))            # not the caller's DS
        self.assertIsNone(far_read_extent(body, 9, {10: 0x1F2A, 12: "ds", 14: 9, 16: "ds"}))

    def test_nothing_else_may_move_the_pointer(self):
        from binding import far_read_extent
        arguments = {6: 0x1F2A, 8: "ds"}
        self.assertEqual(far_read_extent(bytes.fromhex("558bec561ec57606adac"), 0x1F2A, arguments), 3)
        for code in ("558bec561ec576064ead", "558bec561ec576061607ad1fad", "558bec561efdc57606ad",
                     "558bec895e06c57606ad"):
            self.assertIsNone(far_read_extent(bytes.fromhex(code), 0x1F2A, arguments), code)


class BaseExtentTests(unittest.TestCase):
    """`mov r,S` and an access [r+k] on the fall-through path prove [S, S+k+w) is one object."""

    def test_near_heap_descriptor_of_nfree(self):
        from oracle import Oracle
        from binding import base_extent
        root = Oracle.load().spaces["root"]
        self.assertEqual(base_extent(root.data[0x1FF11:0x1FF21], 0x20EC), 10)     # cmp [si+8],bx
        self.assertEqual(base_extent(root.data[0x1FF11:0x1FF17], 0x20EC), 8)
        self.assertIsNone(base_extent(root.data[0x1FF11:0x1FF21], 0x20EE))

    def test_every_part_is_required(self):
        from binding import base_extent
        self.assertEqual(base_extent(bytes.fromhex("beec2089" "5c08"), 0x20EC), 10)
        for code in ("beec204689" "5c08",                     # SI changed
                     "beec20268c" "5c08",                     # ES access
                     "beec208d" "5c08",                       # LEA
                     "beec208958" "08",                       # an index register
                     "beec20895cfe",                          # below S
                     "beec201f895c08"):                       # DS changed
            self.assertIsNone(base_extent(bytes.fromhex(code), 0x20EC), code)
        with self.assertRaisesRegex(Exception, "fall through"):
            base_extent(bytes.fromhex("beec20e80000895c08"), 0x20EC)


class ArchivePublicTests(unittest.TestCase):
    """A pinned library member's unique data can be the provider of an extern."""
    ARCHIVE = {"library_path": "C:/tools/msc-6.00/LIB/mlibcr.lib",
               "library_sha256": "03a85ba8ac1752b64c82eff829bd2de34f776630e1aa90874155b55f187d64f0",
               "archive_offset": 32704, "segment": "_DATA", "placement": 0x234A,
               "dgroup_segment": {"segment": 13058, "witnesses": [
                   {"space": "root", "image_offset": 5647, "relocation_index": 54}]},
               "member_sha256": "9b7dd91fa60b803dfa5a4f7b3b03e45022c44e42291fc9c813ac55481839a1a2"}

    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        cls.oracle = Oracle.load()
        library = Path(cls.ARCHIVE["library_path"]).read_bytes()
        start = cls.ARCHIVE["archive_offset"]
        cls.extent = next(n for n in range(16, 4096)
                          if common.sha(library[start:start + n]) == cls.ARCHIVE["member_sha256"])

    def ground(self, name="__ctype", offset=0x234A, **archive):
        symbol = {"kind": "dgroup-public", "group": "DGROUP", "provider": "runtime", "offset": offset,
                  "archive": {**self.ARCHIVE, "member_extent": self.extent, **archive}}
        return module_gate._provider_public(name, symbol, self.oracle, "group:x/x", set())

    def test_ctype_is_grounded_by_the_pinned_member(self):
        self.assertEqual((self.ground()["offset"], self.ground()["span"]), (0x234A, 257))

    def test_other_placements_names_and_members_are_refused(self):
        with self.assertRaisesRegex(ValueError, "unique original DGROUP content"):
            self.ground(offset=0x234C, placement=0x234C)
        with self.assertRaisesRegex(ValueError, "single data public"):
            self.ground(name="__other")
        with self.assertRaisesRegex(ValueError, "identity changed"):
            self.ground(archive_offset=32705)
        with self.assertRaisesRegex(ValueError, "accepted runtime recipe pins"):
            self.ground(library_sha256="0" * 64)


class CallArgumentExtentTests(unittest.TestCase):
    """A callee's copy through a near-pointer argument proves the passed object's extent."""
    INIT_PORT = (0x16252, 0x16266)

    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        cls.root = Oracle.load().spaces["root"]

    def test_init_port_copies_44_bytes_into_screen_port(self):
        from binding import block_copy_extent, call_arguments
        lo, hi = self.INIT_PORT
        at = self.root.data.find(bytes.fromhex("b80a5d50b82a1f1e509ac24f2911"))
        self.assertEqual(at, 0x13EE)
        arguments = call_arguments(self.root.data[at:at + 14])
        self.assertEqual(arguments, {6: 0x1F2A, 8: "ds", 10: 0x5D0A})
        self.assertEqual(block_copy_extent(self.root.data[lo:hi], lo, 0x5D0A, arguments), 44)
        self.assertIsNone(block_copy_extent(self.root.data[lo:hi], lo, 0x1F2A, arguments))   # not the copied argument
        self.assertIsNone(block_copy_extent(self.root.data[lo:hi], lo, 0x5D0A, {6: 0x5D0A}))  # another slot

    def test_call_window_must_be_one_straight_line_call(self):
        from binding import call_arguments
        with self.assertRaisesRegex(ValueError, "straight-line"):
            call_arguments(bytes.fromhex("b8625e50eb00") + bytes.fromhex("9ae04f0111"))
        with self.assertRaisesRegex(ValueError, "stack pointer"):
            call_arguments(bytes.fromhex("b8625e5083c4029ae04f0111"))
        with self.assertRaisesRegex(ValueError, "stack pointer"):
            call_arguments(bytes.fromhex("b8625e505b9ae04f0111"))                              # pop bx
        self.assertEqual(call_arguments(bytes.fromhex("b8625e33c0509ae04f0111")), {})         # overwritten


class ProviderRelocationTests(unittest.TestCase):
    """A provider segment may hold its own relocated far-pointer words, and no others."""

    def test_load_level_frame_procs_grounds_by_name(self):
        from oracle import Oracle
        recipe = common.read_json(ROOT / "recipes/load/load.json")
        public = next(d for d in recipe["data_publics"] if d["name"] == "LEVEL_FRAME_PROCS")
        symbol = {"kind": "dgroup-public", "group": "DGROUP", "provider": recipe["group_id"], "offset": 0x0654}
        grounded = module_gate._provider_public(public["name"], symbol, Oracle.load(), "group:other", set())
        self.assertEqual(grounded["offset"], 0x0654)

    def test_an_unexplained_relocated_word_is_refused(self):
        from oracle import Oracle
        recipe = common.read_json(ROOT / "recipes/load/load.json")
        public = next(d for d in recipe["data_publics"] if d["name"] == "LEVEL_FRAME_PROCS")
        symbol = {"kind": "dgroup-public", "group": "DGROUP", "provider": recipe["group_id"], "offset": 0x0654}
        real = json.loads
        def without_fixups(raw, *a, **k):
            data = real(raw, *a, **k)
            if isinstance(data, dict) and data.get("target_id") == recipe["group_id"]:
                data["fixups"] = []
            return data
        with patch("json.loads", without_fixups), self.assertRaisesRegex(ValueError, "not fixed by unique"):
            module_gate._provider_public(public["name"], symbol, Oracle.load(), "group:other", set())


class BssLayoutTests(unittest.TestCase):
    """bss-layout-v1: anchors come from the original C startup; impossible layouts are flagged."""

    def test_anchors_are_the_original_startup_operands(self):
        from oracle import Oracle
        root = Oracle.load().spaces["root"]
        word = lambda at: struct.unpack_from("<H", root.data, at)[0]
        self.assertEqual((root.data[0x1FFE7], word(0x1FFE8)), (0xBF, module_gate.BSS_START))   # mov di,_edata
        self.assertEqual((root.data[0x1FFEA], word(0x1FFEB)), (0xB9, module_gate.END))         # mov cx,_end
        self.assertEqual((root.data[0x20147], word(0x20148)), (0xBE, module_gate.BSS_END))     # mov si,xontab
        self.assertEqual((root.data[0x2014A], word(0x2014B)), (0xBF, module_gate.BSS_END))     # mov di,xontab

    def test_violations(self):
        recipe = {"language": "c", "group_id": "group:t/t", "name": "t/t"}
        groups = {"r.json": {"placements": {"_BSS": {"placement": 0x3E78, "length": 4}}}}
        self.assertEqual(module_gate.bss_layout_violations(groups, [({"recipe": "r.json"}, recipe)]),
                         {"group:t/t:_BSS@3e78"})
        ok = {"r.json": {"placements": {"_BSS": {"placement": 0x293E, "length": 4}}}}
        self.assertEqual(module_gate.bss_layout_violations(ok, [({"recipe": "r.json"}, recipe)]), set())
        asm = dict(recipe, language="asm")
        self.assertEqual(module_gate.bss_layout_violations(groups, [({"recipe": "r.json"}, asm)]), set())


class NameConsistencyTests(unittest.TestCase):
    """One definition per public name; one address per binding name (whole-link refutation)."""

    def test_duplicates_and_multi_address_names(self):
        import tempfile
        (ROOT / "build/workers/tests").mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=ROOT / "build/workers/tests") as tmp:
            a, b = Path(tmp) / "a.json", Path(tmp) / "b.json"
            a.write_text(json.dumps({"symbols": {"X": {"kind": "dgroup-offset", "offset": 0x100}}}))
            b.write_text(json.dumps({"symbols": {"X": {"kind": "dgroup-offset", "offset": 0x102}}}))
            r1 = {"kind": "same_source_group", "claims": [{"public": "F"}], "drafts": [], "binding": str(a)}
            r2 = {"kind": "same_source_group", "claims": [{"public": "F"}], "drafts": [], "binding": str(b)}
            found = module_gate.name_consistency_violations([({}, r1), ({}, r2)])
            self.assertEqual(found, {"duplicate-public:F", "multi-address:X"})
            b.write_text(json.dumps({"symbols": {"X": {"kind": "dgroup-offset", "offset": 0x100}}}))
            r2["claims"] = [{"public": "G"}]
            self.assertEqual(module_gate.name_consistency_violations([({}, r1), ({}, r2)]), set())

    def test_code_entry_aliases(self):
        import tempfile
        (ROOT / "build/workers/tests").mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=ROOT / "build/workers/tests") as tmp:
            a = Path(tmp) / "a.json"
            entry = {"kind": "far-code-entry", "entry_target_id": "root:0001:0000", "segment": 1, "offset": 0}
            a.write_text(json.dumps({"symbols": {"OLD": entry, "OTHER": entry}}))
            definer = {"kind": "same_source_group", "group_id": "group:d/d", "drafts": [],
                       "claims": [{"public": "NEW", "target_id": "root:0001:0000"}]}
            user = {"kind": "same_source_group", "group_id": "group:u/u", "claims": [], "drafts": [],
                    "binding": str(a)}
            self.assertEqual(module_gate.name_consistency_violations([({}, definer), ({}, user)]),
                             {"alias:group:u/u:OLD", "alias:group:u/u:OTHER", "same-entry:group:u/u:root:0001:0000"})
            a.write_text(json.dumps({"symbols": {"NEW": entry}}))
            self.assertEqual(module_gate.name_consistency_violations([({}, definer), ({}, user)]), set())

    def test_vector_and_data_aliases(self):
        import tempfile
        (ROOT / "build/workers/tests").mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=ROOT / "build/workers/tests") as tmp:
            a = Path(tmp) / "a.json"
            definer = {"kind": "same_source_group", "group_id": "group:d/d", "drafts": [],
                       "claims": [{"public": "FRAME", "target_id": "overlay-14:2fcd:0000"}],
                       "data_publics": [{"name": "TABLE", "address": 0x100}]}
            user = {"kind": "same_source_group", "group_id": "group:u/u", "claims": [], "drafts": [], "binding": str(a)}
            vectors = {(0x220E, 0xDE9): "overlay-14:2fcd:0000", (0x220E, 0xDA3): "overlay-13:2fcd:0000"}

            def found(**symbols):
                a.write_text(json.dumps({"symbols": symbols}))
                return module_gate.name_consistency_violations([({}, definer), ({}, user)], vectors)
            vector = {"kind": "rtlink-vector", "space": "root", "segment": 0x220E, "offset": 0xDE9}
            data = {"kind": "dgroup-offset", "offset": 0x100}
            self.assertEqual(found(Frame=vector, Table=data), set())
            self.assertEqual(found(OTHER=vector, LIST=data),
                             {"vector-alias:group:u/u:OTHER", "data-alias:group:u/u:LIST"})
            self.assertEqual(found(OTHER=dict(vector, offset=0xDA3), LIST=dict(data, offset=0x102)), set())

    def test_library_public_aliases_share_an_entry(self):
        import tempfile
        (ROOT / "build/workers/tests").mkdir(parents=True, exist_ok=True)
        runtime = json.loads((ROOT / "recipes/runtime/crt_aflmul_medium.json").read_text())
        at = runtime["segment"] * 16 + runtime["offset"]
        with tempfile.TemporaryDirectory(dir=ROOT / "build/workers/tests") as tmp:
            a = Path(tmp) / "a.json"
            user = {"kind": "same_source_group", "group_id": "group:u/u", "claims": [], "drafts": [], "binding": str(a)}
            library = ({"kind": "PINNED_RUNTIME"}, runtime)

            def found(names, position):
                target = f"root:{position >> 4:04x}:{position & 15:04x}"
                entry = {"kind": "far-code-entry", "entry_target_id": target, "segment": position >> 4,
                         "offset": position & 15}
                a.write_text(json.dumps({"symbols": {n: entry for n in names}}))
                return {v.rsplit(":", 3)[0] for v in module_gate.name_consistency_violations([library, ({}, user)])}

            self.assertEqual(found(["__aFlmul", "__aFulmul"], at), set())
            # another name at that entry, or the same names at another address, stay violations
            self.assertEqual(found(["__aFlmul", "OTHER"], at), {"same-entry:group:u/u"})
            self.assertEqual(found(["__aFlmul", "__aFulmul"], at + 2), {"same-entry:group:u/u"})


class AlignmentTests(unittest.TestCase):
    """A contribution starts at a multiple of its SEGDEF alignment (whole-link refutation)."""

    def test_misaligned_starts(self):
        rows = [("a", "CODE", 2, 0x15d1), ("b", "CODE", 1, 0x15d1), ("c", "_DATA", 2, 0x1d6),
                ("d", "X", 3, 0x18), ("e", "Y", 3, 0x20)]
        self.assertEqual(module_gate.alignment_violations(rows), {"alignment:a:CODE@15d1", "alignment:d:X@18"})


class CaseAliasTests(unittest.TestCase):
    """A second case spelling of one external is refused unless recorded as debt."""
    SOURCE = (b"extern unsigned char room;\n/* x */\nextern unsigned char ROOM, other[4];\n"
              b"extern int pick(int a);\n")

    def test_new_alias_is_refused_and_debt_keeps_only_its_own(self):
        self.assertEqual(module_gate.case_aliases(self.SOURCE), ["ROOM"])
        with self.assertRaisesRegex(Exception, "two case spellings"):
            module_gate.require_no_case_alias(self.SOURCE, "group:new/new")
        with patch.object(module_gate, "CASE_ALIAS_DEBT", {"group:old/old": ["ROOM"]}):
            module_gate.require_no_case_alias(self.SOURCE, "group:old/old")
            with self.assertRaisesRegex(Exception, "OTHER"):
                module_gate.require_no_case_alias(self.SOURCE + b"extern int OTHER;\n", "group:old/old")


class ConstLayoutDebtTests(unittest.TestCase):
    """New explicit segment placement of data is refused; recorded debt keeps its source."""
    SOURCE = b'static const int _based(_segname("_CONST")) table[2] = {1, 2};\n'

    def test_new_segname_source_is_refused(self):
        with self.assertRaisesRegex(Exception, "_segname data placement"):
            module_gate.require_no_segname(self.SOURCE, "group:new/new")

    def test_listed_debt_and_plain_sources_pass(self):
        with patch.object(module_gate, "CONST_LAYOUT_DEBT", {"group:old/old"}):
            module_gate.require_no_segname(self.SOURCE, "group:old/old")
        module_gate.require_no_segname(b"static const int table[2] = {1, 2};\n", "group:new/new")
        module_gate.require_no_segname(self.SOURCE, "group:new/new", language="asm")
