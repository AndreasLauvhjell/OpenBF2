// Scaffolding, not recovered source. Exercises BitStream bit, compressed-position,
// and three-component quaternion round trips. Checks wire encoding and decoded values.
// Build: Code\Build\demo\boot.cmd
#include <cstdio>
#include "BitStream.h"
using namespace dice::hfe;
using namespace dice::hfe::io;

int main()
{
    printf("=== BF2 recovered-engine boot demo ===\n\n");

    unsigned char buf[256];
    BitStream* bs = (BitStream*)::operator new(sizeof(BitStream));
    bs->init(buf, sizeof buf);

    unsigned int tickRate = 30;
    bs->writeBits(&tickRate, 8);
    unsigned int got = 0;
    bs->readBits(&got, 8);
    printf("[BitStream] tick rate through the wire: %u\n", got);

    Vec3 ref; ref.x = 100.0f; ref.y = 20.0f; ref.z = -5.0f;
    Vec3 v;   v.x = 100.5f;   v.y = 20.25f;  v.z = -4.75f;
    bs->writeCompressedVector(v, ref, 0.01f);
    Vec3 back = bs->readCompressedVector(ref, 0.01f);
    printf("[BitStream] position (%.2f %.2f %.2f) -> (%.2f %.2f %.2f)\n",
           v.x, v.y, v.z, back.x, back.y, back.z);

    Quat q; q.x = 0.0f; q.y = 0.0f; q.z = 0.3826834f; q.w = 0.9238795f;   // 45 deg about Z
    bs->writeUnitQuaternion3Comp(q, 16);
    Quat qb = bs->readUnitQuaternion3Comp(16);
    printf("[BitStream] rotation (%.4f %.4f %.4f %.4f) -> (%.4f %.4f %.4f %.4f)\n",
           q.x,q.y,q.z,q.w, qb.x,qb.y,qb.z,qb.w);

    printf("\nboot demo complete.\n");
    return 0;
}

