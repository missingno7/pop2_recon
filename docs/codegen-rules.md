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
| ABI-4 | The state-step caller and following accepted callee emit 34 B together, publics at 0/22. Prototype-only caller uses CALL FAR; same-file caller uses PUSH CS/CALL near. Both caller contributions are 22 B because the near form gains a tail NOP. Complete callee CODE is unchanged; isolated callee OMF still equals its recipe. Grouped source remains research only. | `evidence/tu/state-step.json`, `docs/tu-state-step.md` | OBSERVED (fresh original-backed contrast) |
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
| REG-7 | Where a file-scope `extern` is declared changes register reuse in later functions: ComputeMobRect keeps the mob's room in AL across two compares (214 B) only with `lower_right_room` declared just before the redraw functions; reordering the neighbouring room externs gives 226 B, declaring it before the trob data 234 B. Use the single real declaration's position, never a second case spelling (docs/acceptance.md "One name per object"). | `src/move/move.c` (w32_move) | CONFIRMED |
| REG-8 | An unsigned-byte read promoted to int into a non-AX register: MSC 6.00A emits `MOV AL,[m]; SUB AH,AH; MOV DI,AX`, MSC 6.00 emits `MOV DI,[m]; AND DI,00FF` for the same C. Both builds emit the word-load/mask pair for `word & 255`. Six accepted 6.00A sites (draw, movespik, collide, move) and an isolated probe agree; static vs extern storage does not matter. | `src/final/final.c` TriggerCrystal/TriggerGargoyle (w34_final notes) | CONFIRMED |
| REG-9 | MSC reuses a constant already held in a register for a call argument only when the prototype parameter type matches (NIS await_palette needs `(void *, unsigned, unsigned)`); pass the literal the original passes, not a variable known equal to it. A NULL store to a pointer stays in place where an int 0 store is sunk into the zero-argument register. | `src/nis/nis.c` Transition (o1_nis) | CONFIRMED |

## Boolean and predicate shapes
| ID | Rule | Evidence | Status |
|---|---|---|---|
| BOOL-1 | `if (k==a\|\|k==b...) return 1; return 0;` compiles to a compare chain in source order, inline false exit `2bc0`, then an aligned `b80100` exit. | is_three/four/six/seven_kind | CONFIRMED |
| BOOL-2 | `if (...) result=1; else result=0; return result;` gives a DX result (frameless without arguments). | overlay_state_pair, overlay_state_window | CONFIRMED |
| BOOL-3 | Check nested false paths against the CFG branch outcomes before compiling: correct nesting plus a staged default Boolean gave 88 B; the combined form gave 74. | `src/is_kind_value.c`, `grinding-wave6.json` | CONFIRMED |
| BOOL-4 | `return a && b` gives two inline AX exits; assigning to a local result gives DX and a shared exit. | is_low5_range; in_remainder_window | CONFIRMED |
| BOOL-5 | Unsigned `x % n == 0` gives `cmp dx,1; sbb ax,ax; neg ax`. | `src/is_scalar_modulo_window.c` | CONFIRMED |
| BOOL-6 | In fallback_tile_kind case 4, one combined OR condition gives 158 B with a backward jump to an earlier JNE; two separate early `break` exits give the exact 160 B. The byte result local and sparse switch are unchanged. `/O` on the combined form gives 162 B and different tails/local placement. | `src/fallback_tile_kind.c`, `evidence/matching/larger-function-followup.json` | CONFIRMED (fresh contrast) |
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
| DATA-4 | Extern struct assignment emits immediate SI/DI bases, `push ds; pop es`, and MOVSW/MOVSW/MOVSB (5 B object) or REP MOVSW (20/64 B object); intrinsic `memset` emits REP STOSB. `/Oe` and `/O` objects agree. Extern fixups are LOC1/F5/T6, A=0, D omitted; same-file initialized/static storage uses refused F1/T4 DATA/BSS. Narrow resident v3 proves the immediate bases and ES=DS idiom. | `src/save_twenty_byte_state.c`, `src/clear_five_byte_state.c`, `evidence/toolchain/immediate-dgroup-msc600.json`, `docs/dgroup-binding.md` | CONFIRMED |
| DATA-5 | A two-word aggregate temporary under `/Oe` spills both words to `[BP-4]/[BP-2]`. | `evidence/toolchain/swap-results.json` | OBSERVED |
| NAME-1 | Renaming identifiers changes OMF identity, not CODE; one or two added used typedefs did not move three parked targets. | `docs/state-helpers.md`, wave9 parked contrasts | CONFIRMED / OBSERVED |
| NAME-2 | An unused declaration before referenced far externs changed CODE (126 vs 130 B); after them it did not. | `evidence/toolchain/convergence-audit.json` | OBSERVED (probe) |

