#include "Crypto.hpp"
#include <zlib.h>

namespace crypto {

std::vector<uint8_t> xorBytes(const std::vector<uint8_t>& data, uint8_t value) {
    std::vector<uint8_t> out(data.size());
    for (size_t i = 0; i < data.size(); i++) {
        out[i] = data[i] ^ value;
    }
    return out;
}

namespace {
    constexpr char kB64Chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789-_";

    int b64Val(unsigned char c) {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '-' || c == '+') return 62;
        if (c == '_' || c == '/') return 63;
        return -1;
    }
}

std::string base64EncodeUrlSafe(const std::vector<uint8_t>& data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);

    size_t i = 0;
    const size_t n = data.size();
    while (i + 3 <= n) {
        uint32_t chunk = (uint32_t(data[i]) << 16) | (uint32_t(data[i + 1]) << 8) | data[i + 2];
        out += kB64Chars[(chunk >> 18) & 0x3F];
        out += kB64Chars[(chunk >> 12) & 0x3F];
        out += kB64Chars[(chunk >> 6) & 0x3F];
        out += kB64Chars[chunk & 0x3F];
        i += 3;
    }

    const size_t rem = n - i;
    if (rem == 1) {
        uint32_t chunk = uint32_t(data[i]) << 16;
        out += kB64Chars[(chunk >> 18) & 0x3F];
        out += kB64Chars[(chunk >> 12) & 0x3F];
        out += '=';
        out += '=';
    } else if (rem == 2) {
        uint32_t chunk = (uint32_t(data[i]) << 16) | (uint32_t(data[i + 1]) << 8);
        out += kB64Chars[(chunk >> 18) & 0x3F];
        out += kB64Chars[(chunk >> 12) & 0x3F];
        out += kB64Chars[(chunk >> 6) & 0x3F];
        out += '=';
    }

    return out;
}

std::vector<uint8_t> base64DecodeUrlSafe(const std::string& data) {
    std::vector<uint8_t> out;
    out.reserve((data.size() / 4) * 3 + 3);

    int vals[4];
    int count = 0;

    for (unsigned char c : data) {
        if (c == '=' || c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
        int v = b64Val(c);
        if (v < 0) continue;
        vals[count++] = v;
        if (count == 4) {
            uint32_t chunk = (uint32_t(vals[0]) << 18) | (uint32_t(vals[1]) << 12) |
                              (uint32_t(vals[2]) << 6) | uint32_t(vals[3]);
            out.push_back(uint8_t((chunk >> 16) & 0xFF));
            out.push_back(uint8_t((chunk >> 8) & 0xFF));
            out.push_back(uint8_t(chunk & 0xFF));
            count = 0;
        }
    }

    if (count == 2) {
        uint32_t chunk = (uint32_t(vals[0]) << 18) | (uint32_t(vals[1]) << 12);
        out.push_back(uint8_t((chunk >> 16) & 0xFF));
    } else if (count == 3) {
        uint32_t chunk = (uint32_t(vals[0]) << 18) | (uint32_t(vals[1]) << 12) | (uint32_t(vals[2]) << 6);
        out.push_back(uint8_t((chunk >> 16) & 0xFF));
        out.push_back(uint8_t((chunk >> 8) & 0xFF));
    }

    return out;
}

bool rawInflate(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
    output.clear();
    if (input.empty()) return false;

    z_stream strm{};
    if (inflateInit2(&strm, -MAX_WBITS) != Z_OK) return false;

    strm.next_in = const_cast<Bytef*>(input.data());
    strm.avail_in = static_cast<uInt>(input.size());

    std::vector<uint8_t> buffer(1 << 16);
    int ret;
    do {
        strm.next_out = buffer.data();
        strm.avail_out = static_cast<uInt>(buffer.size());
        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END && ret != Z_BUF_ERROR) {
            inflateEnd(&strm);
            return false;
        }
        size_t produced = buffer.size() - strm.avail_out;
        output.insert(output.end(), buffer.begin(), buffer.begin() + produced);
        if (ret == Z_BUF_ERROR && strm.avail_in == 0) break;
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);
    return !output.empty() || input.empty();
}

bool rawDeflate(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
    output.clear();

    z_stream strm{};
    if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return false;
    }

    strm.next_in = const_cast<Bytef*>(input.data());
    strm.avail_in = static_cast<uInt>(input.size());

    std::vector<uint8_t> buffer(1 << 16);
    int ret;
    do {
        strm.next_out = buffer.data();
        strm.avail_out = static_cast<uInt>(buffer.size());
        ret = deflate(&strm, Z_FINISH);
        if (ret == Z_STREAM_ERROR) {
            deflateEnd(&strm);
            return false;
        }
        size_t produced = buffer.size() - strm.avail_out;
        output.insert(output.end(), buffer.begin(), buffer.begin() + produced);
    } while (ret != Z_STREAM_END);

    deflateEnd(&strm);
    return true;
}

}
