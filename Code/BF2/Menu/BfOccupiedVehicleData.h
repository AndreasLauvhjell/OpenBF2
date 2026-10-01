// Descriptor-attested dice::meme::BfOccupiedVehicleData; path [INFERRED].
// Complete28-slot table936D10 and descriptor9369C8.
#ifndef MENU_BFOCCUPIEDVEHICLEDATA_H
#define MENU_BFOCCUPIEDVEHICLEDATA_H
#include "../External/BFMeme/Data.h"
#if !DEDICATED
#include <map>
namespace dice { namespace meme {
class BfOccupiedVehicleData : public Data {
public:
    explicit BfOccupiedVehicleData(const char* name = nullptr);
    ~BfOccupiedVehicleData() override; // 7C8F90, only map/base destruction
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override; // 504E70, native sizeof
    bool streamProperties(System*, Stream*) override;
    static TypeDescriptor* getClassType();
    bool setOccupantIcon(int position, int icon); // 7C8DB0; [INFERRED] name
    int getOccupantTexture(System*, int position); // 7C8CC0; System argument unused
private:
    friend struct BfOccupiedVehicleDataFixture;
    std::map<int, int> m_occupantIcons; // PE+8, signed key and icon index
    // Retail allocation/getSize48 leaves exactly seven handles after its12-byte
    // map. Constructor and getter nevertheless access index7: shipped overflow.
    // Do not enlarge this array or silently repair the allocation.
    int m_textures[7]; // PE+14..2F; native+10..2B
};
static_assert(sizeof(BfOccupiedVehicleData) == 44, "native x86 Data/map/seven handles");
} }
#endif
#endif
