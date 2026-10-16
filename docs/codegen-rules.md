# MSC code-generation rules (pop2)

Observed compiler and source-shape facts from byte-exact matching, kept in one
place so workers stop rediscovering them. Profile `msc600` (MS C 6.00 under
MS-DOS Player) with `/c /AM /Oe /Gs /Zl /Gc` unless stated. Each rule holds only
for the cited source, flags, profile and complete object; none proves the
original game's flags, TU, compiler revision or model.

**CONFIRMED** = reproduced in an accepted CODE_EXACT component.
**OBSERVED** = seen only in rejected trials, original-byte review or probes.
Add a rule only when an accepted component or a fresh contrast confirms it;
update the closed-family list instead of repeating its experiments.

## Flags and toolchain
| ID | Rule | Evidence | Status |
|---|---|---|---|
| FLAG-1 | `/c /AM /Oe /Gs /Zl /Gc` reproduces nearly every accepted C component; the setter needs `/O /Or`. | `recipes/*.json` | CONFIRMED |
| FLAG-2 | `/Oe` and `/O` can be identical (increase_step) or differ: overlay_state_window `/O` 62 B rejected; rectangle `/O` 38 vs `/Oe` 40; setter `/Oe` sinks stores. | `docs/data-helpers.md`, `grinding-wave3.json`, `grinding-wave7.json` | CONFIRMED (contrast) |
| FLAG-3 | `/Or` merges return paths into one exit: setter exact 34; membership 78; increase_step 58 (target 56). | `src/set_state_from_record.c`, `grinding-wave3.json` | CONFIRMED |
| FLAG-4 | `/Os` removes internal alignment NOPs (membership `/Oers` 77 B). | `grinding-wave3.json` | OBSERVED |
| FLAG-5 | `/Og` with `/Oe` changed no observed output (membership 94, rectangle 40, signed 26-byte body). | waves 2, 3, 7 | OBSERVED |
| FLAG-6 | `/Gs` is required: `/Ge` adds a far `__aFchkstk` call and fixup (membership 88 B). | `grinding-wave4.json` | OBSERVED |
| FLAG-7 | MSC 5.10 rejects `/Oe` (`/Ox` gives floor_adjust 22 B). 6.00A C2L matches floor_adjust; 6.00AX objects equal 6.00 for is_four_kind and membership. | `docs/game-leaf-matches.md`, `docs/convergence-audit.md` | CONFIRMED / OBSERVED |
| FLAG-8 | Working-directory path length (62 vs 123 chars) leaves the whole object unchanged (pass directory fixed). | `build/workers/wave9_parked_sol/d9.json` | OBSERVED |
| FLAG-9 | `#include` is refused (no pinned headers): declare externs and types inline. | `docs/compiler-codegen.md` | tool rule |

## ABI, frames and alignment
| ID | Rule | Evidence | Status |
|---|---|---|---|
| ABI-1 | `/Gc` makes far functions Pascal: uppercase public, first argument at the highest BP offset, callee cleanup `RETF 2n`. | `src/make_index_rect.c`, `src/signed_byte_times10_adjusted.c` | CONFIRMED |
| ABI-2 | A `char` parameter occupies a word slot; only the low byte is read. A `char` return lives in AL only (AH unspecified). | `src/is_three_kind.c`, `src/select_pair_code.c`, `src/signed_div63_adjusted.c` | CONFIRMED |
| ABI-3 | A same-CS caller of a far function uses `PUSH CS; CALL near` (432 decoded original pairs); MSC 6.00 emits it for a callee defined in the same source file. | `evidence/matching/wave9-far-call-review.json` | OBSERVED (probe + original) |
| FRAME-1 | No arguments and no stack locals: no BP frame, ends in `CB`. A stack local forces a frame. | clear_steps, clear_state_bytes, increase_step, is_special_state, overlay_state_pair | CONFIRMED |
| FRAME-2 | `MOV SP,BP` is emitted whenever `SUB SP` or an SI/DI save exists; it also appears with neither in adjust_byte_for_kind and is_kind_value (trigger not isolated). | accepted sources | CONFIRMED (occurrence) |
| FRAME-3 | Without `/Or`, every `return` gets its own `5D CA nn 00` epilogue. | is_four_kind, is_low5_range, increment_record_index | CONFIRMED |
| ALIGN-1 | A `90` precedes a jump target only after an unconditional transfer; CODE is padded to even. The trailing NOP belongs to the owned extent — never append it by hand. | 17 accepted tails; is_kind_value internal `eb3b 90` | CONFIRMED |

