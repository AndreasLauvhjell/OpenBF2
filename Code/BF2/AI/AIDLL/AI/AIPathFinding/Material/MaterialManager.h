// Owns navigation material definitions and provides indexed access.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIPATHFINDING_MATERIAL_MATERIALMANAGER_H
#define AI_AIDLL_AI_AIPATHFINDING_MATERIAL_MATERIALMANAGER_H

#include <vector>

namespace dice {
namespace hfe {
namespace ai {

class MaterialManager
{
public:
    // Individually allocated material-cost record containing one float.
    class MaterialContainer
    {
    public:
        MaterialContainer() : m_cost(1.0f) {}

        float m_cost;
    };

    MaterialManager();
    ~MaterialManager();

    // All material indices must be in [0, 9); these accessors perform no bounds check.
    bool isValidMaterial(unsigned char) const;
    void setMaterialCost(unsigned char, float);
    float getMaterialCost(unsigned char) const;

    // `sizeof` will not be 12 here - a libstdc++ 3.x vector is three pointers and MSVC's is
    // larger. Same call as everywhere else in this tree.
    /* +0 */ std::vector<MaterialContainer*> m_materials;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPATHFINDING_MATERIAL_MATERIALMANAGER_H
