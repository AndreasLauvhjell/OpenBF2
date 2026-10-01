// Reads BAF timeline metadata: version 4, bone count/ids, frame count, and precision.
// The engine stores frame count in 16 bits and plays at 24 fps. This engine-free reader
// supports editor timeline sizing and joint-count validation.
#ifndef BF2EDITOR_FRAMEWORK_BAFHEADER_H
#define BF2EDITOR_FRAMEWORK_BAFHEADER_H

#include "Framework/EditorConfig.h"

#include <string>
#include <vector>

namespace bf2editor
{

struct BafHeader
{
    int                         version = 0;
    std::vector<unsigned short> boneIds;
    int                         frames = 0;         // as stored (32 bits)
    unsigned char               precision = 0;
    // As the engine plays it: the frame count narrowed to 16 bits, 24 frames a second.
    float seconds() const { return static_cast<float>(frames & 0xFFFF) / 24.0f; }
    int joints() const { return static_cast<int>(boneIds.size()); }
};

// The header from the file's first bytes (the rest is not needed). False with `error` when the bytes are not
// a version 4 .baf header.
bool readBafHeader(const std::string& bytes, BafHeader& header, std::string& error);

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_BAFHEADER_H
