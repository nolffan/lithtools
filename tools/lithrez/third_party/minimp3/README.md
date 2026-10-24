minimp3.h and LICENSE (CC0 1.0) from https://github.com/lieff/minimp3 at
commit ea99364f61c14656440e8d77e9c233ccf3124633 (2026-07-27). LithRez
Studio uses it to play the MPEG layer 3 sounds in .wav files.

One change, marked "lithtools" in the source: in MPEG-2 (22 and 24 kHz)
intensity stereo, minimp3 took a scalefactor band's largest value as an
"illegal" intensity position and rebuilt the right channel as a copy of
the left. FFmpeg and mpg123 take it as a position, and so do the games'
stereo dialogue: Tron 2.0's VOICE files decode within 1 of mpg123 with
the change, and up to 9,596 apart (12.5 dB SNR) without it.
tests/compare_sound.py checks the MP3 sounds against FFmpeg.
