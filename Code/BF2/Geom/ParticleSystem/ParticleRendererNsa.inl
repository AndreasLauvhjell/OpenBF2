// [INFERRED] private split of ParticleSystemManager.cpp. Complete BF2 1.50 RendDX9
// instructions read: 1016E550/EE70/D640/D8F0/EC10/D9B0. No dedicated ELF renderer body.
// Included after the ordinary renderer helpers; all offsets address renderer-owned records.

// 1016F07C..1016F0A6 uses fsin/fcos and __ftol2 truncation on the live x87 result.
static void psmNsaPackRotation(float angle, short& sine, short& cosine)
{
    const float scale = 32767.0f;
    unsigned short savedControl, truncateControl;
    __int64 sinValue, cosValue;
    __asm {
        fnstcw savedControl
        mov ax, savedControl
        or ax, 0C00h
        mov truncateControl, ax
        fld angle
        fsin
        fmul scale
        fldcw truncateControl
        fistp sinValue
        fldcw savedControl
        fld angle
        fcos
        fmul scale
        fldcw truncateControl
        fistp cosValue
        fldcw savedControl
    }
    sine = static_cast<short>(sinValue);
    cosine = static_cast<short>(cosValue);
}

// The instance parameter is unused. Packed state advances once per dirty
// particle; the oriented corners are supplied by the NSA simulation, not screen axes.
static void psmUpdateNsaDrawState(unsigned char* renderer, unsigned char* particle,
    unsigned char* descriptor, unsigned char*, dx9rend::IRenderView* view)
{
    unsigned int& flags = psmDrawField<unsigned int>(particle, 0x28);
    if (!(flags & 0x80)) return;
    flags &= ~0x80u;
    const double age = psmDrawField<float>(particle, 0x24);
    const float rate = static_cast<float>(age * age * psmDrawField<float>(descriptor, 0xF0)
        + age * psmDrawField<float>(descriptor, 0xF4)
        + age * age * age * psmDrawField<float>(descriptor, 0xEC)
        + psmDrawField<float>(descriptor, 0xF8));
    const float dt = psmParticleClamp(rate, 0, 1) * psmDrawField<float>(renderer, 0x42C);
    float& rotation = psmDrawField<float>(particle, 0x88);
    if (descriptor[0x210])
    {
        const float* corners = reinterpret_cast<float*>(particle + 0x98);
        const float* velocity = reinterpret_cast<float*>(particle + 0x68);
        const float firstZ = corners[2] - corners[11];
        const float x = static_cast<float>((static_cast<double>(corners[0]) - corners[9]) * 0.5);
        const float y = static_cast<float>((static_cast<double>(corners[1]) - corners[10]) * 0.5);
        const float z = firstZ * 0.5f;
        const float secondZ = corners[8] - corners[11];
        const float secondX = static_cast<float>((static_cast<double>(corners[6]) - corners[9]) * 0.5);
        const double secondY = (static_cast<double>(corners[7]) - corners[10]) * 0.5;
        const double secondHalfZ = static_cast<double>(secondZ) * 0.5;
        const float firstDot = static_cast<float>(static_cast<double>(z) * velocity[2]
            + static_cast<double>(y) * velocity[1] + static_cast<double>(x) * velocity[0]);
        const float secondDot = static_cast<float>(secondHalfZ * velocity[2]
            + secondY * velocity[1] + static_cast<double>(secondX) * velocity[0]);
        const float scaledX = secondX * secondDot;
        const float scaledY = static_cast<float>(secondY * secondDot);
        const float scaledZ = z * firstDot;
        float projected[3] = {
            static_cast<float>(static_cast<double>(x) * firstDot + scaledX),
            static_cast<float>(static_cast<double>(y) * firstDot + scaledY),
            static_cast<float>(scaledZ + secondHalfZ * secondDot) };
        const double lengthSquared = static_cast<double>(projected[2]) * projected[2]
            + static_cast<double>(projected[1]) * projected[1]
            + static_cast<double>(projected[0]) * projected[0];
        if (std::fabs(lengthSquared) > 0.0000023841858f)
        {
            psmNormalize3(projected); // PE 100777A0, including its unordered comparison.
            const float cosine = static_cast<float>(static_cast<double>(projected[2]) * z
                + static_cast<double>(projected[1]) * y + static_cast<double>(projected[0]) * x);
            const double angle = std::acos(psmParticleClamp(cosine, -1, 1));
            rotation = static_cast<float>(secondDot > 0 ? angle : 6.2831855f - angle);
        }
        else rotation = 0;
    }
    else rotation = static_cast<float>(std::fmod(static_cast<double>(dt)
        * psmDrawField<float>(particle, 0x84) + rotation, 6.283185482025146));
    psmNsaPackRotation(rotation, psmDrawField<short>(particle, 0x1C), psmDrawField<short>(particle, 0x1E));
    if (descriptor[0xA0])
    {
        const float height = static_cast<float>((static_cast<double>(view->getViewMatrixPtr()->m[13])
            - psmDrawField<float>(particle, 0x14)) * 0.0024999999f);
        psmDrawField<short>(particle, 0x22) = static_cast<short>(static_cast<__int64>(
            static_cast<double>(psmParticleClamp(height, 0, 1)) * 32767.0f));
    }
    else psmDrawField<short>(particle, 0x22) = 0;
    const unsigned int frameCount = psmDrawField<unsigned int>(descriptor, 0xD0);
    if (descriptor[0xC4] && frameCount)
    {
        float& frame = psmDrawField<float>(particle, 0x80);
        const double nextFrame = static_cast<double>(dt) * psmDrawField<float>(descriptor, 0xC0) + frame;
        frame = static_cast<float>(nextFrame);
        const unsigned int whole = static_cast<unsigned int>(static_cast<int>(std::floor(nextFrame)));
        psmDrawField<float>(particle, 0x3C) = static_cast<float>(static_cast<double>(frame) - whole);
        const unsigned int current = whole % frameCount;
        if (current != (flags & 0xF))
        {
            flags ^= (current ^ flags) & 0xF;
            const unsigned int next = (current + 1) % frameCount;
            const unsigned int columns = psmDrawField<unsigned int>(descriptor, 0xD4);
            const unsigned int xs[2] = { current % columns, next % columns };
            const double ys[2] = {
                static_cast<double>(current - xs[0]) * psmDrawField<float>(descriptor, 0xD8),
                static_cast<float>(static_cast<double>(next - xs[1]) * psmDrawField<float>(descriptor, 0xD8)) };
            for (unsigned int i = 0; i < 2; ++i)
            {
                psmDrawField<short>(particle, 0x40 + 4 * i) = static_cast<short>(static_cast<__int64>(
                    (static_cast<double>(xs[i]) * psmDrawField<float>(descriptor, 0x12C)
                    + static_cast<double>(psmDrawField<short>(descriptor, 0x212)) * 0.000030518509f) * 32767.0f));
                psmDrawField<short>(particle, 0x42 + 4 * i) = static_cast<short>(static_cast<__int64>(
                    (static_cast<double>(psmDrawField<short>(descriptor, 0x214)) * 0.000030518509f
                    + ys[i] * psmDrawField<float>(descriptor, 0x130)) * 32767.0f));
            }
        }
    }
}

