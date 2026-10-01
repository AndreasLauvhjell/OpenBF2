// Texture-format conversion for the separate TextureAtlasBuilder image.
// The image guard keeps these definitions out of server and client executable builds.
#if defined(BF2_IMAGE_TEXTUREATLASBUILDER)

#include "TextureFormat.h"

namespace textureatlas {


// (48 B) - `cmp ecx, DXT3; jg; jz` ... a compare tree over the five FOURCCs, `mov al, 1` /
// `xor al, al`. The tree is the compiler's; the set is DXT1..DXT5 and nothing else.

bool __fastcall isDXT(D3DFORMAT format)
{
    switch (format)
    {
    case D3DFMT_DXT1:
    case D3DFMT_DXT2:
    case D3DFMT_DXT3:
    case D3DFMT_DXT4:
    case D3DFMT_DXT5:
        return true;
    default:
        return false;
    }
}


// a binary decision tree over the values below, each arm a
// `std::string::string(ret, "<name>")`. The case SET is read from the tree; the case order is
// [INFERRED] from the image's string-literal layout (0x1001B178..0x1001B338, reversed) and changes
// nothing. Not covered, as shipped: the depth formats, INDEX16/32, vertexdata, MULTI2_ARGB8, A1.

std::string __fastcall formatToString(D3DFORMAT format)
{
    switch (format)
    {
    case D3DFMT_R8G8B8:          return "R8G8B8";
    case D3DFMT_A8R8G8B8:        return "A8R8G8B8";
    case D3DFMT_X8R8G8B8:        return "X8R8G8B8";
    case D3DFMT_R5G6B5:          return "R5G6B5";
    case D3DFMT_X1R5G5B5:        return "X1R5G5B5";
    case D3DFMT_A1R5G5B5:        return "A1R5G5B5";
    case D3DFMT_A4R4G4B4:        return "A4R4G4B4";
    case D3DFMT_R3G3B2:          return "R3G3B2";
    case D3DFMT_A8:              return "A8";
    case D3DFMT_A8R3G3B2:        return "A8R3G3B2";
    case D3DFMT_X4R4G4B4:        return "X4R4G4B4";
    case D3DFMT_A2B10G10R10:     return "A2B10G10R10";
    case D3DFMT_A8B8G8R8:        return "A8B8G8R8";
    case D3DFMT_X8B8G8R8:        return "X8B8G8R8";
    case D3DFMT_G16R16:          return "G16R16";
    case D3DFMT_A2R10G10B10:     return "A2R10G10B10";
    case D3DFMT_A16B16G16R16:    return "A16B16G16R16";
    case D3DFMT_A8P8:            return "A8P8";
    case D3DFMT_P8:              return "P8";
    case D3DFMT_L8:              return "L8";
    case D3DFMT_A8L8:            return "A8L8";
    case D3DFMT_A4L4:            return "A4L4";
    case D3DFMT_V8U8:            return "V8U8";
    case D3DFMT_Q8W8V8U8:        return "Q8W8V8U8";
    case D3DFMT_V16U16:          return "V16U16";
    case D3DFMT_L6V5U5:          return "L6V5U5";
    case D3DFMT_X8L8V8U8:        return "X8L8V8U8";
    case D3DFMT_A2W10V10U10:     return "A2W10V10U10";
    case D3DFMT_L16:             return "L16";
    case D3DFMT_Q16W16V16U16:    return "Q16W16V16U16";
    case D3DFMT_CxV8U8:          return "CxV8U8";
    case D3DFMT_R16F:            return "R16F";
    case D3DFMT_G16R16F:         return "G16R16F";
    case D3DFMT_A16B16G16R16F:   return "A16B16G16R16F";
    case D3DFMT_R32F:            return "R32F";
    case D3DFMT_G32R32F:         return "G32R32F";
    case D3DFMT_A32B32G32R32F:   return "A32B32G32R32F";
    case D3DFMT_DXT1:            return "DXT1";
    case D3DFMT_YUY2:            return "YUY2";
    case D3DFMT_DXT2:            return "DXT2";
    case D3DFMT_DXT3:            return "DXT3";
    case D3DFMT_DXT4:            return "DXT4";
    case D3DFMT_DXT5:            return "DXT5";
    case D3DFMT_UYVY:            return "UYVY";
    case D3DFMT_R8G8_B8G8:       return "R8G8_B8G8";
    case D3DFMT_G8R8_G8B8:       return "G8R8_G8B8";
    default:                     return "UNKNOWN";
    }
}


// a chain of `std::operator==(const string&, const char*)` (the MSVCP71
// import), each hit returning its constant; the last test folds into `== ? 0x74 : 0`. The order is
// the shipped one (L16 after L8, the bump formats after Q16W16V16U16, the float formats last). The
// names are distinct, so the order is behaviour-neutral, and it is kept as read.

D3DFORMAT __fastcall stringToFormat(const std::string& name)
{
    if (name == "R8G8B8")          return D3DFMT_R8G8B8;
    if (name == "A8R8G8B8")        return D3DFMT_A8R8G8B8;
    if (name == "X8R8G8B8")        return D3DFMT_X8R8G8B8;
    if (name == "R5G6B5")          return D3DFMT_R5G6B5;
    if (name == "X1R5G5B5")        return D3DFMT_X1R5G5B5;
    if (name == "A1R5G5B5")        return D3DFMT_A1R5G5B5;
    if (name == "A4R4G4B4")        return D3DFMT_A4R4G4B4;
    if (name == "R3G3B2")          return D3DFMT_R3G3B2;
    if (name == "A8")              return D3DFMT_A8;
    if (name == "A8R3G3B2")        return D3DFMT_A8R3G3B2;
    if (name == "X4R4G4B4")        return D3DFMT_X4R4G4B4;
    if (name == "A2B10G10R10")     return D3DFMT_A2B10G10R10;
    if (name == "A8B8G8R8")        return D3DFMT_A8B8G8R8;
    if (name == "X8B8G8R8")        return D3DFMT_X8B8G8R8;
    if (name == "G16R16")          return D3DFMT_G16R16;
    if (name == "A2R10G10B10")     return D3DFMT_A2R10G10B10;
    if (name == "A16B16G16R16")    return D3DFMT_A16B16G16R16;
    if (name == "A8P8")            return D3DFMT_A8P8;
    if (name == "P8")              return D3DFMT_P8;
    if (name == "L8")              return D3DFMT_L8;
    if (name == "L16")             return D3DFMT_L16;
    if (name == "A8L8")            return D3DFMT_A8L8;
    if (name == "A4L4")            return D3DFMT_A4L4;
    if (name == "V8U8")            return D3DFMT_V8U8;
    if (name == "Q8W8V8U8")        return D3DFMT_Q8W8V8U8;
    if (name == "V16U16")          return D3DFMT_V16U16;
    if (name == "Q16W16V16U16")    return D3DFMT_Q16W16V16U16;
    if (name == "CxV8U8")          return D3DFMT_CxV8U8;
    if (name == "L6V5U5")          return D3DFMT_L6V5U5;
    if (name == "X8L8V8U8")        return D3DFMT_X8L8V8U8;
    if (name == "A2W10V10U10")     return D3DFMT_A2W10V10U10;
    if (name == "G8R8_G8B8")       return D3DFMT_G8R8_G8B8;
    if (name == "R8G8_B8G8")       return D3DFMT_R8G8_B8G8;
    if (name == "DXT1")            return D3DFMT_DXT1;
    if (name == "DXT2")            return D3DFMT_DXT2;
    if (name == "DXT3")            return D3DFMT_DXT3;
    if (name == "DXT4")            return D3DFMT_DXT4;
    if (name == "DXT5")            return D3DFMT_DXT5;
    if (name == "UYVY")            return D3DFMT_UYVY;
    if (name == "YUY2")            return D3DFMT_YUY2;
    if (name == "R16F")            return D3DFMT_R16F;
    if (name == "G16R16F")         return D3DFMT_G16R16F;
    if (name == "A16B16G16R16F")   return D3DFMT_A16B16G16R16F;
    if (name == "R32F")            return D3DFMT_R32F;
    if (name == "G32R32F")         return D3DFMT_G32R32F;
    if (name == "A32B32G32R32F")   return D3DFMT_A32B32G32R32F;
    return D3DFMT_UNKNOWN;
}

} // namespace textureatlas

#endif // BF2_IMAGE_TEXTUREATLASBUILDER
