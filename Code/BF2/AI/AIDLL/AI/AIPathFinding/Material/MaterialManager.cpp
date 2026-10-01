
#include "MaterialManager.h"

#include "../../AIUtils/checkPointer.h"

namespace dice {
namespace hfe {
namespace ai {

// Initializes the navigation material table and its default traversal costs.
MaterialManager::MaterialManager()
{
    for (unsigned int i = 0; i <= 8u; ++i)
    {
        MaterialContainer* const container = new MaterialContainer();
        checkPointer(container);
        m_materials.push_back(container);
    }

    setMaterialCost(0, -1.0f);
    setMaterialCost(5, 1000.0f);
    setMaterialCost(7, 1000.0f);
}

// Deletes each owned material-cost record.
MaterialManager::~MaterialManager()
{
    for (std::vector<MaterialContainer*>::iterator i = m_materials.begin();
         i != m_materials.end(); ++i)
        delete *i;
}

// A material is walkable when its cost compares >= 0; material zero starts at -1.
// All accessors require a valid material index.
bool MaterialManager::isValidMaterial(unsigned char material) const
{
    return m_materials[material]->m_cost >= 0.0f;
}

void MaterialManager::setMaterialCost(unsigned char material, float cost)
{
    m_materials[material]->m_cost = cost;
}

float MaterialManager::getMaterialCost(unsigned char material) const
{
    return m_materials[material]->m_cost;
}

} // namespace ai
} // namespace hfe
} // namespace dice
