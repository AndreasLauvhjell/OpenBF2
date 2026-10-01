// Code/Tools/BF2Editor/Framework/UndergrowthMap.cpp - see UndergrowthMap.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/UndergrowthMap.h"

#include <cmath>
#include <cstddef>
#include <cstdio>

namespace bf2editor
{

bool UndergrowthMap::load(const std::string& bytes, std::string& error)
{
    const int side = static_cast<int>(std::floor(std::sqrt(static_cast<double>(bytes.size())) + 0.5));
    if (side <= 0 || static_cast<size_t>(side) * static_cast<size_t>(side) != bytes.size())
    {
        char text[96];
        std::snprintf(text, sizeof(text), "%u bytes is not a square density map", static_cast<unsigned>(bytes.size()));
        error = text;
        return false;
    }
    m_bytes = bytes;
    m_size = side;
    return true;
}

unsigned char UndergrowthMap::at(int column, int row) const
{
    if (column < 0 || row < 0 || column >= m_size || row >= m_size)
        return 0;
    return static_cast<unsigned char>(m_bytes[static_cast<size_t>(row) * m_size + column]);
}

bool UndergrowthMap::cellAt(float x, float z, float scale, int& column, int& row) const
{
    if (m_size <= 0 || !(scale > 0.0f))
        return false;
    column = static_cast<int>(std::floor(x / scale)) + m_size / 2;
    row = static_cast<int>(std::floor(z / scale)) + m_size / 2;
    return column >= 0 && row >= 0 && column < m_size && row < m_size;
}

void UndergrowthMap::cellPosition(int column, int row, float scale, float& x, float& z) const
{
    x = static_cast<float>(column - m_size / 2) * scale;
    z = static_cast<float>(row - m_size / 2) * scale;
}

bool UndergrowthMap::paintDisc(float x, float z, float radius, float scale, unsigned char value)
{
    if (m_size <= 0 || !(scale > 0.0f) || radius < 0.0f)
        return false;
    int column0 = static_cast<int>(std::floor((x - radius) / scale)) + m_size / 2;
    int column1 = static_cast<int>(std::floor((x + radius) / scale)) + m_size / 2;
    int row0 = static_cast<int>(std::floor((z - radius) / scale)) + m_size / 2;
    int row1 = static_cast<int>(std::floor((z + radius) / scale)) + m_size / 2;
    column0 = column0 < 0 ? 0 : column0;
    row0 = row0 < 0 ? 0 : row0;
    column1 = column1 >= m_size ? m_size - 1 : column1;
    row1 = row1 >= m_size ? m_size - 1 : row1;
    bool changed = false;
    for (int row = row0; row <= row1; ++row)
    {
        for (int column = column0; column <= column1; ++column)
        {
            float cx = 0.0f, cz = 0.0f;
            cellPosition(column, row, scale, cx, cz);
            const float dx = cx + scale * 0.5f - x, dz = cz + scale * 0.5f - z;
            if (dx * dx + dz * dz > radius * radius)
                continue;
            char& cell = m_bytes[static_cast<size_t>(row) * m_size + column];
            if (static_cast<unsigned char>(cell) != value)
            {
                cell = static_cast<char>(value);
                changed = true;
            }
        }
    }
    return changed;
}

UndergrowthChange UndergrowthMap::changeSince(const std::string& before) const
{
    UndergrowthChange change;
    if (before.size() != m_bytes.size() || m_size <= 0)
        return change;
    int column0 = m_size, row0 = m_size, column1 = -1, row1 = -1;
    for (int row = 0; row < m_size; ++row)
    {
        const size_t offset = static_cast<size_t>(row) * m_size;
        if (before.compare(offset, m_size, m_bytes, offset, m_size) == 0)
            continue;
        for (int column = 0; column < m_size; ++column)
        {
            if (before[offset + column] == m_bytes[offset + column])
                continue;
            column0 = column < column0 ? column : column0;
            column1 = column > column1 ? column : column1;
            row0 = row < row0 ? row : row0;
            row1 = row > row1 ? row : row1;
        }
    }
    if (column1 < 0)
        return change;
    change.column = column0;
    change.row = row0;
    change.columns = column1 - column0 + 1;
    change.rows = row1 - row0 + 1;
    for (int row = row0; row <= row1; ++row)
    {
        const size_t offset = static_cast<size_t>(row) * m_size + column0;
        change.before.insert(change.before.end(), before.begin() + static_cast<std::ptrdiff_t>(offset),
                             before.begin() + static_cast<std::ptrdiff_t>(offset + change.columns));
        change.after.insert(change.after.end(), m_bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                            m_bytes.begin() + static_cast<std::ptrdiff_t>(offset + change.columns));
    }
    return change;
}

void UndergrowthMap::apply(const UndergrowthChange& change, bool after)
{
    const std::vector<unsigned char>& data = after ? change.after : change.before;
    if (change.empty() || data.size() != static_cast<size_t>(change.columns) * change.rows
        || change.column + change.columns > m_size || change.row + change.rows > m_size)
        return;
    for (int r = 0; r < change.rows; ++r)
        for (int c = 0; c < change.columns; ++c)
            m_bytes[static_cast<size_t>(change.row + r) * m_size + change.column + c] =
                static_cast<char>(data[static_cast<size_t>(r) * change.columns + c]);
}

std::vector<int> UndergrowthMap::counts() const
{
    std::vector<int> n(256, 0);
    for (size_t i = 0; i < m_bytes.size(); ++i)
        ++n[static_cast<unsigned char>(m_bytes[i])];
    return n;
}

} // namespace bf2editor
