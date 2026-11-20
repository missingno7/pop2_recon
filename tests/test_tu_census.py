"""Near-call attribution wraps IP inside the caller's 64K segment."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from oracle import Space
from tu_census import near_call_sites


class NearCallWrapTests(unittest.TestCase):
    def test_backward_call_beyond_64k_from_space_start_wraps_into_segment(self):
        # Segment base 0x11010; caller at base+0x8a93 calls base+0x02f6 with a
        # negative displacement. Unwrapped arithmetic would land outside the segment.
        base = 0x11010
        site = base + 0x8a93
        data = bytearray(base + 0x10000)
        rel = (0x02f6 - (0x8a93 + 4)) & 0xffff
        data[site:site + 4] = bytes([0x0e, 0xe8, rel & 0xff, rel >> 8])
        space = Space("root", 0, 0, bytes(data), ())
        calls = list(near_call_sites(space, set(), lambda pos: base if pos >= base else None))
        self.assertEqual(calls, [(site, base + 0x02f6)])

    def test_relocated_bytes_are_not_near_calls(self):
        data = bytearray(64)
        data[10:14] = b"\x0e\xe8\x00\x00"
        space = Space("root", 0, 0, bytes(data), ())
        self.assertEqual(list(near_call_sites(space, {12}, lambda pos: 0)), [])


if __name__ == "__main__":
    unittest.main()
