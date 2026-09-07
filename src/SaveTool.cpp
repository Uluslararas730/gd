#include "SaveTool.hpp"
#include "Crypto.hpp"
#include "XmlUtil.hpp"

#include <Geode/Geode.hpp>
#include <zlib.h>

#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

namespace {

#if defined(GEODE_IS_ANDROID)
const std::string kGeodeSavePath = "/storage/emulated/0/Android/media/com.geode.launcher/save";
const std::string kDownloadsPath = "/storage/emulated/0/Download";
#else
// Non-Android platforms: fall back to the mod's own save directory so the
// code still compiles/links, even though this tool is designed for Android
// save files.
const std::string kGeodeSavePath = "";
const std::string kDownloadsPath = "";
#endif

std::string datDir() {
#if defined(GEODE_IS_ANDROID)
    std::error_code ec;
    if (!kGeodeSavePath.empty() && fs::exists(kGeodeSavePath, ec)) return kGeodeSavePath;
    return kDownloadsPath;
#else
    return geode::Mod::get()->getSaveDir().string();
#endif
}

std::string xmlDir() {
#if defined(GEODE_IS_ANDROID)
    return kDownloadsPath;
#else
    return geode::Mod::get()->getSaveDir().string();
#endif
}

bool readFile(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.seekg(0, std::ios::end);
    std::streamoff size = f.tellg();
    if (size < 0) return false;
    f.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(size));
    if (size > 0 && !f.read(reinterpret_cast<char*>(out.data()), size)) return false;
    return true;
}

bool writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::error_code ec;
    fs::remove(path, ec);
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    if (!data.empty()) f.write(reinterpret_cast<const char*>(data.data()), data.size());
    return f.good();
}

std::string joinNames(const std::vector<std::string>& names) {
    std::string out;
    for (size_t i = 0; i < names.size(); i++) {
        if (i) out += ", ";
        out += names[i];
    }
    return out;
}

std::string findExisting(const std::string& dir, const std::vector<std::string>& candidates) {
    std::error_code ec;
    for (auto& name : candidates) {
        std::string p = dir + "/" + name;
        if (fs::exists(p, ec)) return p;
    }
    return {};
}

} // namespace

namespace SaveTool {

std::pair<bool, std::string> decrypt(const std::string& baseName, bool prettify) {
    std::vector<std::string> candidates = {
        baseName + ".dat", baseName, baseName + ".xml", baseName + ".dat.xml"
    };

    std::string foundPath = findExisting(datDir(), candidates);
    if (foundPath.empty()) {
        return { false, "Bulunamadi:\n" + joinNames(candidates) };
    }

    std::vector<uint8_t> encrypted;
    if (!readFile(foundPath, encrypted)) {
        return { false, "Hata:\nDosya okunamadi." };
    }

    auto xored = crypto::xorBytes(encrypted, 11);
    std::string b64(xored.begin(), xored.end());
    auto decoded = crypto::base64DecodeUrlSafe(b64);
    if (decoded.size() <= 10) {
        return { false, "Hata:\nGecersiz ya da bos veri." };
    }
    std::vector<uint8_t> body(decoded.begin() + 10, decoded.end());

    std::vector<uint8_t> decompressed;
    if (!crypto::rawInflate(body, decompressed)) {
        return { false, "Hata:\nCozme (inflate) basarisiz." };
    }

    if (prettify) {
        std::string xmlStr(decompressed.begin(), decompressed.end());
        std::vector<xmlutil::XmlNode> roots;
        if (!xmlutil::parse(xmlStr, roots)) {
            return { false, "Hata:\nXML ayristirilamadi." };
        }
        std::string pretty = xmlutil::serializePretty(roots);
        decompressed.assign(pretty.begin(), pretty.end());
    }

    std::string xmlFilename = baseName + ".xml";
    std::string outPath = xmlDir() + "/" + xmlFilename;
    if (!writeFile(outPath, decompressed)) {
        return { false, "Hata:\nDosya yazilamadi." };
    }

    return { true, "Basarili!\n" + xmlFilename + " olusturuldu." };
}

std::pair<bool, std::string> encrypt(const std::string& baseName) {
    std::vector<std::string> candidates = {
        baseName + ".xml", baseName + ".dat.xml", baseName
    };

    std::string foundPath = findExisting(xmlDir(), candidates);
    if (foundPath.empty()) {
        return { false, "Bulunamadi:\n" + joinNames(candidates) };
    }

    std::vector<uint8_t> xmlData;
    if (!readFile(foundPath, xmlData)) {
        return { false, "Hata:\nDosya okunamadi." };
    }

    // Best-effort compaction (mirrors the Python script's silent try/except
    // around its ElementTree round trip): if parsing fails, keep the raw
    // bytes as-is instead of failing the whole operation.
    {
        std::string xmlStr(xmlData.begin(), xmlData.end());
        std::vector<xmlutil::XmlNode> roots;
        if (xmlutil::parse(xmlStr, roots)) {
            std::string compact = xmlutil::serializeCompact(roots);
            xmlData.assign(compact.begin(), compact.end());
        }
    }

    std::vector<uint8_t> rawDeflated;
    if (!crypto::rawDeflate(xmlData, rawDeflated)) {
        return { false, "Hata:\nSikistirma basarisiz." };
    }

    uint32_t crc = static_cast<uint32_t>(
        crc32(0L, xmlData.empty() ? nullptr : xmlData.data(), static_cast<uInt>(xmlData.size()))
    );
    uint32_t rawSize = static_cast<uint32_t>(xmlData.size());

    static const uint8_t kGzipHeader[10] = { 0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b };

    std::vector<uint8_t> gz;
    gz.reserve(10 + rawDeflated.size() + 8);
    gz.insert(gz.end(), kGzipHeader, kGzipHeader + 10);
    gz.insert(gz.end(), rawDeflated.begin(), rawDeflated.end());

    auto pushLE32 = [&gz](uint32_t v) {
        gz.push_back(uint8_t(v & 0xFF));
        gz.push_back(uint8_t((v >> 8) & 0xFF));
        gz.push_back(uint8_t((v >> 16) & 0xFF));
        gz.push_back(uint8_t((v >> 24) & 0xFF));
    };
    pushLE32(crc);
    pushLE32(rawSize);

    std::string encoded = crypto::base64EncodeUrlSafe(gz);
    std::vector<uint8_t> encodedBytes(encoded.begin(), encoded.end());
    auto encrypted = crypto::xorBytes(encodedBytes, 11);

    std::string datFilename = baseName + ".dat";
    std::string outPath = datDir() + "/" + datFilename;
    if (!writeFile(outPath, encrypted)) {
        return { false, "Hata:\nDosya yazilamadi." };
    }

    return { true, "Basarili!\n" + datFilename + " guncellendi." };
}

}