// PE 1016E550 retains its two leading view calls and one fresh view read per accepted particle.
static void psmCollectNsaParticles(unsigned char* renderer, dx9rend::IRenderView* view,
    unsigned char* instance, unsigned char* descriptor)
{
    view->getViewMatrixPtr();
    view->getViewMatrixPtr();
    const float nearPlane = view->getNearPlane();
    const unsigned int count = psmDrawField<unsigned int>(instance, 0x28);
    if (psmDrawField<int>(descriptor, 8) == -1)
        psmDrawField<unsigned int>(descriptor, 8) = psmDrawField<unsigned int>(renderer, 0x51C)++;
    ParticleSystemManager* manager = reinterpret_cast<ParticleSystemManager*>(g_ParticleSystemManager);
    const unsigned int lod = psmDrawField<unsigned int>(descriptor, 0x6C);
    const float farDistance = manager->m_farDistances[lod];
    const float inverseFarRange = manager->m_inverseFarRanges[lod];
    for (unsigned int i = 0; i < count; ++i)
    {
        unsigned char* particle = psmDrawField<unsigned char**>(instance, 0x24)[i];
        psmDrawField<unsigned short>(particle, 0x2C) = psmDrawField<unsigned short>(descriptor, 4);
        const double depth = static_cast<double>(psmDrawField<float>(particle, 0x14)) * psmDrawField<float>(renderer, 0x434)
            + static_cast<double>(psmDrawField<float>(particle, 0x18)) * psmDrawField<float>(renderer, 0x438)
            + static_cast<double>(psmDrawField<float>(particle, 0x10)) * psmDrawField<float>(renderer, 0x430)
            + psmDrawField<float>(renderer, 0x43C);
        const float roundedDepth = static_cast<float>(depth);
        if (depth < nearPlane) continue; // PE test ah,5 / jnp retains unordered input.
        float& alpha = psmDrawField<float>(particle, 0x34);
        if (roundedDepth > farDistance)
        {
            double fade = (static_cast<double>(roundedDepth) - farDistance) * inverseFarRange;
            if (fade > 1) fade = 1;
            alpha = static_cast<float>((1 - fade) * (1 - fade) * psmDrawField<float>(particle, 0x38));
        }
        else alpha = psmDrawField<float>(particle, 0x38);
        if (alpha < psmDrawField<float>(descriptor, 0x94)) continue;
        const Mat4* camera = view->getViewMatrixPtr();
        const float x = camera->m[12] - psmDrawField<float>(particle, 0x10);
        const double y = static_cast<double>(camera->m[13]) - psmDrawField<float>(particle, 0x14);
        const double z = static_cast<double>(camera->m[14]) - psmDrawField<float>(particle, 0x18);
        psmDrawField<float>(particle, 0x94) = static_cast<float>(z * z + y * y + static_cast<double>(x) * x);
        psmUpdateNsaDrawState(renderer, particle, descriptor, instance, view);
        psmDrawField<unsigned char**>(renderer, 0x1C)[psmDrawField<unsigned int>(renderer, 0x428)++] = particle;
    }
    vc71::vectorRef<unsigned char*>(renderer + 0xC).push_back(instance);
}

