// Edits Undergrowth.raw as one material-id byte per primary-heightmap sample, with zero
// meaning none and rows along Z. World coordinates map through floor(position/scale)
// plus half the grid extent.
#ifndef BF2EDITOR_FRAMEWORK_UNDERGROWTHMAP_H
#define BF2EDITOR_FRAMEWORK_UNDERGROWTHMAP_H

#include "Framework/EditorConfig.h"

#include <string>
#include <vector>

namespace bf2editor
{

// A rectangle of the map before and after an edit.
struct UndergrowthChange
{
    int column  = 0;
    int row     = 0;
    int columns = 0;
    int rows    = 0;
    std::vector<unsigned char> before;
    std::vector<unsigned char> after;
    bool empty() const { return columns == 0 || rows == 0; }
};

class UndergrowthMap
{
public:
    // The map as the file holds it; false with `error` unless it is square.
    bool load(const std::string& bytes, std::string& error);
    const std::string& bytes() const { return m_bytes; }
    int  size() const { return m_size; }
    bool loaded() const { return m_size > 0; }

    unsigned char at(int column, int row) const;
    // The cell under a world position, `scale` metres per sample; false off the map.
    bool cellAt(float x, float z, float scale, int& column, int& row) const;
    // The world position of a cell's corner.
    void cellPosition(int column, int row, float scale, float& x, float& z) const;

    // Every cell whose centre lies within `radius` metres of (x, z) set to `value`; false when nothing
    // changed.
    bool paintDisc(float x, float z, float radius, float scale, unsigned char value);
    // The rectangle that differs from `before` (the map's bytes when a stroke began), before and after.
    UndergrowthChange changeSince(const std::string& before) const;
    // A change's rectangle put back as it was before, or as it is after.
    void apply(const UndergrowthChange& change, bool after);

    // How many cells hold each value.
    std::vector<int> counts() const;

private:
    std::string m_bytes;
    int         m_size = 0;
};

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_UNDERGROWTHMAP_H
