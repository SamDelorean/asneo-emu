# R8 — AlphaWord registration rejected after failed CFI NOR geometry query (2026-11-14)

## User acceptance criterion
On an NEO with AlphaWord correctly installed, first boot `Y` then `Enter` must continue to the text editor. The emulated `Updater Mode` is a **failure**, not successful boot. Keep the two private BOOT and WRITER sets and the sole `neo-diag`; no ROM edits or forced PC.

## Primary firmware reverse engineering
Original NEO 2013 System 3, verified from the canonical private ROM (not invented code):
- `0x00417096`: calls `0x004130CC`, the flash-size probing helper; result in `D6`. `0x0041709C` initializes `[0x00000E8A]=0x00470000` (start).
- `0x004170A6–0x004170E6`: computes `[0x00000E8E]` from the probe result and Flash base. In R7/R8 it is `0x0040000C` (only 12 bytes beyond base), less than the applet start.
- `0x0041331A`: original System 3 reads `0x00470000` and recognizes `C0FFEEAD`; `0x0041332E` reads applet length `0x0001A0BC`; `0x00413350–0x0041335A` checks that its end fits in the calculated NOR storage bound and takes the rejection path.
- Original helper `0x004130CC` calls `0x00412D78`; it transfers a firmware NOR probe to RAM `0x00000490`. That probe includes byte-wide CFI read of `QRY` (0x51,0x52,0x59).

**R8 native register traces** show two CPU-originated reads of AlphaWord signature and four halfword reads of its size at frame 336 on WRITER, before the applet slot remains zero. BOOT has no valid applet header, as designed. [WRITER full trace](https://drive.google.com/file/d/1WkmLIZ68CIhngfV3pHupByUTn7IJeagW/view), [BOOT control](https://drive.google.com/file/d/1VdGklPd0Y7yhNuv16n8QgZVCxai6j-mT/view), [completed job](https://drive.google.com/file/d/1_voVlKkBcyZRdVohsuApssjlHPQbjEOr/view).

The RAM section `0x00000E0A..0x00000E89` is the 32-slot `AppletHeader_t*` table in NEO13. In **this firmware's source** `0x00000E8A` and `0x00000E8E` are explicitly used as 32-bit storage bounds by `0x0041331A`; do not transfer the companion-table layout from NEO 2005.

## Confirmed failing bus transaction (not a guess)
Follow-up non-invasive MAME pass-through taps (native T, `NEO_DIAG_R8_CFI_PROBE_20261114`, PASS) captured:

```
R8_CFI_TAP kind=gate frame=9 PC=000004C4 addr=FFFFF110 data=00007F7F mask=0000FF00
R8_CFI_TAP kind=cmd frame=9 PC=000004D4 addr=004000AA data=00009898 mask=0000FF00
R8_CFI_TAP kind=qry frame=9 PC=000004DC addr=00400020 data=00000000 mask=000000FF
```

The 16-bit flash handler `neo_nor.h` rejected every `mem_mask!=0xffff` before processing CFI. Thus native firmware's high-byte `MOVE.B #0x98,$4000AA` **never enters query state**; reading byte at `0x400021` yields `0x00` rather than `0x51` (Q). [CFI trace](https://drive.google.com/file/d/14QfpCgJcffCBKrhmNW3-7fc9QQ6sd-rK/view) and [PASS receipt](https://drive.google.com/file/d/1W2ScSF-9eYyXZauGzGDQF0gJspPdt1hm/view).

## Minimal hardware-corrective source patch
- `src/mame/skeleton/neo_nor.h`: for the **already selectable** provisional M29W320EB profile only, permit exact high-byte CFI-entry command at `byte_offset=0xAA`, data 0x98, lane `0xff00`; and byte 0xF0 to exit query. All other byte-lane program/erase/unlock commands remain rejected.
- `tests/neo_nor.cpp`: regression tests for byte-lane QRY and QRY exit with CFI geometry 4MiB, plus negative tests proving unknown/read-only profile and arbitrary byte commands are still rejected.
- No hardware identification is claimed by this provisional profile; **do not change its default** and do not broaden x8 support without evidence.
- **Code committed but native compiled-run qualification is pending**. Do not claim BOOT PASS, AlphaWord visible, or that the applet register is fixed until running on T using the ST profile.

## Next native acceptance
One incremental compile in the current `neo-diag` tree, run `tests/neo_nor.cpp`, first inspect CFI `QRY` read from `0x400021` with ST profile selected, then execute WRITER on isolated NVRAM with Y→Enter. Pass only if ROM validates, `0x00000E8E` becomes a credible boundary, slot1 points to AlphaWord header, PC enters the applet area, LCD shows editor and keyboard text input works. If the strict CFI fix is insufficient, preserve the bin and trace the next observed native NOR command; do not invent broader behavior.