// stable radix order, using the particle +94 distance and renderer +20 buckets.
static void psmSortNsaParticles(unsigned char* renderer, unsigned char** table, unsigned int count)
{
    unsigned char** buckets = reinterpret_cast<unsigned char**>(renderer + 0x20);
    for (unsigned int byte = 0; byte < 4; ++byte)
    {
        for (int i = static_cast<int>(count) - 1; i >= 0; --i)
        {
            unsigned char* particle = table[i];
            const unsigned int key = particle[0x94 + byte];
            psmDrawField<unsigned char*>(particle, 0) = buckets[key];
            buckets[key] = particle;
        }
        unsigned int output = 0;
        for (unsigned int key = 0; key < 256; ++key)
        {
            unsigned char* particle = buckets[key];
            buckets[key] = 0;
            while (particle)
            {
                table[output++] = particle;
                particle = psmDrawField<unsigned char*>(particle, 0);
            }
        }
    }
}

static unsigned int psmBatchNsaParticles(unsigned char* renderer, unsigned int begin, unsigned int end)
{
    unsigned char* mapping = psmDrawField<unsigned char*>(renderer, 0x55C);
    ::memset(mapping, 0xFF, 0x800);
    unsigned int& count = psmDrawField<unsigned int>(renderer, 0x560);
    count = 0;
    unsigned int i = begin;
    for (; i < end; ++i)
    {
        unsigned char* particle = psmDrawField<unsigned char**>(renderer, 0x1C)[psmDrawField<int>(renderer, 8) - i];
        const unsigned short id = psmDrawField<unsigned short>(particle, 0x2C);
        if (mapping[id] != 0xFF) continue;
        if (count >= 10) break;
        mapping[id] = static_cast<unsigned char>(count);
        unsigned char* descriptor = reinterpret_cast<unsigned char*>(g_templatesById[id]);
        psmParticleCopyParameters(psmDrawField<unsigned char*>(renderer, 0x564) + count * 112, descriptor + 0x12C);
        ++count;
    }
    psmParticleParameter(renderer, 0x584)->setValue(psmDrawField<void*>(renderer, 0x564), 0);
    return i;
}

