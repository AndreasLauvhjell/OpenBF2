// Path:     [INFERRED] TextureAtlasBuilder/TextureAtlasConfiguration.cpp - no __FILE__ survives in the DLL;
//           the 2006 editor's .tac writer is E9 0x10064890 (same strings, not read line by line).
// Contents: from TextureAtlasBuilder.dll 1.50 (TAB 0x10007460..0x100081BA), every
//           instruction read: the .tac reader and writer, the group/texture bookkeeping, the accessors.

// TextureAtlasBuilder.dll only - see TextureFormat.cpp for the guard.
#if defined(BF2_IMAGE_TEXTUREATLASBUILDER)

#include "TextureAtlasConfiguration.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace textureatlas {

namespace {

// nullsub_2 (0x10007460) - the configuration's default error and warning sink (both, constructor).
void __fastcall ignoreMessage(const std::string& /*message*/)
{
}

// [CHOSEN, not recovered] `in >> value` with VC7.1's failure semantics. The shipped load() reads
// width/height/miplevels/border with MSVCP71's operator>>(int&), which assigns only on success
// (Dinkumware: `if (failbit || out of range) setstate(failbit); else _Val = _Tmp;`). The current STL
// follows C++11 instead and stores 0 when a field does not convert, so `group g border x` would turn
// the default border of 1 into 0 here and not in retail. Same stream operation, the old outcome.
void extractInt(std::istream& in, int& value)
{
    int converted = value;
    in >> converted;
    if (!in.fail())
        value = converted;
}

} // anonymous namespace


// (86 B each) - the two path setters, the same inlined body on +0x04 / +0x20:
// assign, then append '/' unless the path is empty or already ends in '\\' or '/' (the last character
// read through string::at - the `_Xran` test at so a null `directory` faults in the
// assignment, as shipped).

void TextureAtlasConfiguration::setInputPath(const char* directory)
{
    m_inputPath = directory;
    if (m_inputPath.size() != 0)
    {
        const char last = m_inputPath.at(m_inputPath.size() - 1);
        if (last != '\\' && last != '/')
            m_inputPath += '/';
    }
}

void TextureAtlasConfiguration::setOutputPath(const char* directory)
{
    m_outputPath = directory;
    if (m_outputPath.size() != 0)
    {
        const char last = m_outputPath.at(m_outputPath.size() - 1);
        if (last != '\\' && last != '/')
            m_outputPath += '/';
    }
}


// Loads groups and textures from a .tac file, creating a default group when needed.
// Returns false for a missing group name; an unopenable file still returns true.

bool TextureAtlasConfiguration::load(const char* tacPath)
{
    std::ifstream file(tacPath);
    while (file.good())
    {
        char buffer[1024];
        file.getline(buffer, 1023);
        std::istringstream line(buffer);

        std::string token;
        line >> token;
        if (token.size() == 0 || token[0] == '#')
            continue;

        if (token == "group")
        {
            std::string name;
            std::string format;
            int width = 0;
            int height = 0;
            int miplevels = 0;
            int border = 1;

            line >> name;
            for (;;)
            {
                std::string flag;
                line >> flag;
                if (flag.size() == 0 || flag[0] == '#')
                    break;

                if (flag == "width")
                    extractInt(line, width);
                else if (flag == "height")
                    extractInt(line, height);
                else if (flag == "format")
                    line >> format;
                else if (flag == "miplevels")
                    extractInt(line, miplevels);
                else if (flag == "border")
                    extractInt(line, border);
                else
                {
                    m_warning(std::string("Unknown texture flag '") + flag + "'");
                    break;
                }
            }

            if (name.size() == 0 || name[0] == '#')
            {
                m_error(std::string("Expected group name."));
                return false;
            }
            addGroup(name.c_str(), width, height, format.c_str(), miplevels, border);
        }
        else
        {
            std::string path(token);
            bool border = false;
            if (path.size() == 0 || path[0] == '#')
                continue;

            if (m_current == 0)
            {
                m_warning(std::string("No group specified. Using 'default'."));
                addGroup("default", 0, 0, "", 0, 1);
            }

            for (;;)
            {
                std::string flag;
                line >> flag;
                if (flag.size() == 0 || flag[0] == '#')
                    break;

                if (flag == "border")
                    border = true;
                else
                {
                    m_warning(std::string("Unknown texture flag '") + flag + "'");
                    break;
                }
            }
            addTexture(path.c_str(), border);
        }
    }
    return true;
}


// selectGroup(): findGroup (+0x28) against groupsEnd (+0x30); a hit is
// returned, a miss is the warning "Could not set group: <name>" and null.
// IT does NOT select: nothing stores the hit to m_current (+0x4C), so a following addTexture still
// goes to the group addGroup last created. Shipped.

