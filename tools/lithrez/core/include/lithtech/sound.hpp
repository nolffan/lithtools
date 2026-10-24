// Sounds in .wav files, decoded to 16-bit PCM for playback. The shipped
// games use four codecs inside RIFF WAVE: PCM (8 and 16 bit), IMA ADPCM,
// MPEG layer 3 (most dialogue in NOLF, NOLF2 and Contract J.A.C.K.) and,
// once in Tron 2.0, 32-bit float. MS ADPCM, A-law and mu-law are read too.
//
// The LithTech chunks are read as the engine's GetWaveInfo reads them
// (Contract J.A.C.K.'s runtime/sound/src/wave.cpp): 'lith' inside a
// LIST 'info' holds the sound buffer flags and the random pitch range, and
// the first 'cue ' point on the data, with the 'ltxt' of the same name in
// LIST 'adtl', is the loop.

#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace lithtech::sound {

enum FormatTag : uint16_t {
    WAVE_PCM = 0x0001, WAVE_MS_ADPCM = 0x0002, WAVE_IEEE_FLOAT = 0x0003, WAVE_ALAW = 0x0006,
    WAVE_MULAW = 0x0007, WAVE_IMA_ADPCM = 0x0011, WAVE_MPEGLAYER3 = 0x0055, WAVE_EXTENSIBLE = 0xFFFE,
};

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// What the file says about itself.
struct Info {
    uint16_t format_tag = 0;          // the codec (WAVE_EXTENSIBLE resolved to its subformat)
    int channels = 0;
    int sample_rate = 0;
    int bits_per_sample = 0;          // 0 for MP3; 4 for ADPCM
    int block_align = 0;
    uint32_t avg_bytes_per_sec = 0;
    uint32_t data_bytes = 0;          // the coded data, as present in the file
    bool data_truncated = false;      // the data chunk claims more than the file holds
    int samples_per_block = 0;        // ADPCM
    bool has_fact = false;
    uint32_t fact_samples = 0;        // the "fact" chunk's sample count

    // LithTech's 'lith' chunk.
    bool has_lith = false;
    uint32_t lith_flags = 0;          // LITH_* below
    float lith_pitch_mod = 0;         // each play's pitch varies by up to this, either way

    // The engine's loop: the first cue point and its 'ltxt' length, in frames.
    bool has_cue = false;
    uint32_t cue_position = 0, cue_length = 0;
    std::string cue_label;            // its 'labl', if any

    // A "smpl" chunk loop (no game uses one), in frames.
    bool has_loop = false;
    uint32_t loop_start = 0, loop_end = 0;

    std::vector<std::pair<std::string, std::string>> tags;   // LIST INFO: id, text
    std::vector<std::string> chunks;  // every chunk id in order; LIST as "LIST/<form>"

    // MPEG audio, filled in by decode().
    int mpeg_version = 0;             // 1, 2, or 25 for 2.5
    int mpeg_layer = 0;
    int mpeg_mode = -1;               // 0 stereo, 1 joint stereo, 2 dual channel, 3 mono
    int mpeg_mode_ext = 0;            // joint stereo: bit 0 intensity, bit 1 mid/side
    int bitrate_min = 0, bitrate_max = 0;   // kbit/s; they differ for VBR
    uint32_t mpeg_frames = 0;

    // The loop the engine plays, if any, in frames: cue position to
    // position + length.
    bool engine_loop(uint32_t& start, uint32_t& end) const;
};

enum LithFlags : uint32_t { LITH_STREAM = 1u << 0, LITH_DECOMPRESS_ON_LOAD = 1u << 1, LITH_DECOMPRESS_AT_START = 1u << 2 };

struct Pcm {
    int channels = 0;
    int sample_rate = 0;
    std::vector<int16_t> samples;     // interleaved

    std::size_t frames() const { return channels ? samples.size() / std::size_t(channels) : 0; }
    double seconds() const { return sample_rate ? double(frames()) / sample_rate : 0.0; }
};

const char* codec_name(uint16_t format_tag);
std::string tag_name(const std::string& info_id);   // "ISFT" -> "Software"; the id if unknown

// Reads the header and chunk list only.
Info read_info(const uint8_t* data, std::size_t size);

// Decodes the whole sound. Throws Error for a file it can't play.
Pcm decode(const uint8_t* data, std::size_t size, Info* info = nullptr);

// A 16-bit PCM .wav of a sound; with loop_end > loop_start, the loop as
// LithTech's engine reads it (a cue point and its 'ltxt' length) and as a
// "smpl" loop for other tools.
std::vector<uint8_t> write_wav(const Pcm& pcm, uint32_t loop_start = 0, uint32_t loop_end = 0);

// ---------------------------------------------------------------------------
// The PS2 port's sound banks (*.lps, "LithTech PSX Sound Data 1.20"): the
// per-level effect banks (lps/<world>.lps) and voice banks
// (stream/mNN/vo_<world>.lps). Samples are mono PS-ADPCM (SPU "VAG"
// frames). See docs/LPS.md.

struct BankSound {
    std::string name;                 // "LOCKPICK.WAV"
    std::string group;                // its group path, "SND\DOORS"
    std::string path;                 // group + name: the PC path
    uint32_t flags = 0;               // bit 1: looping
    uint32_t offset = 0, size = 0;    // the ADPCM, in the bank's data
    uint32_t rate = 0;
    uint32_t loop_start = 0;          // looping: byte offset of the loop-start frame
    uint32_t play_size = 0;
    uint32_t lip_start = 0, lip_count = 0;   // lip-sync bytes (20 a second)
    uint32_t spu_addr = 0, spu_size = 0;     // the resident head in SPU RAM

    bool looping() const { return flags & 2; }
};

struct BankGroup {
    std::string name;
    uint32_t n_children = 0, n_sounds = 0, first_child = 0, first_sound = 0;
};

struct Bank {
    std::string version;              // "LithTech PSX Sound Data 1.20"
    std::vector<BankSound> sounds;
    std::vector<BankGroup> groups;    // the last is the root
    std::vector<uint8_t> data;        // ADPCM
    std::vector<uint8_t> lip;
    std::vector<uint8_t> spu_image;   // voice banks: a ready copy of the sounds' heads

    const uint8_t* adpcm(const BankSound& s) const { return data.data() + s.offset; }
};

bool is_bank(const uint8_t* data, std::size_t size);
Bank read_bank(const uint8_t* data, std::size_t size);   // throws Error

// PS-ADPCM to PCM, as far as the first frame flagged END (for a loop, one
// pass through it); a frame flagged 7 (void) ends it unplayed. loop_start
// and loop_end, if given, get the loop in samples, as the SPU plays it:
// from the last LOOP START frame (or the start) to the END+LOOP frame's
// end, or to the end of a loop that has no END frame; both 0 for a
// one-shot.
std::vector<int16_t> decode_ps_adpcm(const uint8_t* data, std::size_t size, uint32_t* loop_start = nullptr,
                                     uint32_t* loop_end = nullptr);

// One sound of a bank, mono at its rate.
Pcm decode_bank_sound(const Bank& bank, std::size_t index, uint32_t* loop_start = nullptr,
                      uint32_t* loop_end = nullptr);

}  // namespace lithtech::sound
