"""Evidence forms of 2026-11-24: record walks, index floors, argument writes, fitted objects, far record windows and
structural entries behind a closing jump. Each is re-decoded from original bytes; every test also refutes a variant."""
import struct
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))

ROOT = Path(__file__).resolve().parents[1]


class WalkTests(unittest.TestCase):
    def walk(self, code, base):
        from binding import walk_extent
        return walk_extent(bytes.fromhex(code), base)

    def test_complete_loop_gives_the_records(self):
        # CLEAR_ROW_HANDLES: mov bx,5b9c; add bx,40; mov word [bx-40],0; cmp bx,5cdc; jb
        self.assertEqual(self.walk("bb9c5b83c340c747c0000081fbdc5c72f2", 0x5B9C), 4 * 64 + 2)
        self.assertIsNone(self.walk("bb9c5b83c340c747c0000081fbdc5c72f2", 0x5B9E))

    def test_other_shapes_prove_nothing(self):
        self.assertIsNone(self.walk("bb9c5bc747c00000", 0x5B5C))                       # one access, no loop
        self.assertIsNone(self.walk("bb9c5b83c301c647ff0081fb9f5b72f3", 0x5B9C))       # bytes: d = 1
        self.assertIsNone(self.walk("bb9c5b83c340c747c0000083c3003bd872f2", 0x5B9C))   # bound in a register
        self.assertIsNone(self.walk("bb9c5b83c34026c747c0000081fbdc5c72f1", 0x5B9C))   # ES, not DGROUP
        self.assertIsNone(self.walk("bb9c5b83c340c747c000008bd881fbdc5c72f0", 0x5B9C))  # the pointer is reloaded
        self.assertIsNone(self.walk("bb9c5bc747c0000081fbdc5c72f5", 0x5B5C))           # never ends


class FloorTests(unittest.TestCase):
    def floor(self, code, site):
        from index_floor import index_floor
        raw = bytes.fromhex(code)
        space = SimpleNamespace(data=raw, size=len(raw))
        return index_floor(space, 0, 0, raw.index(bytes.fromhex(site)))

    def test_original_floors(self):
        from index_floor import index_floor
        from oracle import Oracle
        root = Oracle.load().spaces["root"]
        # GETABOVEARY: column 1..10 in [bp-1], room*30 in SI; ADD_ROOM_TROBS: tile*5 behind `cmp si,1e; jge`
        self.assertEqual(index_floor(root, root.position(0xF9E, 0x1BA), 0xF9E0, root.position(0xF9E, 0x1BA) + 0xC5),
                         (0x2B99, 1))
        self.assertEqual(index_floor(root, root.position(0, 0xB78), 0, root.position(0, 0xB78) + 0x73), (0x5C5E, 150))
        self.assertIsNone(index_floor(root, root.position(0, 0xB78), 0, root.position(0, 0xB78) + 0x70))

    def test_counter_that_starts_at_one(self):
        # push bp; mov bp,sp; sub sp,2; mov byte [bp-1],1; L: mov al,[bp-1]; cbw; mov bx,ax; mov al,[bx+1233];
        # inc byte [bp-1]; cmp byte [bp-1],0b; jl L; mov sp,bp; pop bp; retf
        loop = "558bec83ec02c646ff01{}8a46ff988bd88a873312fe46ff807eff0b7ced8be55dcb"
        self.assertEqual(self.floor(loop.format(""), "8a873312"), (0x1233, 1))
        self.assertIsNone(self.floor(loop.format("").replace("c646ff01", "c646ff00"), "8a873312"))   # starts at 0
        self.assertIsNone(self.floor(loop.format("").replace("fe46ff", "fe4eff"), "8a873312"))       # counts down
        # lea ax,[bp-1] gives the slot's address away: nothing is known about it
        self.assertIsNone(self.floor(loop.format("8d46ff50"), "8a873312"))

    def test_guard_bounds_a_register_and_a_call_loses_the_others(self):
        # cmp si,1e; jl R; cmp si,28; jge R; mov bx,si; shl bx,1; mov al,[bx+1000]; R: retf
        self.assertEqual(self.floor("83fe1e7c0d83fe287d088bded1e38a870010cb", "8a870010"), (0x1000, 60))
        self.assertIsNone(self.floor("83fe1e7d0d83fe287d088bded1e38a870010cb", "8a870010"))
        # mov bx,5; call far; mov al,[bx+1000]
        self.assertIsNone(self.floor("bb05009a000000008a870010cb", "8a870010"))
        self.assertEqual(self.floor("bb05008a870010cb", "8a870010"), (0x1000, 5))
        self.assertIsNone(self.floor("bb0500268a870010cb", "268a870010"))                             # ES access
        # add bx,7fff may leave the signed range
        self.assertIsNone(self.floor("bb050081c3ff7f8a870010cb", "8a870010"))

    def test_frame_writes(self):
        from index_floor import frame_writes
        raw = bytes.fromhex("558becc6460c028b5e08890789571e5dcb")
        space = SimpleNamespace(data=raw, size=len(raw))
        self.assertEqual(frame_writes(space, 0, 0), [(12, 13)])
        raw = bytes.fromhex("558becc64200025dcb")
        self.assertEqual(frame_writes(SimpleNamespace(data=raw, size=len(raw)), 0, 0), [(-0x8000, 0x8000)])


