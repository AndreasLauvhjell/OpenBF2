// Engine XML tree wrappers over TinyXML, used by mod and level descriptor readers.
// XMLDocument adds no state to XMLNode. Nodes own their child and attribute collections.
// [PLATFORM] String and vector layouts depend on the compiler; binary sizes are not portable.

#ifndef GAME_UTILITY_XMLDOCUMENT_H          // [INFERRED] guard style
#define GAME_UTILITY_XMLDOCUMENT_H

#include <string>
#include <vector>

class TiXmlNode;                            // External/TinyXML - parseNode's second parameter

namespace dice {
namespace hfe {

// A name/value pair. No behaviour of its own: the constructor (0x82B56A0, 45 B) default-constructs
// both strings onto the shared empty rep and the destructor (0x82B5700, 100 B) destroys them in
// reverse declaration order - i.e. both are exactly what the compiler emits implicitly, so neither
// is written out. XMLNode owns every instance and deletes them in its own destructor.
class XMLAttribute
{
public:
    std::string name;                       // +0x00  [INFERRED] name
    std::string value;                      // +0x04  [INFERRED] name
};

// One element of the parsed tree. IT owns ITS children AND ITS attributes - see the destructor.
class XMLNode
{
public:
    XMLNode();
    virtual ~XMLNode();
                                                            //    (D0), 292 B

    // Both refuse a null argument with false and otherwise push_back. Returns [S] bool: the ELF
    // leaves 0 in eax on the null path and 1 on the append (0x82B5C76 / 0x82B5C92).
    bool addNode(XMLNode* node);
    bool addAttribute(XMLAttribute* attribute);

    // Set-or-insert by case-insensitive name. Return [CHOSEN] void - the mangled name carries no
    // return type and both exits simply leave whatever the last call put in eax.
    void setAttribute(const std::string& name, const std::string& value);

    // Case-insensitive lookup; returns the empty string when there is no such attribute, not a
    // sentinel and not a reference - the ELF builds a fresh std::string on the miss path.
    std::string getAttribute(const std::string& name) const;

public:
    std::string                 name;                       // +0x04
    std::string                 value;                      // +0x08
    std::vector<XMLNode*>       nodes;                      // +0x0C  owned
    std::vector<XMLAttribute*>  attributes;                 // +0x18  owned
};

// The document root. Adds no state - only a vtable and parse().
class XMLDocument : public XMLNode
{
public:
    XMLDocument();
    virtual ~XMLDocument();

    // Opens through io::fileManager (so archived .desc files resolve), reads
    // the whole file into a NUL-terminated buffer, hands it to TiXmlDocument::Parse, and adopts the
    // first root element into `this` - see the body for why that means the document's own `nodes`
    // are the root's children, not the root itself.
    bool parse(const std::string& file);
};

// Converts a parsed TinyXML tree into the XMLNode tree above; parse()'s
// worker, and recursive. The two libraries' word "value" means different things and the mapping is
// crossed: TiXmlNode::Value() becomes our `name`, and a text child's text becomes our `value`.
void parseNode(XMLNode* out, TiXmlNode* in);

} // namespace hfe
} // namespace dice

#endif // GAME_UTILITY_XMLDOCUMENT_H
