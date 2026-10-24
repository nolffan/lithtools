// WAV decoding to 16-bit PCM. The ADPCM decoders are the reference
// algorithms (IMA's from the IMA ADPCM recommendation, Microsoft's from
// its ACM codec), and MP3 goes through the vendored minimp3. Floats round
// to nearest even, as FFmpeg's do.

#include "lithtech/sound.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_SIMD_CHECK_WARNING
#include "minimp3.h"

namespace lithtech::sound {

namespace {

uint32_t rd32(const uint8_t* p) {
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}
uint16_t rd16(const uint8_t* p) { return uint16_t(p[0] | p[1] << 8); }
int16_t rds16(const uint8_t* p) { return int16_t(rd16(p)); }

int16_t clamp16(int v) { return int16_t(std::clamp(v, -32768, 32767)); }

struct Parsed {
    Info info;
    const uint8_t* fmt = nullptr;
    std::size_t fmt_size = 0;
    const uint8_t* data = nullptr;
    uint32_t cue_name = 0;
};

std::string cstr(const uint8_t* p, std::size_t n) {
    std::size_t e = 0;
    while (e < n && p[e]) ++e;
    while (e > 0 && (p[e - 1] == ' ' || p[e - 1] == '\n' || p[e - 1] == '\r')) --e;
    return std::string(reinterpret_cast<const char*>(p), e);
}

// The chunks from o to end; form is the enclosing LIST's ("" at the top).
void read_chunks(const uint8_t* d, std::size_t o, std::size_t end, const std::string& form, Parsed& p, int depth = 0) {
    Info& i = p.info;
    while (o + 8 <= end) {
        std::string id(reinterpret_cast<const char*>(d + o), 4);
        uint32_t len = rd32(d + o + 4);
        const uint8_t* body = d + o + 8;
        std::size_t avail = std::min<std::size_t>(len, end - o - 8);
        if (id == "LIST" && avail >= 4) {
            std::string sub(reinterpret_cast<const char*>(body), 4);
            if (form.empty()) i.chunks.push_back("LIST/" + sub);
            if (depth < 8) read_chunks(d, o + 12, o + 8 + avail, sub, p, depth + 1);
        } else {
            if (form.empty()) i.chunks.push_back(id);
            if (form.empty() && id == "fmt " && !p.fmt) {
                p.fmt = body;
                p.fmt_size = avail;
            } else if (form.empty() && id == "data" && !p.data) {
                p.data = body;
                i.data_bytes = uint32_t(avail);
                i.data_truncated = avail < len;
            } else if (form.empty() && id == "fact" && avail >= 4) {
                i.has_fact = true;
                i.fact_samples = rd32(body);
            } else if (form.empty() && id == "smpl" && avail >= 36 + 24 && rd32(body + 28) > 0) {
                i.has_loop = true;
                i.loop_start = rd32(body + 36 + 8);
                i.loop_end = rd32(body + 36 + 12);
            } else if (form.empty() && id == "cue " && avail >= 4 + 24 && rd32(body) > 0 &&
                       std::memcmp(body + 4 + 8, "data", 4) == 0) {
                i.has_cue = true;   // the first point, as the engine takes it
                p.cue_name = rd32(body + 4);
                i.cue_position = rd32(body + 4 + 4);
            } else if (form == "info" && id == "lith" && avail >= 8) {
                i.has_lith = true;
                i.lith_flags = rd32(body);
                uint32_t u = rd32(body + 4);
                std::memcpy(&i.lith_pitch_mod, &u, 4);
            } else if (form == "adtl" && id == "ltxt" && avail >= 8 && i.has_cue && rd32(body) == p.cue_name) {
                i.cue_length = rd32(body + 4);
            } else if (form == "adtl" && id == "labl" && avail > 4 && i.has_cue && rd32(body) == p.cue_name) {
                i.cue_label = cstr(body + 4, avail - 4);
            } else if (form == "INFO" && avail > 0) {
                i.tags.emplace_back(id, cstr(body, avail));
            }
        }
        if (avail < len) break;
        o += 8 + std::size_t(len) + (len & 1);
    }
}

Parsed parse(const uint8_t* d, std::size_t n) {
    if (n < 12 || std::memcmp(d, "RIFF", 4) != 0 || std::memcmp(d + 8, "WAVE", 4) != 0)
        throw Error("not a RIFF WAVE file");
    Parsed p;
    read_chunks(d, 12, n, "", p);
    if (!p.fmt || p.fmt_size < 16) throw Error("no format (fmt) chunk");
    if (!p.data) throw Error("no data chunk");
    Info& i = p.info;
    i.format_tag = rd16(p.fmt);
    i.channels = rd16(p.fmt + 2);
    i.sample_rate = int(rd32(p.fmt + 4));
    i.avg_bytes_per_sec = rd32(p.fmt + 8);
    i.block_align = rd16(p.fmt + 12);
    i.bits_per_sample = rd16(p.fmt + 14);
    if (i.format_tag == WAVE_EXTENSIBLE && p.fmt_size >= 26)
        i.format_tag = rd16(p.fmt + 24);   // the subformat GUID's first two bytes
    if (i.channels > 0 && i.format_tag == WAVE_IMA_ADPCM && i.block_align > 4 * i.channels)
        i.samples_per_block = (i.block_align - 4 * i.channels) * 8 / (4 * i.channels) + 1;
    if (i.channels > 0 && i.format_tag == WAVE_MS_ADPCM && i.block_align > 7 * i.channels)
        i.samples_per_block = (i.block_align - 7 * i.channels) * 2 / i.channels + 2;
    return p;
}

const int kImaIndex[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};
const int kImaStep[89] = {
    7,     8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,    25,    28,
    31,    34,    37,    41,    45,    50,    55,    60,    66,    73,    80,    88,    97,    107,   118,
    130,   143,   157,   173,   190,   209,   230,   253,   279,   307,   337,   371,   408,   449,   494,
    544,   598,   658,   724,   796,   876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
    2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845,  8630,
    9493,  10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

struct Ima {
    int pred = 0, index = 0;
    int16_t step(int nib) {
        int s = kImaStep[index];
        int diff = s >> 3;
        if (nib & 4) diff += s;
        if (nib & 2) diff += s >> 1;
        if (nib & 1) diff += s >> 2;
        pred = std::clamp(nib & 8 ? pred - diff : pred + diff, -32768, 32767);
        index = std::clamp(index + kImaIndex[nib], 0, 88);
        return int16_t(pred);
    }
};

// Each block: per channel a 4-byte header (first sample, step index), then
// 4-byte groups of eight nibbles per channel in turn, low nibble first. A
// short last block gives the whole groups it holds.
void decode_ima(const Parsed& p, Pcm& out) {
    const int ch = p.info.channels, ba = p.info.block_align;
    if (ba < 4 * ch) throw Error("IMA ADPCM block too small");
    std::size_t left = p.info.data_bytes;
    const uint8_t* b = p.data;
    std::vector<Ima> st(std::size_t(ch), Ima{});
    while (left >= std::size_t(4 * ch)) {
        std::size_t blk = std::min<std::size_t>(left, std::size_t(ba));
        for (int c = 0; c < ch; ++c) {
            st[std::size_t(c)].pred = rds16(b + 4 * c);
            st[std::size_t(c)].index = std::min<int>(b[4 * c + 2], 88);
            out.samples.push_back(int16_t(st[std::size_t(c)].pred));
        }
        std::size_t groups = (blk - std::size_t(4 * ch)) / std::size_t(4 * ch);
        const uint8_t* q = b + 4 * ch;
        std::size_t base = out.samples.size();
        out.samples.resize(base + groups * 8 * std::size_t(ch));
        for (std::size_t g = 0; g < groups; ++g)
            for (int c = 0; c < ch; ++c, q += 4)
                for (int k = 0; k < 8; ++k) {
                    int nib = (q[k >> 1] >> ((k & 1) * 4)) & 15;
                    out.samples[base + (g * 8 + std::size_t(k)) * std::size_t(ch) + std::size_t(c)] =
                        st[std::size_t(c)].step(nib);
                }
        b += blk;
        left -= blk;
    }
}

const int kMsAdapt[16] = {230, 230, 230, 230, 307, 409, 512, 614, 768, 614, 512, 409, 307, 230, 230, 230};
const int kMsCoef[7][2] = {{256, 0}, {512, -256}, {0, 0}, {192, 64}, {240, 0}, {460, -208}, {392, -232}};

// Each block: per channel a predictor index, then per channel the delta,
// sample 1 and sample 2 (sample 2 plays first); then nibbles, high first,
// alternating channels.
void decode_ms(const Parsed& p, Pcm& out) {
    const int ch = p.info.channels, ba = p.info.block_align;
    if (ba < 7 * ch) throw Error("MS ADPCM block too small");
    std::vector<std::array<int, 2>> coefs;
    if (p.fmt_size >= 22) {
        int n = rd16(p.fmt + 20);
        for (int k = 0; k < n && 22 + 4 * std::size_t(k) + 4 <= p.fmt_size; ++k)
            coefs.push_back({rds16(p.fmt + 22 + 4 * k), rds16(p.fmt + 24 + 4 * k)});
    }
    if (coefs.empty())
        for (auto& c : kMsCoef) coefs.push_back({c[0], c[1]});
    struct St { int c1, c2, delta, s1, s2; };
    std::vector<St> st(static_cast<std::size_t>(ch));
    std::size_t left = p.info.data_bytes;
    const uint8_t* b = p.data;
    while (left >= std::size_t(7 * ch)) {
        std::size_t blk = std::min<std::size_t>(left, std::size_t(ba));
        for (int c = 0; c < ch; ++c) {
            auto& co = coefs[std::min<std::size_t>(b[c], coefs.size() - 1)];
            St& s = st[std::size_t(c)];
            s.c1 = co[0];
            s.c2 = co[1];
            s.delta = rds16(b + ch + 2 * c);
            s.s1 = rds16(b + 3 * ch + 2 * c);
            s.s2 = rds16(b + 5 * ch + 2 * c);
        }
        for (int c = 0; c < ch; ++c) out.samples.push_back(int16_t(st[std::size_t(c)].s2));
        for (int c = 0; c < ch; ++c) out.samples.push_back(int16_t(st[std::size_t(c)].s1));
        int c = 0;
        for (std::size_t k = std::size_t(7 * ch); k < blk; ++k)
            for (int half = 0; half < 2; ++half) {
                int nib = half ? b[k] & 15 : b[k] >> 4;
                St& s = st[std::size_t(c)];
                int pred = (s.s1 * s.c1 + s.s2 * s.c2) / 256 + (nib >= 8 ? nib - 16 : nib) * s.delta;
                int16_t v = clamp16(pred);
                s.s2 = s.s1;
                s.s1 = v;
                s.delta = std::max(16, kMsAdapt[nib] * s.delta / 256);
                out.samples.push_back(v);
                c = (c + 1) % ch;
            }
        b += blk;
        left -= blk;
    }
}

int16_t alaw(uint8_t a) {
    a ^= 0x55;
    int t = (a & 15) << 4, seg = (a & 0x70) >> 4;
    t = seg == 0 ? t + 8 : seg == 1 ? t + 0x108 : (t + 0x108) << (seg - 1);
    return int16_t(a & 0x80 ? t : -t);
}

int16_t mulaw(uint8_t u) {
    u = uint8_t(~u);
    int t = (((u & 15) << 3) + 0x84) << ((u & 0x70) >> 4);
    return int16_t(u & 0x80 ? 0x84 - t : t - 0x84);
}

void decode_mp3(const Parsed& p, Pcm& out, Info& info) {
    mp3dec_t dec;
    mp3dec_init(&dec);
    std::vector<mp3d_sample_t> frame(MINIMP3_MAX_SAMPLES_PER_FRAME);
    // Zeros after the data, so a last frame the file cuts short still
    // decodes (as FFmpeg's does): one sound in NOLF2 and Contract J.A.C.K.
    // ends 24 bytes into its last frame.
    constexpr int kPad = 4096;
    std::vector<uint8_t> buf(p.data, p.data + std::min<uint32_t>(p.info.data_bytes, 0x7fffffff - kPad));
    buf.resize(buf.size() + kPad);
    const uint8_t* q = buf.data();
    int left = int(buf.size()) - kPad;
    out.channels = 0;
    while (left > 0) {
        // minimp3 takes a frame that ends where its input does, so a cut
        // short frame gets exactly its length, the rest zeros.
        int avail = left;
        if (left >= 4 && hdr_valid(q)) {
            int size = hdr_frame_bytes(q, dec.free_format_bytes) + hdr_padding(q);
            if (size > left && size <= left + kPad) avail = size;
        }
        mp3dec_frame_info_t fi;
        int n = mp3dec_decode_frame(&dec, q, avail, frame.data(), &fi);
        if (fi.frame_bytes <= 0) break;   // nothing more it can sync to
        const uint8_t* h = q + fi.frame_offset;
        q += fi.frame_bytes;
        left -= fi.frame_bytes;
        if (n <= 0) continue;
        ++info.mpeg_frames;
        info.bitrate_min = info.bitrate_min ? std::min(info.bitrate_min, fi.bitrate_kbps) : fi.bitrate_kbps;
        info.bitrate_max = std::max(info.bitrate_max, fi.bitrate_kbps);
        if (!out.channels) {
            out.channels = fi.channels;
            out.sample_rate = fi.hz;
            int v = (h[1] >> 3) & 3;
            info.mpeg_version = v == 3 ? 1 : v == 2 ? 2 : 25;
            info.mpeg_layer = fi.layer;
            info.mpeg_mode = h[3] >> 6;
            info.mpeg_mode_ext = (h[3] >> 4) & 3;
        }
        for (int k = 0; k < n; ++k) {
            if (fi.channels == out.channels) {
                for (int c = 0; c < out.channels; ++c) out.samples.push_back(frame[std::size_t(k * fi.channels + c)]);
            } else if (out.channels == 1) {   // a stereo frame in a mono stream
                out.samples.push_back(int16_t((frame[std::size_t(2 * k)] + frame[std::size_t(2 * k + 1)]) / 2));
            } else {                            // a mono frame in a stereo stream
                out.samples.push_back(frame[std::size_t(k)]);
                out.samples.push_back(frame[std::size_t(k)]);
            }
        }
    }
    if (!out.channels) throw Error("no MPEG audio frames in the data");
}

}  // namespace

const char* codec_name(uint16_t tag) {
    switch (tag) {
        case WAVE_PCM: return "PCM";
        case WAVE_MS_ADPCM: return "Microsoft ADPCM";
        case WAVE_IEEE_FLOAT: return "IEEE float";
        case WAVE_ALAW: return "A-law";
        case WAVE_MULAW: return "mu-law";
        case WAVE_IMA_ADPCM: return "IMA ADPCM";
        case WAVE_MPEGLAYER3: return "MPEG layer 3";
    }
    return "unknown";
}

Info read_info(const uint8_t* data, std::size_t size) { return parse(data, size).info; }

Pcm decode(const uint8_t* data, std::size_t size, Info* info) {
    Parsed p = parse(data, size);
    const Info& i = p.info;
    Pcm out;
    out.channels = i.channels;
    out.sample_rate = i.sample_rate;
    if (i.format_tag != WAVE_MPEGLAYER3 && (i.channels < 1 || i.channels > 8 || i.sample_rate <= 0))
        throw Error("bad channel count or sample rate");
    const uint8_t* d = p.data;
    const std::size_t n = i.data_bytes;
    switch (i.format_tag) {
        case WAVE_PCM: {
            int bytes = (i.bits_per_sample + 7) / 8;
            if (bytes < 1 || bytes > 4) throw Error("unsupported PCM sample size");
            std::size_t count = n / std::size_t(bytes);
            count -= count % std::size_t(i.channels);
            out.samples.resize(count);
            for (std::size_t k = 0; k < count; ++k) {
                const uint8_t* s = d + k * std::size_t(bytes);
                // the top 16 bits of each sample; 8-bit samples are unsigned
                out.samples[k] = bytes == 1 ? int16_t((s[0] - 128) * 256) : rds16(s + bytes - 2);
            }
            break;
        }
        case WAVE_IEEE_FLOAT: {
            if (i.bits_per_sample != 32 && i.bits_per_sample != 64) throw Error("unsupported float sample size");
            std::size_t bytes = std::size_t(i.bits_per_sample / 8), count = n / bytes;
            count -= count % std::size_t(i.channels);
            out.samples.resize(count);
            for (std::size_t k = 0; k < count; ++k) {
                double v;
                if (bytes == 4) {
                    uint32_t u = rd32(d + 4 * k);
                    float f;
                    std::memcpy(&f, &u, 4);
                    v = f;
                } else {
                    uint64_t u = uint64_t(rd32(d + 8 * k)) | uint64_t(rd32(d + 8 * k + 4)) << 32;
                    std::memcpy(&v, &u, 8);
                }
                out.samples[k] = std::isfinite(v) ? clamp16(int(std::lrint(std::clamp(v, -2.0, 2.0) * 32768.0))) : 0;
            }
            break;
        }
        case WAVE_ALAW:
        case WAVE_MULAW: {
            std::size_t count = n - n % std::size_t(i.channels);
            out.samples.resize(count);
            for (std::size_t k = 0; k < count; ++k) out.samples[k] = i.format_tag == WAVE_ALAW ? alaw(d[k]) : mulaw(d[k]);
            break;
        }
        case WAVE_IMA_ADPCM:
            if (i.bits_per_sample != 4) throw Error("unsupported IMA ADPCM sample size");
            decode_ima(p, out);
            break;
        case WAVE_MS_ADPCM: decode_ms(p, out); break;
        case WAVE_MPEGLAYER3: decode_mp3(p, out, p.info); break;
        default: throw Error("unsupported codec (format tag " + std::to_string(i.format_tag) + ")");
    }
    if (info) *info = p.info;
    return out;
}

bool Info::engine_loop(uint32_t& start, uint32_t& end) const {
    if (!has_cue || !cue_length) return false;
    start = cue_position;
    end = cue_position + cue_length;
    return true;
}

std::string tag_name(const std::string& id) {
    static const std::pair<const char*, const char*> names[] = {
        {"INAM", "Title"}, {"IART", "Artist"}, {"ICMT", "Comment"}, {"ICOP", "Copyright"}, {"ICRD", "Created"},
        {"IENG", "Engineer"}, {"IGNR", "Genre"}, {"ISFT", "Software"}, {"ISRC", "Source"}, {"IPRD", "Product"},
        {"IKEY", "Keywords"}, {"ITCH", "Technician"}, {"ISBJ", "Subject"}};
    for (auto& [k, v] : names)
        if (id == k) return v;
    return id.c_str();
}

}  // namespace lithtech::sound