class WriteAndFitTests(unittest.TestCase):
    def test_near_call_arguments(self):
        from binding import call_arguments
        # mov ax,6194; push ax; mov ax,6195; push ax; mov al,[bp-1]; push ax; push cs; call rel16
        raw = bytes.fromhex("b8946150b89561508a46ff500ee80000")
        self.assertEqual(call_arguments(raw), {8: 0x6195, 10: 0x6194})
        with self.assertRaisesRegex(ValueError, "far CALL"):
            call_arguments(bytes.fromhex("b8946150e80000"))          # a near call has another frame

    def test_argument_writes(self):
        from binding import argument_writes
        self.assertEqual(argument_writes(bytes.fromhex("8b5e088907895702"), {8: 0x6195}), [(0x6195, 0x6197), (0x6197, 0x6199)])
        with self.assertRaisesRegex(ValueError, "stores elsewhere"):
            argument_writes(bytes.fromhex("8b5e088907"), {10: 0x6195})
        with self.assertRaisesRegex(ValueError, "only MOV"):
            argument_writes(bytes.fromhex("8b5e08438907"), {8: 0x6195})
        with self.assertRaisesRegex(ValueError, "stores elsewhere"):
            argument_writes(bytes.fromhex("8b5e08b3008907"), {8: 0x6195})          # BL rewritten

    def test_only_one_place(self):
        from binding import fits_once
        self.assertTrue(fits_once(0x6194, 5, 0x6193, 0x619A, True))
        self.assertFalse(fits_once(0x6194, 5, 0x6193, 0x619A, False))
        self.assertFalse(fits_once(0x6194, 4, 0x6193, 0x619A, True))
        self.assertFalse(fits_once(0x6196, 5, 0x6193, 0x619A, True))
        self.assertTrue(fits_once(0x6940, 8, 0x6940, 0x6948, False))
        self.assertFalse(fits_once(0x6940, 7, 0x6940, 0x6948, False))

    def test_fitted_object_needs_two_claimed_sites_and_a_floor_one(self):
        from module_gate import _internal_support

        def run(symbol, fields):
            states = {p: {"v4": [{"key": ("A", k), "insn": at, "equal": True, "offset": at} for k, at in rows],
                          "equations": []} for p, rows in fields.items()}
            reports = [{"public": p, "exact": True} for p in fields]
            _internal_support({"A": dict(symbol, kind="dgroup-offset", offset=8)}, states, reports, set())
            return {r["public"]: r["exact"] for r in reports}
        fit = {"witnesses": [], "extent": {"length": 5}, "fit": {"below": {}, "above": {}},
               "members": [{"offset": 1, "witnesses": []}, {"offset": 2, "witnesses": []}]}
        self.assertEqual(run(fit, {"F": [(0, 1), (1, 2), (2, 3)]}), {"F": True})
        self.assertEqual(run(fit, {"F": [(0, 1)]}), {"F": False})
        self.assertEqual(run(dict(fit, fit=None), {"F": [(0, 1), (1, 2), (2, 3)]}), {"F": False})
        low = {"witnesses": [{}], "members": [{"offset": 0xFFFF, "witnesses": [], "floor": {"entry": ""}}]}
        self.assertEqual(run(low, {"F": [(0xFFFF, 1)]}), {"F": True})
        low["members"][0].pop("floor")
        self.assertEqual(run(low, {"F": [(0xFFFF, 1)]}), {"F": False})


