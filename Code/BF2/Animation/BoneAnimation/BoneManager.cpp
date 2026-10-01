// Case-insensitive bone-name interning.

#include "BoneManager.h"

#include "../../Core/StringUtil.h"      // compareIgnoreCase - every lookup here uses it

namespace dice {
namespace anim {


// Initializes an empty bone registry and its fallback name.

BoneManager::BoneManager()
    : m_invalidName("Invalid bone name index")
{
}

BoneManager& BoneManager::getInstance()
{
    static BoneManager instance;
    return instance;
}


// Returns the first case-insensitive name match, or appends name and returns its new index.

int BoneManager::addBoneName(const std::string& name)
{
    for (size_t i = 0; i < m_names.size(); ++i)
    {
        if (hfe::compareIgnoreCase(name.c_str(), m_names[i].c_str()) == 0)
            return static_cast<int>(i);
    }

    m_names.push_back(name);
    return static_cast<int>(m_names.size()) - 1;
}

// The same scan without the append. -1 for "not known", which is the value that
// makes getBoneName below dangerous.
int BoneManager::getBoneNameIndex(const std::string& name) const
{
    for (size_t i = 0; i < m_names.size(); ++i)
    {
        if (hfe::compareIgnoreCase(name.c_str(), m_names[i].c_str()) == 0)
            return static_cast<int>(i);
    }
    return -1;
}


// Returns the bone name or the upper-bound error string. The caller must reject negative indices.

const std::string& BoneManager::getBoneName(short index) const
{
    if (index >= static_cast<short>(m_names.size()))
        return m_invalidName;
    return m_names[static_cast<size_t>(index)];      // negative index: as shipped
}


// Retains the empty name walk from builds without debug printing.

void BoneManager::debugInfo() const
{
    for (size_t i = 0; i < m_names.size(); ++i)
    {
        // empty in the shipped binary - see above
    }
}

} // namespace anim
} // namespace dice
