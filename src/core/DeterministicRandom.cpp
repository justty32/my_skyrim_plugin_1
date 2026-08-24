#include "DeterministicRandom.h"

#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace qe {
namespace {

constexpr std::string_view kDomain = "QE-RANDOM-PRF-V1";

constexpr std::array<std::uint32_t, 64> kRoundConstants = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
};

constexpr std::uint32_t rotateRight(std::uint32_t value, unsigned count) {
    return (value >> count) | (value << (32U - count));
}

void appendU64Be(std::vector<std::uint8_t>& out, std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8)
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
}

void appendFramedUtf8(std::vector<std::uint8_t>& out, std::string_view bytes) {
    if (bytes.size() > std::numeric_limits<std::uint64_t>::max())
        throw std::length_error("PRF field is too large for uint64 framing");
    appendU64Be(out, static_cast<std::uint64_t>(bytes.size()));
    out.insert(out.end(), bytes.begin(), bytes.end());
}

}  // namespace

std::vector<std::uint8_t> frameRandomPrfInput(std::string_view masterSeed,
                                              std::string_view questId,
                                              std::string_view siteKey) {
    std::vector<std::uint8_t> out;
    out.reserve(kDomain.size() + 24U + masterSeed.size() + questId.size() + siteKey.size());
    out.insert(out.end(), kDomain.begin(), kDomain.end());
    appendFramedUtf8(out, masterSeed);
    appendFramedUtf8(out, questId);
    appendFramedUtf8(out, siteKey);
    return out;
}

Sha256Digest sha256(std::span<const std::uint8_t> bytes) {
    std::array<std::uint32_t, 8> state = {
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U,
    };

    std::vector<std::uint8_t> padded(bytes.begin(), bytes.end());
    const std::uint64_t bitLength = static_cast<std::uint64_t>(padded.size()) * 8U;
    padded.push_back(0x80U);
    while ((padded.size() % 64U) != 56U) padded.push_back(0U);
    appendU64Be(padded, bitLength);

    for (std::size_t offset = 0; offset < padded.size(); offset += 64U) {
        std::array<std::uint32_t, 64> w{};
        for (std::size_t i = 0; i < 16U; ++i) {
            const std::size_t p = offset + i * 4U;
            w[i] = (static_cast<std::uint32_t>(padded[p]) << 24U) |
                   (static_cast<std::uint32_t>(padded[p + 1U]) << 16U) |
                   (static_cast<std::uint32_t>(padded[p + 2U]) << 8U) |
                   static_cast<std::uint32_t>(padded[p + 3U]);
        }
        for (std::size_t i = 16U; i < 64U; ++i) {
            const std::uint32_t s0 = rotateRight(w[i - 15U], 7U) ^
                                     rotateRight(w[i - 15U], 18U) ^ (w[i - 15U] >> 3U);
            const std::uint32_t s1 = rotateRight(w[i - 2U], 17U) ^
                                     rotateRight(w[i - 2U], 19U) ^ (w[i - 2U] >> 10U);
            w[i] = w[i - 16U] + s0 + w[i - 7U] + s1;
        }

        std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        std::uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
        for (std::size_t i = 0; i < 64U; ++i) {
            const std::uint32_t sum1 = rotateRight(e, 6U) ^ rotateRight(e, 11U) ^
                                       rotateRight(e, 25U);
            const std::uint32_t choose = (e & f) ^ ((~e) & g);
            const std::uint32_t t1 = h + sum1 + choose + kRoundConstants[i] + w[i];
            const std::uint32_t sum0 = rotateRight(a, 2U) ^ rotateRight(a, 13U) ^
                                       rotateRight(a, 22U);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t t2 = sum0 + majority;
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
        state[4] += e; state[5] += f; state[6] += g; state[7] += h;
    }

    Sha256Digest digest{};
    for (std::size_t i = 0; i < state.size(); ++i) {
        digest[i * 4U] = static_cast<std::uint8_t>(state[i] >> 24U);
        digest[i * 4U + 1U] = static_cast<std::uint8_t>(state[i] >> 16U);
        digest[i * 4U + 2U] = static_cast<std::uint8_t>(state[i] >> 8U);
        digest[i * 4U + 3U] = static_cast<std::uint8_t>(state[i]);
    }
    return digest;
}

DeterministicRandomSample deterministicRandom(std::string_view masterSeed,
                                              std::string_view questId,
                                              std::string_view siteKey) {
    DeterministicRandomSample sample;
    const auto framed = frameRandomPrfInput(masterSeed, questId, siteKey);
    sample.digest = sha256(framed);
    for (std::size_t i = 0; i < 8U; ++i)
        sample.word = (sample.word << 8U) | sample.digest[i];
    const std::uint64_t top53 = sample.word >> 11U;
    sample.unit = std::ldexp(static_cast<double>(top53), -53);
    return sample;
}

bool passesChance(double unit, double chance) {
    if (!std::isfinite(chance)) return false;
    if (chance <= 0.0) return false;
    if (chance >= 1.0) return true;
    return unit < chance;
}

std::string hexLower(std::span<const std::uint8_t> bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2U);
    for (std::uint8_t byte : bytes) {
        out.push_back(digits[byte >> 4U]);
        out.push_back(digits[byte & 0x0fU]);
    }
    return out;
}

}  // namespace qe
