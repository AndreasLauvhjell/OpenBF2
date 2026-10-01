// Code/Tools/BF2Editor/Framework/Selection.cpp - see Selection.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/Selection.h"

#include <algorithm>

namespace bf2editor
{

Selection::Selection()
    : m_revision(0)
{
}

void Selection::clear()
{
    if (m_ids.empty())
        return;
    m_ids.clear();
    ++m_revision;
}

void Selection::set(unsigned id)
{
    if (id == 0)
    {
        clear();
        return;
    }
    if (m_ids.size() == 1 && m_ids[0] == id)
        return;
    m_ids.assign(1, id);
    ++m_revision;
}

void Selection::add(unsigned id)
{
    if (id == 0 || (!m_ids.empty() && m_ids.back() == id))
        return;
    std::vector<unsigned>::iterator found = std::find(m_ids.begin(), m_ids.end(), id);
    if (found != m_ids.end())
        m_ids.erase(found);
    m_ids.push_back(id);
    ++m_revision;
}

void Selection::remove(unsigned id)
{
    std::vector<unsigned>::iterator found = std::find(m_ids.begin(), m_ids.end(), id);
    if (found == m_ids.end())
        return;
    m_ids.erase(found);
    ++m_revision;
}

void Selection::toggle(unsigned id)
{
    if (contains(id))
        remove(id);
    else
        add(id);
}

bool Selection::contains(unsigned id) const
{
    return std::find(m_ids.begin(), m_ids.end(), id) != m_ids.end();
}

} // namespace bf2editor