static void psmBindNsaParticleParameters(unsigned char* renderer)
{
    if (!psmDrawPointerVectorSize(renderer + 0xC)) return;
    psmParticleParameter(renderer, 0x570)->setValue(renderer + 0x448, 0);
    psmParticleParameter(renderer, 0x574)->setValue(renderer + 0x4C8, 0);
    HemiMap* map = g_HemiMapManager->getBaseHemiMap();
    if (map)
    {
        psmParticleParameter(renderer, 0x578)->setValue(&g_HemiMapManager->getHemiMapInfo(map), 0);
        psmParticleParameter(renderer, 0x57C)->setFloat(map->falloff);
        psmParticleParameter(renderer, 0x580)->setValue(map->texture, 0);
    }
    else
    {
        HemiMap* fallback = g_HemiMapManager->getBaseHemiMap(); // PE 100F1490 rereads the owner.
        psmParticleParameter(renderer, 0x580)->setValue(fallback ? fallback->texture : 0, 0);
    }
    psmParticleParameter(renderer, 0x594)->setValue(&g_LightManager->getEffectSunColor(), 0);
    psmParticleParameter(renderer, 0x598)->setValue(&g_LightManager->getEffectShadowColor(), 0);
    ParticleSystemManager* manager = reinterpret_cast<ParticleSystemManager*>(g_ParticleSystemManager);
    if (!manager->m_shaderQuality)
    {
        const float scale = 1.0f / manager->m_value8CA4;
        const Vec3 color = { scale, scale, scale };
        psmParticleParameter(renderer, 0x594)->setValue(&color, 0);
    }
    psmParticleParameter(renderer, 0x58C)->setValue(static_cast<unsigned char*>(dx9rend::g_raShaderConstants) + 0xA8, 0);
    psmParticleParameter(renderer, 0x590)->setValue(static_cast<unsigned char*>(dx9rend::g_raShaderConstants) + 0xBC, 0);
    manager = reinterpret_cast<ParticleSystemManager*>(g_ParticleSystemManager);
    static const char* const techniques[] = {"NSAParticleShowFill", "NSAParticleLow", "NSAParticleMedium", "NSAParticleHigh"};
    if (manager->m_shaderQuality <= 3)
        dx9rend::effectSetTechniqueByName(psmDrawField<void*>(renderer, 0x56C), vc71::string(techniques[manager->m_shaderQuality]));
    dx9rend::vertexDeclarationBind(psmDrawField<void*>(renderer, 0x59C));
    dx9rend::vertexBufferBindStream(psmDrawField<void*>(renderer, 0x568), 0);
    dx9rend::effectBegin(psmDrawField<void*>(renderer, 0x56C));
    dx9rend::effectBeginPass(psmDrawField<void*>(renderer, 0x56C), 0);
}

static void psmWriteNsaParticleVertices(unsigned char* renderer, unsigned char* particle, unsigned char* output)
{
    float minimumSize = psmDrawField<float>(particle, 0x8C);
    if (psmDrawField<float>(particle, 0x48) < minimumSize) minimumSize = psmDrawField<float>(particle, 0x48);
    const unsigned short id = psmDrawField<unsigned short>(particle, 0x2C);
    const float descriptorIndex = static_cast<float>(static_cast<signed char>(psmDrawField<unsigned char*>(renderer, 0x55C)[id]));
    const unsigned int corners[6] = {0, 1, 3, 3, 1, 2};
    const short uvs[6][2] = {{-32768,32767},{32767,32767},{-32768,-32768},
        {-32768,-32768},{32767,32767},{32767,-32768}};
    for (unsigned int i = 0; i < 6; ++i)
    {
        unsigned char* vertex = output + i * 72;
        ::memcpy(vertex, particle + 0x10, 12);
        psmDrawField<unsigned int>(vertex, 0xC) = psmDrawField<unsigned int>(particle, 0x24);
        psmDrawField<float>(vertex, 0x10) = descriptorIndex;
        ::memcpy(vertex + 0x14, particle + 0x30, 8);
        psmDrawField<unsigned int>(vertex, 0x1C) = psmDrawField<unsigned int>(particle, 0x3C);
        ::memcpy(vertex + 0x20, particle + 0x98 + 12 * corners[i], 12);
        psmDrawField<float>(vertex, 0x2C) = uvs[i][1] > 0 ? psmDrawField<float>(particle, 0x48) : minimumSize;
        psmDrawField<unsigned int>(vertex, 0x30) = psmDrawField<unsigned int>(particle, 0x4C);
        ::memcpy(vertex + 0x34, particle + 0x1C, 8);
        ::memcpy(vertex + 0x3C, particle + 0x40, 8);
        psmDrawField<short>(vertex, 0x44) = uvs[i][0];
        psmDrawField<short>(vertex, 0x46) = uvs[i][1];
    }
}

