// Unit checks for lithtech::sound that need no game data: write_wav's loop
// reads back as the engine's and as a "smpl" loop; a small PS2 bank built
// here parses (group paths, lip-sync) and its PS-ADPCM decodes, loops
// included; malformed banks throw.
#include <lithtech/sound.hpp>

#include <cstdio>
#include <cstring>

using namespace lithtech;

static int failures = 0;
#define CHECK(c)                                                       \
    do {                                                               \
        if (!(c)) {                                                    \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c);   \
            ++failures;                                                \
        }                                                              \
    } while (0)

static void put32(std::vector<uint8_t>& v, std::size_t at, uint32_t x) {
    for (int k = 0; k < 4; ++k) v[at + std::size_t(k)] = uint8_t(x >> (8 * k));
}

int main() {
    // write_wav round trip, with a loop
    sound::Pcm p;
    p.channels = 1;
    p.sample_rate = 12000;
    for (int i = 0; i < 1000; ++i) p.samples.push_back(int16_t(i * 37 - 18000));
    auto wav = sound::write_wav(p, 100, 900);
    sound::Info info;
    auto back = sound::decode(wav.data(), wav.size(), &info);
    CHECK(back.samples == p.samples);
    CHECK(back.sample_rate == 12000);
    uint32_t a = 0, b = 0;
    CHECK(info.engine_loop(a, b) && a == 100 && b == 900);
    CHECK(info.cue_label == "loop");
    CHECK(info.has_loop && info.loop_start == 100 && info.loop_end == 899);
    auto plain = sound::write_wav(p);
    CHECK(sound::read_info(plain.data(), plain.size()).chunks.size() == 2);

    // A bank: two sounds under root -> "SND" -> "EVENT".
    // Sound 0: one-shot, a zero frame, one frame of nibble 1 (shift 12,
    // filter 0: each sample 1), END, padding. Sound 1: whole loop.
    std::vector<uint8_t> data;
    auto frame = [&](uint8_t hdr, uint8_t flags, uint8_t fill) {
        data.push_back(hdr);
        data.push_back(flags);
        for (int k = 0; k < 14; ++k) data.push_back(fill);
    };
    frame(0, 0, 0);
    frame(0x0c, 1, 0x11);
    frame(0, 0, 0);
    const uint32_t s1_at = uint32_t(data.size());
    frame(0, 0, 0);
    frame(0, 2, 0);
    frame(0x0c, 6, 0x22);   // loop start: each sample 2
    frame(0x0c, 3, 0x22);   // end + loop
    const std::size_t head = 0x34, nsnd = 2, ngrp = 3;
    std::vector<uint8_t> bank(head + nsnd * 72 + ngrp * 48);
    std::memcpy(bank.data(), "LithTech PSX Sound Data 1.20\r\n\x1a", 32);
    const uint32_t lip = 3;
    put32(bank, 32, uint32_t(data.size()));
    put32(bank, 36, ngrp);
    put32(bank, 40, nsnd);
    put32(bank, 44, lip);
    put32(bank, 48, 0);
    auto sound_rec = [&](std::size_t i, const char* name, uint32_t flags, uint32_t off, uint32_t size, uint32_t ls,
                         uint32_t lip_start, uint32_t lip_count) {
        std::size_t r = head + i * 72;
        std::strcpy(reinterpret_cast<char*>(&bank[r]), name);
        put32(bank, r + 32, flags);
        put32(bank, r + 36, off);
        put32(bank, r + 40, size);
        put32(bank, r + 44, 11025);
        put32(bank, r + 48, ls);
        put32(bank, r + 56, lip_start);
        put32(bank, r + 60, lip_count);
    };
    sound_rec(0, "ONE.WAV", 0, 0, s1_at, 0, 0, 3);
    sound_rec(1, "LOOP.WAV", 2, s1_at, uint32_t(data.size()) - s1_at, 32, 0, 0);
    auto group = [&](std::size_t i, const char* name, uint32_t nch, uint32_t ns, uint32_t fc, uint32_t fs) {
        std::size_t r = head + nsnd * 72 + i * 48;
        std::strcpy(reinterpret_cast<char*>(&bank[r]), name);
        put32(bank, r + 32, nch);
        put32(bank, r + 36, ns);
        put32(bank, r + 40, fc);
        put32(bank, r + 44, fs);
    };
    group(0, "EVENT", 0, 2, 0, 0);
    group(1, "SND", 1, 0, 0, 0);
    group(2, "", 1, 0, 1, 0);   // the root
    bank.insert(bank.end(), data.begin(), data.end());
    bank.insert(bank.end(), {0x10, 0x20, 0x30});
    CHECK(sound::is_bank(bank.data(), bank.size()));
    auto bk = sound::read_bank(bank.data(), bank.size());
    CHECK(bk.sounds.size() == 2 && bk.sounds[1].path == "SND\\EVENT\\LOOP.WAV" && bk.sounds[0].group == "SND\\EVENT");
    CHECK(bk.lip.size() == 3 && bk.sounds[0].lip_count == 3);
    uint32_t ls = 9, le = 9;
    auto one = sound::decode_bank_sound(bk, 0, &ls, &le);
    CHECK(one.samples.size() == 56 && one.samples[0] == 0 && one.samples[28] == 1 && one.samples[55] == 1);
    CHECK(ls == 0 && le == 0);
    auto lp = sound::decode_bank_sound(bk, 1, &ls, &le);
    CHECK(lp.samples.size() == 112 && ls == 56 && le == 112 && lp.samples[60] == 2);

    // malformed: a size that doesn't add up, a sound outside the data, a group loop
    auto bad = bank;
    bad.push_back(0);
    bool threw = false;
    try { sound::read_bank(bad.data(), bad.size()); } catch (const sound::Error&) { threw = true; }
    CHECK(threw);
    bad = bank;
    put32(bad, head + 36, 1000);
    threw = false;
    try { sound::read_bank(bad.data(), bad.size()); } catch (const sound::Error&) { threw = true; }
    CHECK(threw);
    bad = bank;
    put32(bad, head + nsnd * 72 + 2 * 48 + 40, 2);   // the root is its own child
    threw = false;
    try { sound::read_bank(bad.data(), bad.size()); } catch (const sound::Error&) { threw = true; }
    CHECK(threw);

    std::printf("%s\n", failures ? "FAILED" : "sound_test passed");
    return failures ? 1 : 0;
}
