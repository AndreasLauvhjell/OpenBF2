// Pathfinding material properties configured by console commands.

#include "Material.h"

#include "../../../../../Core/Debug.h"
#include "../../../../../Core/StringUtil.h"

#include <istream>
#include <string>

namespace dice {
namespace hfe {
namespace ai {

std::istream& operator>>(std::istream& stream, materialId& material)
{
    std::string token;
    stream >> token;

    if (equalsIgnoreCase(token, "Invalid"))
        material = 0;
    else if (equalsIgnoreCase(token, "Ground"))
        material = 1;
    else if (equalsIgnoreCase(token, "Road"))
        material = 2;
    else if (equalsIgnoreCase(token, "Shallows"))
        material = 3;
    else if (equalsIgnoreCase(token, "DeepWater"))
        material = 4;
    else if (equalsIgnoreCase(token, "DangerObject"))
        material = 5;
    else if (equalsIgnoreCase(token, "OtherDangerObject"))
        material = 6;
    else if (equalsIgnoreCase(token, "BothDangerObject"))
        material = 7;
    else if (equalsIgnoreCase(token, "NumberOfMaterials"))
        material = 9;
    else
    {
        // Both ports report this exact source line and abort through DebugTypeError. There is no
        // fallback assignment for an unknown token.
        Debug(DebugTypeError, __FILE__, __LINE__, "")
            << token << " is not recognised as a proper materialId.";
    }

    return stream;
}

} // namespace ai
} // namespace hfe
} // namespace dice