## Object records and relocation order
| ID | Rule | Evidence | Status |
|---|---|---|---|
| OMF-1 | The 24-byte LOAD_AND_EXCHANGE_ATTRIBUTE emits far FIXUPPs at 13, then 7. A 300-call contrast emits descending FIXUPPs inside each of three LEDATA chunks, then proceeds to the next chunk. Preserve raw traversal; do not globally sort or reverse it. MASM's contrast ascends. | `src/load_and_exchange_attribute.c`, `evidence/rtlink-probe/relocation-order/receipt.json` | CONFIRMED (accepted component + fresh contrast) |
| LINK-1 | RTLink 6.10 buckets references by target symbol inside the combined output CODE segment, across modules and LEDATA; within each symbol, raw FIXUPP order survives. Equal-address public aliases stay distinct. Between-symbol order and original output placement remain structural debt. | `docs/far-call-binding.md`, `evidence/rtlink-probe/relocation-order/receipt.json` | OBSERVED (synthetic linker; original identity unproved) |
| OMF-1 | With `/Zi`, MSC 6.00 ends a LEDATA record at each function start, after a framed prologue, at every 52nd line-number entry counted from the start of the file, and at about 944 data bytes. RTLink keeps raw FIXUPP order within a symbol (LINK-1), so the original relocation order inside a function fixes how many code lines precede each split; joining `if (x) stmt;` onto one line changes the count without changing code. | `src/nis/nis.c` Transition (o1_nis feasible.py) | CONFIRMED |
| LINK-2 | In the tested source-only RTLink 6.10 configuration, same-file near calls resolve as `S-(P+2)` (forward +2, backward -32), unchanged after moving the whole contribution. Separately compiled caller/callee in the same combined UNIT_TEXT retain CALL FAR and its MZ relocation; the linker does not convert this control to the grouped encoding. | `evidence/rtlink-probe/near-public/receipt.json` | OBSERVED (bounded compiler/linker controls; no original-TU proof) |

## Closed families (unmatched targets)
- **root:1000:190c record-row helper, 90 B.** A six-trial bitfield follow-up produced 76/78/82 B, two MSC600 allocator ICEs and a final duplicate object. The 12/3/1 bitfield view explains byte extraction but not the four-byte frame, BP-2 pointer home or delayed BX-to-SI transfer. Stop until independent type/lifetime/TU evidence predicts those remaining differences. Receipt: `evidence/matching/larger-function-followup.json`; earlier arithmetic trials remain in the wave-9 worker journal.
- **overlay-2:28a1:0e68 swap, 46 B.** Closed: 277 trials, 39 forms (aggregate, long, float, union, scalar, qualifier, register); `/Oe /O /Ox /Or /Og /Os /Ol`; msc510/600/600A; wave-9 renames and used types. Open: context predicting low word in CX plus one `[BP-2]` spill in a 4-byte frame.
- **root:0000:3020 signed value, 26 B.** Closed: conditional, ternary, compound, local, pointer forms; register parameter `/O`; selective volatile; `/Og /Oes /Oer`; declaration reorder; renames, two typedefs. Open: context predicting `mov ax,dx; add ax,[bias]`.
- **overlay-5:29cf:0320 membership, 80 B.** Closed: OR, if, ternary, goto, parameter reuse, switch, staged Boolean; `/Oer /Oeg /Oers`; `/Od`+`/Oer`; `/Ge`; `check_stack`; 6.00AX; path length. Best 78 B, missing `MOV SP,BP`. Open: what triggers FRAME-2 without saves; `/O /Or` without `/Oe` is untried.
- **root:1101:4d6e rectangle, 34 B.** Closed: natural, short-OR, false-early-return, nested-result (`/Oe` 40); MSC510 `/O` 48; `/O` 38; `/Oeg`; operand orientation; renames, typedef. Open: early `mov ax,1`, CX/DX temporaries, single exit; `/Or`/`/Oer` untried.

## Untested sibling leads
- SimAnt: declaration-count sensitivity is periodic mod 17 (symbol hash); pop2 tried only +1/+2.
- SimAnt VER-3 / C4203: whole-file pass-2 budget effects; pop2 tested only single-function files.
- SimAnt "slots follow first use", Stunts L1 "BP homes follow identifier hash": relevant to the swap's `[BP-2]`.
- SimAnt ZI-1/ZD-1: `/Zi`/`/Zd` move LEDATA breaks only — matters for relocation order, not CODE.
