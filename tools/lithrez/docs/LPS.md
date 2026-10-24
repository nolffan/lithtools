# PS2 sound banks (`.lps`)

The PS2 port of NOLF keeps its sounds in banks, "LithTech PSX Sound Data
1.20", inside `LITH_PS2.REZ`:

- `lps/<world>.lps`: a level's effect bank (doors, events, ambience,
  weapons, barks). Levels' banks share content; `LPS/global.lps` holds the
  interface sounds.
- `stream/mNN/vo_<world>.lps`: the level's voice lines, in the same
  container plus an SPU preload image.
- `stream/mNN/nolf_ps2_NN.wav`: the music, ordinary 16-bit PCM WAVs.

This summarises the layout from the nolfsrc ps2 branch
(`build/ps2/spec-sound.md`, `ps2sound.py`), adding the loop rules below.
`lithtech::sound::read_bank` reads it and `decode_bank_sound` decodes a
sound; on all 416 banks of both discs (58,403 sounds) the samples equal
`ps2sound.py`'s (`tests/compare_lps.py`).

## Layout (little-endian)

    0x00  char[32]  "LithTech PSX Sound Data 1.20\r\n\x1a\0"
    0x20  u32 data_size, n_groups, n_sounds, lip_size, spu_image_size
    0x34  sound[n_sounds]   72 bytes each
          group[n_groups]   48 bytes each
          data[data_size]   PS-ADPCM; sound offsets are relative to here
          lip[lip_size]
          spu_image[spu_image_size]
    (the sizes add up to the file's length exactly)

Sound: `char name[32]`, then u32 `flags` (bit 1: looping), `offset`, `size`
(a multiple of 16), `rate` (2,000 to 22,050 Hz; voice mostly 12,000),
`loop_start` (byte offset of the loop's first frame), `play_size`,
`lip_start`, `lip_count`, `spu_addr`, `spu_size` (the resident head in SPU
RAM, at most 0x3000; longer sounds stream).

Group: `char name[32]`, u32 `n_children`, `n_sounds`, `first_child`,
`first_sound`. The last group is the root; a sound's path (its PC path) is
the group names from the root down, then its name, e.g.
`GUNS\SND\P38\FIRE.WAV`.

Lip-sync: one byte (mouth opening) per 50 ms for the sounds characters
speak. The voice banks' SPU image is a copy of each sound's head, ready to
copy to SPU RAM.

## PS-ADPCM

Mono, 16-byte frames of 28 samples: byte 0 is filter (high nibble, 0 to 4)
and shift (low nibble, 0 to 12); byte 1 the flags (1 END, 2 LOOP, 4 LOOP
START); then 28 four-bit samples, low nibble first:

    s = (int16)(nibble << 12) >> shift
    s += (s1*f0 + s2*f1 + 32) >> 6     (f0,f1) = (0,0) (60,0) (115,-52) (98,-55) (122,-60)
    clamp to int16

Decoding stops after the first END frame (a frame flagged 7 ends it
unplayed). Loops, as the SPU plays them, in all 2,258 looping sounds:

- every looping sound marks frame 2 LOOP START; a sound with an intro marks
  its real loop start again later. The SPU repeats from the last LOOP START
  before the END+LOOP frame, which is always the record's `loop_start`;
- ten long streamed loops have no END frame: they loop from LOOP START to
  the end of their data (`play_size` 0);
- four 32-byte placeholders end END+LOOP at frame 1 with no LOOP START:
  they repeat from the start.
