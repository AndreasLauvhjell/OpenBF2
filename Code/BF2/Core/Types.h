// Engine fixed-width type names. [CHOSEN] Header placement; widths follow the x86 reference.

#ifndef CORE_TYPES_H     // [INFERRED] guard style
#define CORE_TYPES_H

namespace dice {
namespace hfe {

typedef unsigned int   U32;     // attested spelling (console metadata "U32")
typedef signed int     S32;     // [INFERRED] sibling
typedef unsigned short U16;     // [INFERRED] sibling
typedef unsigned char  U8;      // [INFERRED] sibling

} // namespace hfe
} // namespace dice

#endif // CORE_TYPES_H
