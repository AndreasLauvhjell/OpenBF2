// Quantized animation sample decoding.

#include "CompressedAnim.h"

#include "../../Math/Quat.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace anim {


// Returns a decoded bone/channel sample at frame.
// Channel offsets are cumulative; samples are signed 16-bit values and frame boundaries advance to the next block.

float CompressedAnim::GetValue(int bone, int frame, int channel)
{
    const BoneData& data = boneData[bone];                                       // +0x04 each
    const unsigned short* stream = channel ? data.samples + data.channelEnd[channel - 1]
                                           : data.samples;

    int f = frame;
    int blockStart = 0;                                          // in 16-bit words from `stream`
    const unsigned char* header = reinterpret_cast<const unsigned char*>(stream);
    int run = header[0] & 0x7F;
    unsigned char flags;
    if (f <= run - 1)
    {
        flags = header[0];
    }
    else
    {
        do
        {
            f -= run;
            blockStart += header[1];                             // the block's length, in words
            header = reinterpret_cast<const unsigned char*>(stream + blockStart);
            flags = header[0];
            run = flags & 0x7F;
        }
        while (f > run - 1);
    }

    float scale = 1.0f / static_cast<float>((1 << precisionBits) - 1);          // +0x03
    if (channel <= 3)
        scale = 1.0f / 32767.0f;                                 // flt_88CC0E8 - the rotation scale

    // bit 7 of the header's low byte: a constant block carries one sample for every frame in it.
    const short sample = (flags & 0x80)
        ? static_cast<short>(stream[blockStart + 1])
        : static_cast<short>(stream[blockStart + 1 + f]);
    return scale * static_cast<float>(sample);
}

// Channels 0, 1, 2 into x, y, z by a three-round loop, then channel 3 into w.
void CompressedAnim::GetQuat(int bone, int frame, hfe::Quat& out)
{
    out.x = GetValue(bone, frame, 0);
    out.y = GetValue(bone, frame, 1);
    out.z = GetValue(bone, frame, 2);
    out.w = GetValue(bone, frame, 3);
}

// Channels 4, 5, 6 into x, y, z.
void CompressedAnim::GetTrans(int bone, int frame, hfe::Vec3& out)
{
    out.x = GetValue(bone, frame, 4);
    out.y = GetValue(bone, frame, 5);
    out.z = GetValue(bone, frame, 6);
}

} // namespace anim
} // namespace dice
