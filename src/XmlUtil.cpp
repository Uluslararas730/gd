#include "XmlUtil.hpp"
#include <cctype>
#include <cstring>

namespace xmlutil {

namespace {

std::string trim(const std::string& str) {
    size_t a = 0, b = str.size();
    while (a < b && std::isspace(static_cast<unsigned char>(str[a]))) a++;
    while (b > a && std::isspace(static_cast<unsigned char>(str[b - 1]))) b--;
    return str.substr(a, b - a);
}

struct Parser {
    const std::string& s;
    size_t i = 0;
    bool ok = true;

    explicit Parser(const std::string& str) : s(str) {}

    void skipWs() {
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) i++;
    }

    bool startsWith(const char* p) const {
        size_t len = std::strlen(p);
        return s.compare(i, len, p) == 0;
    }

    // Skips XML declarations, comments and doctype-like directives.
    void skipMeta() {
        for (;;) {
            skipWs();
            if (startsWith("<?")) {
                size_t end = s.find("?>", i);
                if (end == std::string::npos) { ok = false; return; }
                i = end + 2;
            } else if (startsWith("<!--")) {
                size_t end = s.find("-->", i);
                if (end == std::string::npos) { ok = false; return; }
                i = end + 3;
            } else if (startsWith("<!")) {
                size_t end = s.find(">", i);
                if (end == std::string::npos) { ok = false; return; }
                i = end + 1;
            } else {
                break;
            }
        }
    }

    bool parseElement(XmlNode& node) {
        if (i >= s.size() || s[i] != '<') { ok = false; return false; }
        i++; // consume '<'

        size_t nameStart = i;
        while (i < s.size() && !std::isspace(static_cast<unsigned char>(s[i])) &&
               s[i] != '>' && s[i] != '/') {
            i++;
        }
        node.tag = s.substr(nameStart, i - nameStart);
        if (node.tag.empty()) { ok = false; return false; }

        size_t attrStart = i;
        bool inQuote = false;
        char quoteChar = 0;
        while (i < s.size()) {
            char c = s[i];
            if (inQuote) {
                if (c == quoteChar) inQuote = false;
                i++;
                continue;
            }
            if (c == '"' || c == '\'') { inQuote = true; quoteChar = c; i++; continue; }
            if (c == '>') break;
            i++;
        }
        if (i >= s.size()) { ok = false; return false; }

        std::string rawAttrs = s.substr(attrStart, i - attrStart);
        i++; // consume '>'

        size_t end = rawAttrs.size();
        while (end > 0 && std::isspace(static_cast<unsigned char>(rawAttrs[end - 1]))) end--;
        if (end > 0 && rawAttrs[end - 1] == '/') {
            node.selfClosing = true;
            rawAttrs = rawAttrs.substr(0, end - 1);
            size_t e2 = rawAttrs.size();
            while (e2 > 0 && std::isspace(static_cast<unsigned char>(rawAttrs[e2 - 1]))) e2--;
            rawAttrs = rawAttrs.substr(0, e2);
        }
        node.attrs = rawAttrs;

        if (node.selfClosing) return true;

        std::string textBuf;
        for (;;) {
            if (i >= s.size()) { ok = false; return false; }

            if (s[i] == '<') {
                if (startsWith("</")) {
                    size_t closeStart = i + 2;
                    size_t closeEnd = s.find('>', closeStart);
                    if (closeEnd == std::string::npos) { ok = false; return false; }
                    i = closeEnd + 1;
                    if (node.children.empty()) node.text = trim(textBuf);
                    return true;
                } else if (startsWith("<!--")) {
                    size_t end2 = s.find("-->", i);
                    if (end2 == std::string::npos) { ok = false; return false; }
                    i = end2 + 3;
                } else {
                    XmlNode child;
                    if (!parseElement(child)) return false;
                    node.children.push_back(std::move(child));
                }
            } else {
                size_t textStart = i;
                while (i < s.size() && s[i] != '<') i++;
                textBuf += s.substr(textStart, i - textStart);
            }
        }
    }
};

void serializeNodePretty(const XmlNode& node, std::string& out, int depth) {
    std::string indent(depth, '\t');
    out += indent;
    out += '<';
    out += node.tag;
    out += node.attrs;
    if (node.selfClosing) {
        out += "/>";
        return;
    }
    out += '>';
    if (!node.children.empty()) {
        for (auto& c : node.children) {
            out += '\n';
            serializeNodePretty(c, out, depth + 1);
        }
        out += '\n';
        out += indent;
    } else {
        out += node.text;
    }
    out += "</";
    out += node.tag;
    out += '>';
}

void serializeNodeCompact(const XmlNode& node, std::string& out) {
    out += '<';
    out += node.tag;
    out += node.attrs;
    if (node.selfClosing) {
        out += "/>";
        return;
    }
    out += '>';
    if (!node.children.empty()) {
        for (auto& c : node.children) serializeNodeCompact(c, out);
    } else {
        out += node.text;
    }
    out += "</";
    out += node.tag;
    out += '>';
}

} // namespace

bool parse(const std::string& xml, std::vector<XmlNode>& outRoots) {
    outRoots.clear();
    Parser p(xml);
    p.skipMeta();
    while (p.ok) {
        p.skipWs();
        if (p.i >= xml.size()) break;
        if (xml[p.i] != '<') { p.ok = false; break; }
        XmlNode node;
        if (!p.parseElement(node)) { p.ok = false; break; }
        outRoots.push_back(std::move(node));
        p.skipMeta();
    }
    return p.ok && !outRoots.empty();
}

std::string serializePretty(const std::vector<XmlNode>& roots) {
    std::string out;
    for (size_t i = 0; i < roots.size(); i++) {
        if (i > 0) out += '\n';
        serializeNodePretty(roots[i], out, 0);
    }
    return out;
}

std::string serializeCompact(const std::vector<XmlNode>& roots) {
    std::string out;
    for (auto& r : roots) serializeNodeCompact(r, out);
    return out;
}

}
