#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Low level building blocks that mirror the original Python implementation:
//  - XOR obfuscation with a single byte key (11)
//  - Base64 using the GD-style alphabet ('-' and '_' instead of '+' and '/')
//  - Raw deflate/inflate (no zlib/gzip header), since the actual gzip framing
//    is built/stripped manually to match the save file format exactly.
namespace crypto {

std::vector<uint8_t> xorBytes(const std::vector<uint8_t>& data, uint8_t value);

std::string base64EncodeUrlSafe(const std::vector<uint8_t>& data);
std::vector<uint8_t> base64DecodeUrlSafe(const std::string& data);

// Raw deflate (windowBits = -15), i.e. no zlib/gzip header or trailer.
bool rawInflate(const std::vector<uint8_t>& input, std::vector<uint8_t>& output);
bool rawDeflate(const std::vector<uint8_t>& input, std::vector<uint8_t>& output);

}
