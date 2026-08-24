#pragma once

// Cross-language deterministic random primitive for QUEST_ENGINE_SPEC.md
// appendix B. This module has zero game/runtime dependencies.

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace qe {

using Sha256Digest = std::array<std::uint8_t, 32>;

struct DeterministicRandomSample {
    Sha256Digest digest{};
    std::uint64_t word = 0;
    double unit = 0.0;
};

// Canonical bytes:
//   ASCII "QE-RANDOM-PRF-V1"
//   u64be(seed UTF-8 byte count) || seed bytes
//   u64be(quest-id UTF-8 byte count) || quest-id bytes
//   u64be(site-key UTF-8 byte count) || site-key bytes
// std::string carries the exact UTF-8 bytes; no locale conversion or Unicode
// normalisation is performed.
std::vector<std::uint8_t> frameRandomPrfInput(std::string_view masterSeed,
                                              std::string_view questId,
                                              std::string_view siteKey);

Sha256Digest sha256(std::span<const std::uint8_t> bytes);

// SHA-256(frame), first 8 digest bytes as a big-endian uint64, then the high
// 53 bits divided by 2^53. The resulting binary64 is exactly in [0, 1).
DeterministicRandomSample deterministicRandom(std::string_view masterSeed,
                                              std::string_view questId,
                                              std::string_view siteKey);

// SPEC boundary rule: chance <= 0 is always false; chance >= 1 is always true;
// otherwise the deterministic unit sample is compared with `< chance`.
bool passesChance(double unit, double chance);

std::string hexLower(std::span<const std::uint8_t> bytes);

}  // namespace qe
