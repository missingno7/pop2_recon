# Game C leaf match

`root:02cc:08c2` is a complete 20-byte root-space function with a reviewed entry and exit. The far call at root image position `0x1999` (with its relocated segment word at `0x199c`) encodes `02cc:08c2`, which maps to root position `0x3582`; the caller pushes the signed word followed by a near pointer. The candidate body decrements the signed word when negative, stores it through the pointer, and returns with `RETF 4`.

The accepted historical C is [floor_adjust.c](../src/floor_adjust.c), with its pinned [recipe](../recipes/floor_adjust.json). It uses an explicit far Pascal function and an explicit near data pointer. Microsoft C 6.00 with `/c /AM /Oe /Gs /Zl /Gc` emits one complete 20-byte CODE segment matching the entire reviewed extent byte for byte. The independent candidate and callsite evidence is [game_leaf.json](../evidence/matching/game_leaf.json).

The semantic label `store_negative_adjusted_word` is a hypothesis for this helper's behavior. The original source symbol and precise gameplay role remain unrecovered. The object has no actual fixups; six OMF fixup-thread declarations and an unreferenced `FLOOR_ADJUST` external declaration are retained in the report. Complete CODE equality establishes this function extent only; original translation-unit identity and structural linking remain unproved.

Compiler discrimination: MSC 5.10 rejects `/Oe`; its supported `/Ox` variant
emits 22 bytes, differing at offset 3. The pinned 6.00A C2L-bound profile also
matches all 20 bytes under `/c /AS /Oe /Gs /Zl /Gc`. Thus this target supports
the MSC 6 generation and its optimization behavior, while exact revision and
memory model remain ambiguous. Those observations never redefine accepted bytes.