## Registers
| ID | Rule | Evidence | Status |
|---|---|---|---|
| REG-1 | Under `/Oe`, an explicit `register int` parameter/local went to DI (push DI), not SI. | `src/in_remainder_window*.c`, `src/append_record_if_room.c` | CONFIRMED |
| REG-2 | Under `/Oe`, `register` is only a hint: floor_adjust keeps its value in DX with no save. | `src/floor_adjust.c` | CONFIRMED |
| REG-3 | `/O /Or` with an explicit `register` near-pointer parameter gives saved SI, separate DS stores and one `MOV SP,BP` exit; without `register` the same flags give a 30-byte BX body. | `src/set_state_from_record.c`, `wave9_parked_sol/summary.json` | CONFIRMED |
| REG-4 | `/Oe` global allocation puts plain parameters in DI/SI; save order is push DI, push SI. | `src/make_index_rect.c`, `src/append_record_if_room.c` | CONFIRMED |
| REG-5 | `char` locals are not enregistered: stack home `[BP-1]` with `SUB SP,2`; times10 keeps an unused home while the value stays in AL. | select_pair_code, signed_div63, update_pair_*, times10 | CONFIRMED |
| REG-6 | An int Boolean `result` local lives in DX and exits through `MOV AX,DX`; near-pointer temporaries use BX. | is_level4_byte_window, is_low5_special, overlay_state_*, is_kind_value | CONFIRMED |

## Boolean and predicate shapes
| ID | Rule | Evidence | Status |
|---|---|---|---|
| BOOL-1 | `if (k==a\|\|k==b...) return 1; return 0;` compiles to a compare chain in source order, inline false exit `2bc0`, then an aligned `b80100` exit. | is_three/four/six/seven_kind | CONFIRMED |
| BOOL-2 | `if (...) result=1; else result=0; return result;` gives a DX result (frameless without arguments). | overlay_state_pair, overlay_state_window | CONFIRMED |
| BOOL-3 | Check nested false paths against the CFG branch outcomes before compiling: correct nesting plus a staged default Boolean gave 88 B; the combined form gave 74. | `src/is_kind_value.c`, `grinding-wave6.json` | CONFIRMED |
| BOOL-4 | `return a && b` gives two inline AX exits; assigning to a local result gives DX and a shared exit. | is_low5_range; in_remainder_window | CONFIRMED |
| BOOL-5 | Unsigned `x % n == 0` gives `cmp dx,1; sbb ax,ax; neg ax`. | `src/is_scalar_modulo_window.c` | CONFIRMED |
| BOOL-6 | An `unsigned long` zero guard gives `mov ax,[hi]; or ax,[lo]` with BX result (word-pair binding). | `src/is_special_state.c` | CONFIRMED |
| BOOL-7 | Short-circuit OR returned directly: `/O` 38 B, late result, two exits; `/Oe` nested result gives saved SI, BX result. Neither gives early AX=1 with one exit. | `grinding-wave7.json` | OBSERVED |

