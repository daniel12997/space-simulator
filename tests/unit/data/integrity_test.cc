// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// requirements: REQ-INT-007
//
// Phase-1 §8: SHA-256 integrity test for data/ contents. Reads
// data/SHA256SUMS, computes SHA-256 of each listed file, asserts each
// matches.

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#ifndef APSIS_DATA_DIR
#error "APSIS_DATA_DIR must be defined by the build system"
#endif

namespace {

// SHA-256 implementation (FIPS 180-4). Self-contained so the test does
// not depend on OpenSSL / libcrypto. Phase 1 needs this only for a
// couple of small files; performance is irrelevant.
class Sha256 {
 public:
  Sha256() { reset(); }
  void reset() {
    state_ = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
              0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
    bit_count_ = 0;
    buf_len_ = 0;
  }
  void update(const uint8_t* data, std::size_t n) {
    while (n > 0) {
      const std::size_t take = std::min<std::size_t>(64 - buf_len_, n);
      std::copy(data, data + take, buf_.begin() + static_cast<std::ptrdiff_t>(buf_len_));
      buf_len_ += take;
      data += take;
      n -= take;
      bit_count_ += static_cast<uint64_t>(take) * 8;
      if (buf_len_ == 64) {
        process(buf_.data());
        buf_len_ = 0;
      }
    }
  }
  std::string hex_digest() {
    // Padding.
    const uint64_t bits = bit_count_;
    uint8_t one = 0x80;
    update(&one, 1);
    while (buf_len_ != 56) {
      uint8_t zero = 0;
      update(&zero, 1);
    }
    std::array<uint8_t, 8> len_be{};
    for (int i = 0; i < 8; ++i) {
      len_be[static_cast<std::size_t>(i)] = static_cast<uint8_t>((bits >> (56 - 8 * i)) & 0xff);
    }
    update(len_be.data(), 8);
    std::ostringstream oss;
    for (int i = 0; i < 8; ++i) {
      const uint32_t s = state_[static_cast<std::size_t>(i)];
      oss << std::hex << std::setw(8) << std::setfill('0') << s;
    }
    return oss.str();
  }

 private:
  static constexpr std::array<uint32_t, 64> kK = {
      0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u,
      0x923f82a4u, 0xab1c5ed5u, 0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
      0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u, 0xe49b69c1u, 0xefbe4786u,
      0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
      0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u,
      0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
      0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u, 0xa2bfe8a1u, 0xa81a664bu,
      0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
      0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au,
      0x5b9cca4fu, 0x682e6ff3u, 0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
      0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
  };
  static uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }
  void process(const uint8_t* block) {
    std::array<uint32_t, 64> w{};
    for (int i = 0; i < 16; ++i) {
      const std::size_t idx = static_cast<std::size_t>(i);
      w[idx] = (static_cast<uint32_t>(block[i * 4 + 0]) << 24) |
               (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
               (static_cast<uint32_t>(block[i * 4 + 2]) << 8) |
                static_cast<uint32_t>(block[i * 4 + 3]);
    }
    for (int i = 16; i < 64; ++i) {
      const uint32_t s0 = rotr(w[static_cast<std::size_t>(i - 15)], 7) ^
                          rotr(w[static_cast<std::size_t>(i - 15)], 18) ^
                          (w[static_cast<std::size_t>(i - 15)] >> 3);
      const uint32_t s1 = rotr(w[static_cast<std::size_t>(i - 2)], 17) ^
                          rotr(w[static_cast<std::size_t>(i - 2)], 19) ^
                          (w[static_cast<std::size_t>(i - 2)] >> 10);
      w[static_cast<std::size_t>(i)] = w[static_cast<std::size_t>(i - 16)] + s0 +
                                       w[static_cast<std::size_t>(i - 7)] + s1;
    }
    uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
    uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];
    for (int i = 0; i < 64; ++i) {
      const uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
      const uint32_t ch = (e & f) ^ ((~e) & g);
      const uint32_t t1 = h + S1 + ch + kK[static_cast<std::size_t>(i)] +
                          w[static_cast<std::size_t>(i)];
      const uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
      const uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
      const uint32_t t2 = S0 + mj;
      h = g; g = f; f = e; e = d + t1;
      d = c; c = b; b = a; a = t1 + t2;
    }
    state_[0] += a; state_[1] += b; state_[2] += c; state_[3] += d;
    state_[4] += e; state_[5] += f; state_[6] += g; state_[7] += h;
  }
  std::array<uint32_t, 8> state_{};
  std::array<uint8_t, 64> buf_{};
  std::size_t buf_len_ = 0;
  uint64_t bit_count_ = 0;
};

std::string sha256_file(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  EXPECT_TRUE(in.is_open()) << "Failed to open " << path;
  Sha256 h;
  std::vector<uint8_t> buf(64 * 1024);
  while (in) {
    in.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
    const auto got = static_cast<std::size_t>(in.gcount());
    h.update(buf.data(), got);
  }
  return h.hex_digest();
}

TEST(DataIntegrity, ManifestMatches) {
  const std::string data_dir = APSIS_DATA_DIR;
  std::ifstream sums(data_dir + "/SHA256SUMS");
  ASSERT_TRUE(sums.is_open()) << "Cannot open SHA256SUMS at " << data_dir;
  std::string line;
  int rows = 0;
  while (std::getline(sums, line)) {
    if (line.empty()) continue;
    // Format: "<hash>  <filename>" (sha256sum default with two spaces).
    const auto sep = line.find("  ");
    ASSERT_NE(sep, std::string::npos) << "Malformed line: " << line;
    const std::string expected = line.substr(0, sep);
    const std::string fname    = line.substr(sep + 2);
    const std::string actual = sha256_file(data_dir + "/" + fname);
    EXPECT_EQ(expected, actual) << "SHA mismatch for " << fname;
    ++rows;
  }
  EXPECT_GT(rows, 0) << "No rows read from SHA256SUMS";
}

}  // namespace
