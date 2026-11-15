// SPDX-License-Identifier: GPL-3.0-or-later
#include "../src/gfx/nvn/nvn_texture_hash.h"
#include "test_framework.h"
#include <random>
#include <vector>
NXTEST_GROUP(texture_hash)

NXTEST(paired_hash_preserves_existing_cache_keys) {
    using namespace nxrt::nvn::be::texture_hash;
    std::mt19937_64 rng(0x20261115);
    std::vector<uint8_t> data(8192 + 8);
    for (auto& b : data) b = static_cast<uint8_t>(rng());
    // Stripe/tail boundaries, unaligned pointers and arbitrary metadata seeds.
    for (size_t offset = 0; offset < 8; ++offset) {
        for (size_t n = 0; n <= 8192; n += n < 128 ? 1 : 127) {
            const std::array<uint64_t, 2> seeds{rng(), rng()};
            const auto got = hash_pair(data.data() + offset, n, seeds);
            CHECK_EQ(got[0], hash_seeded(data.data() + offset, n, seeds[0]));
            CHECK_EQ(got[1], hash_seeded(data.data() + offset, n, seeds[1]));
        }
    }
}