static bool psmDrawNsaParticles(unsigned char* renderer, unsigned int target)
{
    if (!psmDrawField<unsigned int>(renderer, 0x428)) return false;
    const int initialIndex = psmDrawField<int>(renderer, 8);
    unsigned int tableIndex = initialIndex;
    unsigned char* first = psmDrawField<unsigned char**>(renderer, 0x1C)[tableIndex];
    void* texture = psmDrawField<void*>(first, 0x58);
    psmParticleParameter(renderer, 0x588)->setValue(texture, 0);
    const unsigned int count = psmDrawField<unsigned int>(renderer, 0x428);
    unsigned int batchEnd = psmBatchNsaParticles(renderer, 0, count);
    psmBindNsaParticleParameters(renderer);
    unsigned int vertices = 0;
    unsigned char* buffer = static_cast<unsigned char*>(dx9rend::vertexBufferLock(
        psmDrawField<void*>(renderer, 0x568), psmDrawField<unsigned int>(renderer, 0x444),
        6 * count, psmDrawField<unsigned int>(renderer, 0x444) ? 6144 : 10240));
    if (!buffer) return false;
    unsigned int consumed = 0;
    for (; consumed < count; ++consumed)
    {
        unsigned char* particle = psmDrawField<unsigned char**>(renderer, 0x1C)[tableIndex];
        if (psmDrawField<unsigned int>(particle, 0x94) < target) break;
        if (consumed == batchEnd)
        {
            dx9rend::vertexBufferUnlock(psmDrawField<void*>(renderer, 0x568));
            dx9rend::rendererDrawPrimitive(vertices / 3, 4, psmDrawField<unsigned int>(renderer, 0x444));
            psmDrawField<unsigned int>(renderer, 0x444) += vertices;
            vertices = 0;
            batchEnd = psmBatchNsaParticles(renderer, consumed, count);
            if (psmDrawField<void*>(particle, 0x58) != texture)
            {
                psmParticleParameter(renderer, 0x588)->setValue(psmDrawField<void*>(particle, 0x58), 0);
                texture = psmDrawField<void*>(particle, 0x58);
            }
            buffer = static_cast<unsigned char*>(dx9rend::vertexBufferLock(psmDrawField<void*>(renderer, 0x568),
                psmDrawField<unsigned int>(renderer, 0x444), 6 * (batchEnd - consumed), 4096));
            if (!buffer) return false; // Retail leaves the effect active on these lock failures.
            psmCountParticleDraw(0x8C48);
        }
        if (psmDrawField<void*>(particle, 0x58) != texture)
        {
            dx9rend::vertexBufferUnlock(psmDrawField<void*>(renderer, 0x568));
            dx9rend::rendererDrawPrimitive(vertices / 3, 4, psmDrawField<unsigned int>(renderer, 0x444));
            psmDrawField<unsigned int>(renderer, 0x444) += vertices;
            vertices = 0;
            psmParticleParameter(renderer, 0x588)->setValue(psmDrawField<void*>(particle, 0x58), 0);
            buffer = static_cast<unsigned char*>(dx9rend::vertexBufferLock(psmDrawField<void*>(renderer, 0x568),
                psmDrawField<unsigned int>(renderer, 0x444), 6 * (batchEnd - consumed), 4096));
            if (!buffer) return false;
            texture = psmDrawField<void*>(particle, 0x58);
            psmCountParticleDraw(0x8C48);
        }
        psmWriteNsaParticleVertices(renderer, particle, buffer + vertices * 72);
        vertices += 6;
        --tableIndex;
        ++psmDrawField<unsigned int>(renderer, 0x420);
        --psmDrawField<unsigned int>(renderer, 0x428);
    }
    dx9rend::vertexBufferUnlock(psmDrawField<void*>(renderer, 0x568));
    psmCountParticleDraw(0x8C48);
    dx9rend::rendererDrawPrimitive(vertices / 3, 4, psmDrawField<unsigned int>(renderer, 0x444));
    psmDrawField<unsigned int>(renderer, 0x444) += vertices;
    psmDrawField<int>(renderer, 8) = initialIndex - consumed;
    dx9rend::effectEnd(psmDrawField<void*>(renderer, 0x56C));
    return true;
}
