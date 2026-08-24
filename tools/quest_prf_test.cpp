#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

#include "DeterministicRandom.h"

namespace {
int failures = 0;

void check(bool condition, const std::string& label) {
    if (condition) std::cout << "PASS: " << label << '\n';
    else { std::cerr << "FAIL: " << label << '\n'; ++failures; }
}

std::span<const std::uint8_t> bytesOf(const std::string& text) {
    return {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()};
}
}  // namespace

int main() {
    const std::string abc = "abc";
    check(qe::hexLower(qe::sha256(bytesOf(abc))) ==
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
          "SHA-256 implementation matches the standard abc vector");

    const auto framed1 = qe::frameRandomPrfInput(
        "seed", "quest", "path:/triggers/0/when/random");
    check(qe::hexLower(framed1) ==
              "51452d52414e444f4d2d5052462d5631"
              "000000000000000473656564"
              "00000000000000057175657374"
              "000000000000001c706174683a2f74726967676572732f302f7768656e2f72616e646f6d",
          "golden 1 canonical framed bytes");
    const auto golden1 = qe::deterministicRandom(
        "seed", "quest", "path:/triggers/0/when/random");
    check(qe::hexLower(golden1.digest) ==
              "0fe72c7122d71c586f354eb4be5f23787bc3c4b75e9e722b4144592e282ae7e8",
          "golden 1 SHA-256 digest");
    check(golden1.word == UINT64_C(0x0fe72c7122d71c58),
          "golden 1 first 64 bits are big-endian");
    check(golden1.unit == 0.062121179219357336, "golden 1 binary64 mapping");

    const auto framed2 = qe::frameRandomPrfInput("種子-01", "任務/alpha", "key:門檻");
    check(qe::hexLower(framed2) ==
              "51452d52414e444f4d2d5052462d5631"
              "0000000000000009e7a8aee5ad902d3031"
              "000000000000000ce4bbbbe58b992f616c706861"
              "000000000000000a6b65793ae99680e6aabb",
          "golden 2 canonical UTF-8 framed bytes");
    const auto golden2 = qe::deterministicRandom("種子-01", "任務/alpha", "key:門檻");
    check(qe::hexLower(golden2.digest) ==
              "0569538156f6057cb02ffddb1f895fdec269053b09ca57ba80fa7a71c5d404c4",
          "golden 2 UTF-8 digest");
    check(golden2.word == UINT64_C(0x0569538156f6057c),
          "golden 2 first 64 bits are big-endian");
    check(golden2.unit == 0.0211384001513224, "golden 2 binary64 mapping");

    const auto repeat = qe::deterministicRandom(
        "seed", "quest", "path:/triggers/0/when/random");
    check(repeat.digest == golden1.digest && repeat.word == golden1.word &&
              repeat.unit == golden1.unit,
          "same seed and fields are stable");
    check(qe::deterministicRandom("seed-2", "quest", "path:/triggers/0/when/random").digest != golden1.digest,
          "changing master_seed changes the result");
    check(qe::deterministicRandom("seed", "quest-2", "path:/triggers/0/when/random").digest != golden1.digest,
          "changing quest_id changes the result");
    check(qe::deterministicRandom("seed", "quest", "path:/triggers/1/when/random").digest != golden1.digest,
          "changing site_key changes the result");

    check(!qe::passesChance(golden1.unit, 0.0), "chance 0 is always false");
    check(qe::passesChance(golden1.unit, 1.0), "chance 1 is always true");
    check(!qe::passesChance(golden1.unit, golden1.unit),
          "chance comparison is strict less-than at equality");
    check(qe::passesChance(golden1.unit, std::nextafter(golden1.unit, 1.0)),
          "next representable threshold above the sample passes");
    check(!qe::passesChance(golden1.unit, std::numeric_limits<double>::quiet_NaN()),
          "NaN chance fails safely");
    check(!qe::passesChance(golden1.unit, std::numeric_limits<double>::infinity()),
          "infinite chance fails safely");
    check(!qe::passesChance(golden1.unit, -0.25), "negative chance is false");

    const std::string savedSeed = "save-slot-seed";
    const std::string quest = "quest-after-reload";
    const std::string site = "key:stable-roll";
    const auto beforeReload = qe::deterministicRandom(savedSeed, quest, site);
    const std::string restoredSeed(savedSeed);
    const auto afterReload = qe::deterministicRandom(restoredSeed, quest, site);
    check(beforeReload.digest == afterReload.digest && beforeReload.word == afterReload.word &&
              beforeReload.unit == afterReload.unit,
          "pure-function reconstruction is save/reload state-independent");

    if (failures != 0) { std::cerr << failures << " failure(s)\n"; return 1; }
    std::cout << "all deterministic PRF primitive tests passed\n";
    return 0;
}