class OriginalBindingTests(unittest.TestCase):
    """The accepted bindings, and what happens when one piece of their evidence is changed."""

    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        cls.oracle = Oracle.load()

    def ground(self, group, name, change=None, hull=("root", 0, 0, 1)):
        import copy
        import json
        from binding import ground
        binding = json.loads((ROOT / f"evidence/bindings/{group}.json").read_text())
        symbol = copy.deepcopy(binding["symbols"][name])
        if change:
            change(symbol)
        space, segment, offset, size = hull
        ground({**binding, "symbols": {name: symbol}},
               {"id": binding["target_id"], "space": space, "segment": segment, "offset": offset, "size": size},
               self.oracle, claimed=[])

    @staticmethod
    def member(symbol, offset):
        return next(m for m in symbol["members"] if m["offset"] == offset)

    def test_floor_must_reach_a_witnessed_address(self):
        for group, name, offset in (("misc/misc", "LEVEL", 0xFFE1), ("prince/prince", "SCRLEFT", 0xFF6A)):
            self.ground(group, name)
            for change in (lambda m: m["floor"].update(image_offset=m["floor"]["image_offset"] + 4),
                           lambda m: m.update(offset=m["offset"] - 1)):
                with self.assertRaisesRegex(ValueError, "Index floor"):
                    self.ground(group, name, lambda s: change(self.member(s, offset)))
            with self.assertRaisesRegex(ValueError, "Index floor|independent witness|positive offset"):
                self.ground(group, name, lambda s: (s.update(witnesses=[]),
                                                    [m.update(witnesses=[]) for m in s["members"]]))

    def test_fitted_objects(self):
        self.ground("misc/misc", "CELLRIGHT")
        for change in (lambda s: s["extent"].update(length=4),
                       lambda s: s["extent"]["writes"].pop(),
                       lambda s: s.update(offset=s["offset"] + 2),
                       lambda s: s["fit"].pop("above"),
                       lambda s: s.pop("extent")):
            with self.assertRaises((ValueError, KeyError)):
                self.ground("misc/misc", "CELLRIGHT", change)
        self.ground("movecave/movecave", "BRIDGE")
        for change in (lambda m: m["fit"].update(below=m["fit"]["above"]),
                       lambda m: m.pop("extent")):
            with self.assertRaises((ValueError, KeyError)):
                self.ground("movecave/movecave", "BRIDGE", lambda s: change(self.member(s, 4)))

    def test_far_record_windows_of_the_foreground_table(self):
        from far_binding import _record_window
        import json
        row = next(m for m in json.loads((ROOT / "evidence/bindings/drawruin/drawruin.json").read_text())
                   ["symbols"]["FORETABLE"]["members"] if m["offset"] == 0xFFF6)["record"]
        base, copied = row["base"], row["member"]
        self.assertEqual(_record_window(base, 0x31BD, 0x0A28, self.oracle), 20)
        self.assertEqual(_record_window(copied, 0x31BD, 0x0A1E, self.oracle), 20)
        self.assertIsNone(_record_window(base, 0x31BD, 0x0A29, self.oracle))
        self.assertIsNone(_record_window(copied, 0x31BD, 0x0A1F, self.oracle))
        with self.assertRaisesRegex(ValueError, "far pointer"):
            _record_window(base, 0x31BE, 0x0A28, self.oracle)
        self.assertIsNone(_record_window(dict(copied, bytes_hex=copied["bytes_hex"][:-2]), 0x31BD, 0x0A1E, self.oracle))

    def test_structural_entry_behind_a_runtime_members_closing_jump(self):
        from far_binding import _structural_entry
        root = self.oracle.spaces["root"]
        at = root.position(0x1FEF, 0x652)
        self.assertTrue(_structural_entry(self.oracle, at, 0x1FEF, 0x652, 186, "root", 0, 1)["anchor"]["closing_jump"])
        with self.assertRaises(ValueError):
            _structural_entry(self.oracle, at + 2, 0x1FEF, 0x654, 184, "root", 0, 1)


if __name__ == "__main__":
    unittest.main()
