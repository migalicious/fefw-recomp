// SPDX-License-Identifier: GPL-3.0-or-later
// Texture cache hashing. Keep the scalar algorithm and existing cache keys stable.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace nxrt::nvn::be::texture_hash {
constexpr uint64_t kP1 = 0x9E3779B185EBCA87ull, kP2 = 0xC2B2AE3D27D4EB4Full, kP3 = 0x165667B19E3779F9ull,
                   kP4 = 0x85EBCA77C2B2AE63ull, kP5 = 0x27D4EB2F165667C5ull;

inline uint64_t rotl64(uint64_t x, int r) { return (x << r) | (x >> (64 - r)); }
inline uint64_t read64(const uint8_t* p) { uint64_t v; std::memcpy(&v, p, 8); return v; }
inline uint32_t read32(const uint8_t* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }
inline uint64_t lane(uint64_t acc, uint64_t in) { acc += in * kP2; acc = rotl64(acc, 31); return acc * kP1; }
inline uint64_t merge_lane(uint64_t h, uint64_t v) { h ^= lane(0, v); return h * kP1 + kP4; }

inline uint64_t hash_seeded(const uint8_t* p, size_t n, uint64_t seed) {
    const uint8_t* const end = p + n;
    uint64_t h;
    if (n >= 32) {
        uint64_t v1 = seed + kP1 + kP2, v2 = seed + kP2, v3 = seed, v4 = seed - kP1;
        do {
            v1 = lane(v1, read64(p));
            v2 = lane(v2, read64(p + 8));
            v3 = lane(v3, read64(p + 16));
            v4 = lane(v4, read64(p + 24));
            p += 32;
        } while (end - p >= 32);
        h = rotl64(v1, 1) + rotl64(v2, 7) + rotl64(v3, 12) + rotl64(v4, 18);
        h = merge_lane(h, v1);
        h = merge_lane(h, v2);
        h = merge_lane(h, v3);
        h = merge_lane(h, v4);
    } else {
        h = seed + kP5;
    }
    h += n;
    while (end - p >= 8) {
        h ^= lane(0, read64(p));
        h = rotl64(h, 27) * kP1 + kP4;
        p += 8;
    }
    if (end - p >= 4) {
        h ^= static_cast<uint64_t>(read32(p)) * kP1;
        h = rotl64(h, 23) * kP2 + kP3;
        p += 4;
    }
    while (p < end) {
        h ^= static_cast<uint64_t>(*p) * kP5;
        h = rotl64(h, 11) * kP1;
        ++p;
    }
    h ^= h >> 33;
    h *= kP2;
    h ^= h >> 29;
    h *= kP3;
    h ^= h >> 32;
    return h;
}


// Two independent seeds, one traversal of the source. Existing cache keys stay
// byte identical; no texture-generation or upload-validity checks are removed.
inline std::array<uint64_t, 2> hash_pair(const uint8_t* p, size_t n, std::array<uint64_t, 2> seeds) {
    const uint8_t* end = p + n;
    std::array<uint64_t, 2> h;
    if (n >= 32) {
        std::array<uint64_t, 2> v1, v2, v3, v4;
        for (size_t i = 0; i < 2; ++i) {
            v1[i] = seeds[i] + kP1 + kP2; v2[i] = seeds[i] + kP2;
            v3[i] = seeds[i]; v4[i] = seeds[i] - kP1;
        }
        do {
            const auto a = read64(p), b = read64(p+8), c = read64(p+16), d = read64(p+24);
            for (size_t i = 0; i < 2; ++i) {
                v1[i] = lane(v1[i], a); v2[i] = lane(v2[i], b);
                v3[i] = lane(v3[i], c); v4[i] = lane(v4[i], d);
            }
            p += 32;
        } while (end - p >= 32);
        for (size_t i = 0; i < 2; ++i) {
            h[i] = rotl64(v1[i], 1) + rotl64(v2[i], 7) + rotl64(v3[i], 12) + rotl64(v4[i], 18);
            h[i] = merge_lane(h[i], v1[i]); h[i] = merge_lane(h[i], v2[i]);
            h[i] = merge_lane(h[i], v3[i]); h[i] = merge_lane(h[i], v4[i]);
        }
    } else for (size_t i = 0; i < 2; ++i) h[i] = seeds[i] + kP5;
    for (auto& x : h) x += n;
    while (end - p >= 8) {
        const auto v = read64(p);
        for (auto& x : h) { x ^= lane(0, v); x = rotl64(x, 27) * kP1 + kP4; }
        p += 8;
    }
    if (end - p >= 4) {
        const auto v = read32(p);
        for (auto& x : h) { x ^= uint64_t(v) * kP1; x = rotl64(x, 23) * kP2 + kP3; }
        p += 4;
    }
    while (p < end) {
        const auto v = *p++;
        for (auto& x : h) { x ^= uint64_t(v) * kP5; x = rotl64(x, 11) * kP1; }
    }
    for (auto& x : h) { x ^= x >> 33; x *= kP2; x ^= x >> 29; x *= kP3; x ^= x >> 32; }
    return h;
}
} // namespace nxrt::nvn::be::texture_hash