## Arithmetic and data access
| ID | Rule | Evidence | Status |
|---|---|---|---|
| ARITH-1 | Signed `/63`, `%32` emit `cwd; idiv cx` (no shift reduction); unsigned emits `sub dx,dx; div`. | signed_div63_adjusted, in_remainder_window, is_scalar_modulo_window | CONFIRMED |
| ARITH-2 | In-place parameter normalization (`v = (v-144) % 32`) gives `lea ax,[bx-144]`. | in_remainder_window | CONFIRMED |
| ARITH-3 | A natural ternary clamp `(e>310)?310:e` matches; the local-variable form is rejected. | `src/make_index_rect.c` | CONFIRMED |
| ARITH-4 | `signed char * 10` emits byte `shl`/`add`; global `x = -x` emits `neg byte [mem]`; `char zero=0` gives `sub al,al` plus repeated AL stores. | times10, update_pair_*, clear_steps | CONFIRMED |
| ARITH-5 | `(unsigned)uc & 31` on a char parameter emits `mov dl,[bp+6]; and dx,1f`. | is_low5_range, is_low5_special | CONFIRMED |
| DATA-1 | `volatile` forces each source read of a global and a reload after a compound store. | adjust_byte_for_kind, apply_adjustment, is_level4_byte_window | CONFIRMED |
| DATA-2 | Constant indexes fold into displacements (`p[16]` → `[si+10]`); `#pragma pack(1)` reproduces odd member offsets. | setter, decrement_record_index, append_record_if_room | CONFIRMED |
| DATA-3 | A `far *` parameter loads with `les bx,[bp+6]` plus `ES:` overrides. LES/LDS are refused only when a DS data binding is needed; relocation-free LES is fine. | rectangle; `docs/dgroup-binding.md` | OBSERVED |
| DATA-4 | Original structure copies are frameless `push di; push si; mov di,imm; mov si,imm; push ds; pop es; rep movsw`. The gate cannot own them yet (immediate DS operands, ES write). Uncompiled. | root:02cc:13e4, 1c3e (`wave9_review_a_sol/review.json`) | OBSERVED |
| DATA-5 | A two-word aggregate temporary under `/Oe` spills both words to `[BP-4]/[BP-2]`. | `evidence/toolchain/swap-results.json` | OBSERVED |
| NAME-1 | Renaming identifiers changes OMF identity, not CODE; one or two added used typedefs did not move three parked targets. | `docs/state-helpers.md`, wave9 parked contrasts | CONFIRMED / OBSERVED |
| NAME-2 | An unused declaration before referenced far externs changed CODE (126 vs 130 B); after them it did not. | `evidence/toolchain/convergence-audit.json` | OBSERVED (probe) |

## Closed families (unmatched targets)
- **overlay-2:28a1:0e68 swap, 46 B.** Closed: 277 trials, 39 forms (aggregate, long, float, union, scalar, qualifier, register); `/Oe /O /Ox /Or /Og /Os /Ol`; msc510/600/600A; wave-9 renames and used types. Open: context predicting low word in CX plus one `[BP-2]` spill in a 4-byte frame.
- **root:0000:3020 signed value, 26 B.** Closed: conditional, ternary, compound, local, pointer forms; register parameter `/O`; selective volatile; `/Og /Oes /Oer`; declaration reorder; renames, two typedefs. Open: context predicting `mov ax,dx; add ax,[bias]`.
- **overlay-5:29cf:0320 membership, 80 B.** Closed: OR, if, ternary, goto, parameter reuse, switch, staged Boolean; `/Oer /Oeg /Oers`; `/Od`+`/Oer`; `/Ge`; `check_stack`; 6.00AX; path length. Best 78 B, missing `MOV SP,BP`. Open: what triggers FRAME-2 without saves; `/O /Or` without `/Oe` is untried.
- **root:1101:4d6e rectangle, 34 B.** Closed: natural, short-OR, false-early-return, nested-result (`/Oe` 40); MSC510 `/O` 48; `/O` 38; `/Oeg`; operand orientation; renames, typedef. Open: early `mov ax,1`, CX/DX temporaries, single exit; `/Or`/`/Oer` untried.

## Untested sibling leads
- SimAnt: declaration-count sensitivity is periodic mod 17 (symbol hash); pop2 tried only +1/+2.
- SimAnt VER-3 / C4203: whole-file pass-2 budget effects; pop2 tested only single-function files.
- SimAnt "slots follow first use", Stunts L1 "BP homes follow identifier hash": relevant to the swap's `[BP-2]`.
- Stunts E6: struct assignment emits `rep movs` (DATA-4 targets).
- SimAnt ZI-1/ZD-1: `/Zi`/`/Zd` move LEDATA breaks only — matters for relocation order, not CODE.
