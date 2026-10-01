// Owned XML document tree backed by the vendored TinyXML parser.

#include "XMLDocument.h"

#include "../../Core/StringUtil.h"      // compareIgnoreCase - the (const char*, const char*) overload
#include "../../IO/File/FileManager.h"  // io::fileManager - parse() opens through the VFS
#include "../../IO/Stream/IStream.h"    // IStream read/getSize/release

#include "../../External/TinyXML/tinyxml.h"   // third party - see External/readme.md

namespace dice {
namespace hfe {

// XMLNode - 0x82B57E0 (C2) / 0x82B5850 (C1), 100 B.
// Two empty-rep strings and two empty vectors, which is what default construction of all four
// produces; the only statement that is not implicit is the vptr store the compiler emits anyway.
// Nothing is allocated here.
XMLNode::XMLNode()
{
}

// Deletes every owned child and attribute, recursively releasing the parsed tree.
// Child nodes have virtual destructors; attributes are destroyed nonvirtually.
XMLNode::~XMLNode()
{
    for (unsigned int i = 0; i < nodes.size(); ++i)
        if (nodes[i])
            delete nodes[i];                                 // vt +0x04 = D0

    for (unsigned int i = 0; i < attributes.size(); ++i)
        if (attributes[i])
            delete attributes[i];                            // ~XMLAttribute + operator delete
}

// Takes ownership of a non-null child and returns true; null returns false.
// Children must be heap-allocated because the parent deletes them.
bool XMLNode::addNode(XMLNode* node)
{
    if (!node)
        return false;
    nodes.push_back(node);
    return true;
}

bool XMLNode::addAttribute(XMLAttribute* attribute)
{
    if (!attribute)
        return false;
    attributes.push_back(attribute);
    return true;
}

// Updates the first case-insensitive attribute-name match, or inserts a new attribute.
void XMLNode::setAttribute(const std::string& name, const std::string& value)
{
    for (unsigned int i = 0; i < attributes.size(); ++i)
    {
        if (compareIgnoreCase(attributes[i]->name.c_str(), name.c_str()) == 0)
        {
            attributes[i]->value.assign(value);              // 0x82B5E9x - overwrite, then return
            return;
        }
    }

    XMLAttribute* attribute = new XMLAttribute;              // operator new(8u), 0x82B5DFE
    attribute->name.assign(name);
    attribute->value.assign(value);
    attributes.push_back(attribute);
}

// Looks up attributes case-insensitively, returning a copy.
// Absent and explicitly empty attributes both return an empty string.
std::string XMLNode::getAttribute(const std::string& name) const
{
    for (unsigned int i = 0; i < attributes.size(); ++i)
    {
        if (compareIgnoreCase(attributes[i]->name.c_str(), name.c_str()) == 0)
            return attributes[i]->value;                     // 0x82B5D4x
    }
    return std::string("");                                  // the miss path - NOT a sentinel
}

// XMLDocument adds no state beyond XMLNode; base construction and destruction manage its tree.
XMLDocument::XMLDocument()
{
}

XMLDocument::~XMLDocument()
{
}

// Recursively copies elements and attributes into the owned XMLNode tree. Element
// tags become names; text children become values, with the last text child winning.
// Comments, declarations, and other node types are ignored.
void parseNode(XMLNode* out, TiXmlNode* in)
{
    out->name.assign(in->Value());

    if (in->Type() == TiXmlNode::ELEMENT)                    // vt +0x14, element == 1
    {
        TiXmlElement* element = in->ToElement();             // inlined type check at 0x82B60D5
        for (TiXmlAttribute* a = element->FirstAttribute(); a; a = a->Next())
        {
            XMLAttribute* attribute = new XMLAttribute;      // operator new(8u), 0x82B6104
            attribute->name.assign(a->Name());
            attribute->value.assign(a->Value());
            out->addAttribute(attribute);                    // takes ownership
        }
    }

    for (TiXmlNode* child = in->FirstChild(); child; child = child->NextSibling())
    {
        switch (child->Type())
        {
        case TiXmlNode::ELEMENT:                             // 1
            {
                XMLNode* node = new XMLNode;                 // operator new(0x24u), 0x82B61D2
                out->addNode(node);                          // takes ownership before recursing
                parseNode(node, child);
            }
            break;

        case TiXmlNode::TEXT:                                // 4
            out->value.assign(child->Value());               // assign, not append
            break;

        default:                                             // comment / unknown / declaration
            continue;
        }
    }
}

// Parses a mounted file into this node as the first root element. Returns false
// when opening fails or no root exists. The file buffer is terminated and its stream
// released before parsing; additional root elements and parser status are ignored.
bool XMLDocument::parse(const std::string& file)
{
    // Filesystem streams provide INonlinearStream, allowing size queries after this downcast.
    io::INonlinearStream* stream = static_cast<io::INonlinearStream*>(
        io::fileManager->open(file,
                              io::IFileSystem::FILEMODE_READ,            // 2
                              (io::IFileSystem::FileAccess)0,            // [UNRESOLVED] enumerator
                              0));                                       // vt +0x18
    if (!stream)
        return false;

    const int size = stream->getSize();                      // vt +0x38
    unsigned char* buffer = new unsigned char[size + 1];
    memset(buffer, 0, size + 1);
    stream->read(buffer, size);                              // vt +0x10
    stream->release();                                       // vt +0x08 - released before parsing

    TiXmlDocument document;
    document.Parse((const char*)buffer, 0, TIXML_ENCODING_UNKNOWN);   // result deliberately unchecked
    delete[] buffer;

    TiXmlElement* root = document.FirstChildElement();
    if (!root)
        return false;

    parseNode(this, root);
    return true;
}

} // namespace hfe
} // namespace dice
