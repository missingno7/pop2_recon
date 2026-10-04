# Initial function discovery

`tools/discover.py` creates a candidate inventory from immutable bytes exposed by
`Oracle.load()`. It uses the supplied Capstone 5.0.3 package at
`C:\tools\capstone-5.0.3`; it does not install or download dependencies. Run it
from the repository root with:

```powershell
python tools/discover.py
```

The default output is `build/workers/inventory/discovery.json`. That file is a
generated working inventory, not canonical evidence. The initial MZ entry,
direct near calls, relocated far calls, relocated far pointers into prologues,
the public reference prologue list, and the reference extra-entry list have
separate provenance labels. A raw `push bp; mov bp,sp` byte pattern remains a
low-confidence candidate and does not establish an instruction boundary.

Each candidate uses the stable `space:segment:offset` identity. Its extent is a
tentative linear sweep ending at a return, undecodable byte, unconditional jump,
or configured scan limit. Conditional branches can make this estimate include
data or miss non-linear code; extent status always records that it is tentative.
Unreached bytes stay unclassified. Candidate rows include caller and callee
links, discovery evidence, and scan stop reason. This inventory does not claim
that every executable byte is code, or that each candidate is a canonical
function. Every row starts at `state: UNKNOWN`; ABI and class hypotheses remain
empty until separately reviewed.

To write a compact evidence packet for one ID after discovery:

```powershell
python tools/context.py root:1101:08d2 --asm
```

The packet is written under `build/workers/inventory/context.json` by default.
It reloads `Oracle` and rejects a discovery file whose target SHA-256 is stale.
`--asm` adds a bounded disassembly of the immutable bytes with its own SHA-256,
file positions, and input CS:IP addresses; a canonical address is shown
separately. If a requested ID is absent from discovery, the CLI can use its
reviewed row in `evidence/targets.json` after checking the executable and extent
hashes against `Oracle`. It attaches matching canonical ownership from
`layout/manifest.json` and the referenced recipe as separate metadata, without
changing discovery state. In particular, Oracle models the known one-byte
physical shortfall at overlay 17 without manufacturing the missing byte.

The matching and validation stage must independently check every proposed
identity and extent against `Oracle` bytes. The public reference checkout is
useful as attributed evidence, but is not the byte authority.
