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
        diag = json.loads((ROOT / "evidence/bindings/diag/diag.json").read_text())
        cls.base = {k: diag[k] for k in ("schema", "mode", "target_sha256", "frame", "dgroup_segment")}

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
        # "Pos=%d" occurs once in the original DGROUP, at DS:111e.
        report = self.judge('char pos_format[] = "Pos=%d";\n', 0x111E, "pos")
        self.assertTrue(report["placements"]["_DATA"]["unique"])
        self.assertEqual([(d["name"], d["address"]) for d in report["data_publics"]], [("POS_FORMAT", 0x111E)])
        with self.assertRaisesRegex(ValueError, "differs"):
            self.judge('char pos_format[] = "Pos=%d";\n', 0x1120, "pos_wrong")

    def test_code_is_refused(self):
        with self.assertRaisesRegex(ValueError, "emits no code"):
            self.judge('char pos_format[] = "Pos=%d";\nint f(void) { return 1; }\n', 0x111E, "pos_code")


class AggregateExtentTests(unittest.TestCase):
    """An original block copy anchored at S proves [S, S+L) is one copied unit (MOVE's 13-byte mob
    records: `lea si,[bx+2aa4]; ...; mov cx,6; rep movsw; movsb`)."""

    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        cls.root = Oracle.load().spaces["root"]

    def test_source_and_destination_copies_give_the_record_length(self):
        from binding import block_copy_extent
        self.assertEqual(block_copy_extent(self.root.data[0xCE7F:0xCE8B], 0xCE7F, 0x2AA4), 13)
        self.assertEqual(block_copy_extent(self.root.data[0xCE9F:0xCEB0], 0xCE9F, 0x2AA4), 13)

    def test_other_base_or_control_transfer_proves_nothing(self):
        from binding import block_copy_extent
        self.assertIsNone(block_copy_extent(self.root.data[0xCE7F:0xCE8B], 0xCE7F, 0x2AA5))
        # lea si,[bx+2aa4]; jmp short $+2; mov cx,6; rep movsw: a jump inside the window
        with self.assertRaisesRegex(ValueError, "straight-line"):
            block_copy_extent(bytes.fromhex("8db7a42aeb00b90600f3a5"), 0, 0x2AA4)
        # the count register rewritten after `mov cx,n`: no proven length
        self.assertIsNone(block_copy_extent(bytes.fromhex("8db7a42ab9060041f3a5"), 0, 0x2AA4))


class BssLayoutTests(unittest.TestCase):
    """bss-layout-v1: anchors come from the original C startup; impossible layouts are flagged."""

    def test_anchors_are_the_original_startup_operands(self):
        from oracle import Oracle
        root = Oracle.load().spaces["root"]
        word = lambda at: struct.unpack_from("<H", root.data, at)[0]
        self.assertEqual((root.data[0x1FF57], word(0x1FF58)), (0xBF, module_gate.BSS_START))   # mov di,_edata
        self.assertEqual((root.data[0x1FF5A], word(0x1FF5B)), (0xB9, module_gate.END))         # mov cx,_end
        self.assertEqual((root.data[0x200B7], word(0x200B8)), (0xBE, module_gate.BSS_END))     # mov si,xontab
        self.assertEqual((root.data[0x200BA], word(0x200BB)), (0xBF, module_gate.BSS_END))     # mov di,xontab

    def test_violations(self):
        recipe = {"language": "c", "group_id": "group:t/t", "name": "t/t"}
        groups = {"r.json": {"placements": {"_BSS": {"placement": 0x3FC2, "length": 4}}}}
        self.assertEqual(module_gate.bss_layout_violations(groups, [({"recipe": "r.json"}, recipe)]),
                         {"group:t/t:_BSS@3fc2"})
        ok = {"r.json": {"placements": {"_BSS": {"placement": 0x2AA4, "length": 4}}}}
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