Group* TextureAtlasConfiguration::selectGroup(const char* name)
{
    const GroupIter group = findGroup(name);
    if (group != groupsEnd())
        return &*group;

    m_warning(std::string("Could not set group: ") + name);
    return 0;
}


// save(): the .tac writer. Eleven '#' lines, then per group
//     group <name> width <w> height <h> format <format> miplevels <m> border <b>
// its textures one per line (" border" appended when flagged) and an empty line. Text mode (CRLF);
// the stream state is never tested. Every instruction read; load() reads it back.

void TextureAtlasConfiguration::save(const char* tacPath)
{
    std::ofstream out(tacPath);

    out << "# Texture Atlas Configuration file" << std::endl;
    out << "#" << std::endl;
    out << "#   create new group:" << std::endl;
    out << "#     group <groupname> [width <width>] [height <height>]" << std::endl;
    out << "#           [format <format>] [miplevels <miplevels>]" << std::endl;
    out << "#           [border <border>]" << std::endl;
    out << "#" << std::endl;
    out << "#   add texture to group:" << std::endl;
    out << "#     <path> [<flag>*]" << std::endl;
    out << "#         flag: border" << std::endl;
    out << "#" << std::endl;

    for (std::vector<Group>::iterator group = m_groups.begin(); group != m_groups.end(); ++group)
    {
        out << "group " << group->name << " width " << group->width << " height " << group->height
            << " format " << group->format << " miplevels " << group->miplevels
            << " border " << group->border << std::endl;
        for (std::vector<TextureInfo>::iterator tex = group->textures.begin();
             tex != group->textures.end(); ++tex)
            out << tex->name << (tex->border ? " border" : "") << std::endl;
        out << std::endl;
    }
}


// findGroup(): a linear search of the groups by exact
// name (std::operator==(const string&, const char*)); a miss returns the virtual groupsEnd() (+0x30),
// not the vector's end directly.

GroupIter TextureAtlasConfiguration::findGroup(const char* name)
{
    for (GroupIter group = m_groups.begin(); group != m_groups.end(); ++group)
    {
        if (group->name == name)
            return group;
    }
    return groupsEnd();
}


// Adds a source texture to the current group and returns the stored record, or null on load failure.
// A group must already exist. Bordered textures grow by twice that group's border in each axis.

TextureInfo* TextureAtlasConfiguration::addTexture(const char* path, bool border)
{
    TextureInfo info;
    if (!getTextureInfo(m_inputPath, std::string(path), info))
    {
        m_warning(std::string("Failed to load texture: ") + path);
        return 0;
    }

    std::replace(info.name.begin(), info.name.end(), '\\', '/');
    info.border = border;
    if (border)
    {
        info.rect.w += m_current->border * 2;
        info.rect.h += m_current->border * 2;
    }
    m_current->textures.push_back(info);
    return &m_current->textures.back();
}


// the base constructor: two empty strings and an empty group list, no current
// group, nullsub_2 as both sinks.

TextureAtlasConfiguration::TextureAtlasConfiguration()
    : m_current(0),
      m_error(ignoreMessage),
      m_warning(ignoreMessage)
{
}

// the members only (the groups, then the two paths). The D3D9 leaf adds nothing,
// so its scalar deleting destructor and the base's are one ICF-folded body (0x10008310, slot +0x00 of
// both vftables).
TextureAtlasConfiguration::~TextureAtlasConfiguration()
{
}


// addGroup(): a Group built through the out-of-line constructor
// (0x10006A20, both strings by value) is pushed, and the new back() becomes the current group - and
// the return value.

Group* TextureAtlasConfiguration::addGroup(const char* name, int width, int height,
                                           const char* format, int miplevels, int border)
{
    m_groups.push_back(Group(name, width, height, format, miplevels, border));
    m_current = &m_groups.back();
    return m_current;
}

// (14 B each): c_str() of +0x04 / +0x20.
const char* TextureAtlasConfiguration::getInputPath()
{
    return m_inputPath.c_str();
}

const char* TextureAtlasConfiguration::getOutputPath()
{
    return m_outputPath.c_str();
}

// (13 B each).
void TextureAtlasConfiguration::setErrorCallback(MessageFn callback)
{
    m_error = callback;
}

void TextureAtlasConfiguration::setWarningCallback(MessageFn callback)
{
    m_warning = callback;
}

// (15 B each, hidden return).
GroupIter TextureAtlasConfiguration::groupsBegin()
{
    return m_groups.begin();
}

GroupIter TextureAtlasConfiguration::groupsEnd()
{
    return m_groups.end();
}

// vector::erase (0x100081C0). m_current is not updated: removing the
// current group (or any group before it) leaves it pointing at the wrong record or past the end.
GroupIter TextureAtlasConfiguration::removeGroup(GroupIter group)
{
    return m_groups.erase(group);
}

} // namespace textureatlas

#endif // BF2_IMAGE_TEXTUREATLASBUILDER
