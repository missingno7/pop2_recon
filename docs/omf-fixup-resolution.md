# Symbolic 16-bit OMF FIXUPP resolution

`tools/omf.py` now adds a `resolved` array to each `ObjectModule.fixups` row.
It parallels that FIXUPP record's existing `decoded` subrecords; the original
record body, byte order, and decoded fields are retained. Thread tables are
independent for frame and target threads, persist across FIXUPP records, and a
later definition replaces an earlier definition with the same number.

For a resolved fixup the diagnostic row reports its LEDATA segment and the
absolute offset within that SEGDEF. The LOCAT data-record offset is added to
the nearest preceding LEDATA base. The complete relocation field must fit both
that LEDATA payload and the declared SEGDEF extent. `field_width` describes
the LOCAT field size; `self_relative` is true when LOCAT.M is zero. The resolver
does not patch the bytes, infer a final address, or turn a partially relocated
object into comparable machine code.

Inline and threaded frame/target references are mapped symbolically to the
SEGDEF, GRPDEF, EXTDEF, absolute frame, or location segment they identify.
The FIXDAT P bit both controls whether a target displacement word is present
and supplies the high bit of the target method. Thus target-thread method T0
with P set resolves as T4 (segment with zero displacement), and similarly T1
to T5 and T2 to T6. T3 is decoded diagnostically as an explicit 16-bit frame
number in its two-byte target datum, including in a TARGET THREAD definition;
the high byte is not parsed as an OMF index marker. Microsoft LINK does not
support T3, so this symbolic description is not evidence that such an object
is linkable. A TARGET THREAD's raw high method bit is ignored as specified;
raw method 7 is therefore T3 and uses the same word datum. The TIS/MS linker
profile marks frame method F3 invalid; older Intel OMF formats describe an
explicit frame number, but this MSC 16-bit reader rejects F3 intentionally.
Effective T7 and F6 are rejected. Inline F5 and threaded F5 produce the same
`target_frame` representation. It retains the target binding, including an
EXTDEF name whose final frame depends on another module's PUBLIC definition.

Unknown thread numbers, invalid table indexes, unsupported LOCAT widths,
fields outside their anchoring LEDATA/SEGDEF, and fixups anchored to LIDATA
are rejected. For this 16-bit MSC reader, recognized LOCAT values are 0–5;
later-defined 32-bit LOCAT values 9, 11, and 13 are intentionally refused,
along with 32-bit FIXUPP records. LIDATA expansion is parsed for data coverage,
but a relocation's record-relative offset can point into a repeated literal; mapping those
positions is deliberately outside this resolver. COMDAT and communal storage
are not accepted by the object reader. These are diagnostic OMF semantics;
binding acceptance is a separate, explicitly restricted proof gate.

The field interpretation follows Intel's *8086 Relocatable Object Module
Formats*, version 4.0 (FIXUPP/THREAD sections), and the later Tool Interface
Standards *OMF Specification*, version 1.1, sections 3.2–3.3. The original
Intel THREAD datum diagram labels its conditional field “Index or Frame
Number”; absolute T3 uses the frame-number form (two bytes), while T0–T2 use
the variable-width index form. The high-byte unit test distinguishes these
encodings. Later 32-bit LOCAT values and record types are outside this reader's
deliberate profile. `loc_type` now travels with each resolved row so different
two-byte operation kinds cannot be mistaken for one another. A separate narrow
[external data proof](dgroup-binding.md) now accepts independently grounded
offset16 components; the runtime and other fixup modes remain rejected.
The resolver implementation is local
code. A sibling OMF parser was consulted as a behavior cross-check only; no
sibling source was copied. See [the isolated MSC probes](../evidence/toolchain/omf-fixup-probes.json)
for compiler and runner identities, hashes, switches, object hashes, and the
observed fixup locations.

- [Intel 8086 Relocatable Object Module Formats, v4.0 (PDF)](https://elhacker.info/manuales/Hardware/Intel/Intel%20121748-001%208086%20Relocatable%20Object%20Module%20Formats%20Nov81.pdf)
- [Tool Interface Standards OMF Specification, v1.1 (PDF)](https://sininenankka.dy.fi/~sami/watcom_mirror/devel/omf.pdf)
- [IBM OS/2 OMF Specification, Revision 8 (PDF; FIXUPP/THREAD, pp. 38–41)](https://archive.decromancer.ca/bitsavers.org/pdf/ibm/pc/os2/OS2_OMF_and_LX_Object_Formats_Revision_8_199406.pdf)
