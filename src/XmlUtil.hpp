#pragma once
#include <string>
#include <vector>

// A small, forgiving XML parser used only to reformat GD's plist-style save
// XML: strip stray whitespace and either pretty-print with indentation, or
// compact it back down before re-compressing. It intentionally does not
// handle every XML feature (CDATA, entity decoding, mixed text+element
// content) since GD's own save files never use them - this mirrors the
// scope of the original Python script, which used a plain
// xml.etree.ElementTree round trip for the same purpose.
namespace xmlutil {

struct XmlNode {
    std::string tag;
    std::string attrs;        // raw attribute text, preserved verbatim
    std::string text;         // text content, only meaningful if children is empty
    std::vector<XmlNode> children;
    bool selfClosing = false;
};

// Parses top-level element(s) in `xml`. Returns false on malformed input.
bool parse(const std::string& xml, std::vector<XmlNode>& outRoots);

// Serializes with tab indentation before nested elements (leaf text nodes
// stay inline), matching Python's ElementTree.indent() behavior.
std::string serializePretty(const std::vector<XmlNode>& roots);

// Serializes with no added whitespace between tags.
std::string serializeCompact(const std::vector<XmlNode>& roots);

}
