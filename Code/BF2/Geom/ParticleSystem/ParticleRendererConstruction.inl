// Renderer construction helpers included after psmFillTrailIndices in the manager's namespace.
// Fixed-layout storage is Win32-owned in both host flavors; keep its compatibility vectors.

template<class T>
static T& psmCtorField(unsigned char* renderer, unsigned int offset)
{
    return *reinterpret_cast<T*>(renderer + offset);
}

static void psmCtorZeroWords(unsigned char* renderer, unsigned int offset, unsigned int count)
{
    for (unsigned int i = 0; i < count; ++i)
        psmCtorField<unsigned int>(renderer, offset + 4u * i) = 0;
}

static void psmCtorIdentity(unsigned char* matrix)
{
    psmCtorZeroWords(matrix, 0, 16);
    for (unsigned int i = 0; i < 4; ++i)
        psmCtorField<float>(matrix, 20u * i) = 1.0f;
}

// three identity matrices followed by one zero Vec4.
static void psmCtorParticleMatrices(unsigned char* matrices)
{
    for (unsigned int i = 0; i < 3; ++i)
        psmCtorIdentity(matrices + 64u * i);
    psmCtorZeroWords(matrices, 0xC0, 4);
}

// four identity matrices.
static void psmCtorMeshMatrices(unsigned char* matrices)
{
    for (unsigned int i = 0; i < 4; ++i)
        psmCtorIdentity(matrices + 64u * i);
}

static void psmCtorPointerVector(unsigned char* storage)
{
    // allocator<> is empty. As in the PE, its padding word is not written.
    vc71::vector_layout& vector = *reinterpret_cast<vc71::vector_layout*>(storage);
    vector.first = 0;
    vector.last = 0;
    vector.end = 0;
}

// The source operations are resize(30, nullptr), then resize(0), retaining capacity 30.
// 10172890 / 1016F4F0 / 10174410, with the first-growth branch of 10172660. This private
// construction adapter handles that initial reserve only, rather than pretending to implement
// a general-purpose STL container. The 120-byte allocation is unconditionally populated in PE.
static void psmCtorPrimePointerVector(unsigned char* storage)
{
    vc71::vector_layout& vector = *reinterpret_cast<vc71::vector_layout*>(storage);
    void** const entries = static_cast<void**>(::renddx9abi::engineAlloc(30u * sizeof(void*)));
    for (unsigned int i = 0; i < 30; ++i)
        entries[i] = 0;
    vector.end = reinterpret_cast<char*>(entries + 30);
    vector.last = reinterpret_cast<char*>(entries + 30);
    vector.first = reinterpret_cast<char*>(entries);
    vector.last = vector.first;
}

struct PsmCtorParameter
{
    unsigned int offset;
    const char* name;
};

template<unsigned int N>
static void psmCtorParameters(unsigned char* renderer, unsigned int effectOffset,
                              const PsmCtorParameter (&parameters)[N])
{
    for (unsigned int i = 0; i < N; ++i)
    {
        // Each temporary string and repeated effect read follows the corresponding PE call.
        const vc71::string name(parameters[i].name);
        psmCtorField<dx9rend::ShaderParameter*>(renderer, parameters[i].offset) =
            dx9rend::effectGetShaderParameterTyped(
                psmCtorField<dx9rend::Effect*>(renderer, effectOffset), name);
    }
}

// Each of the ten 0x70-byte records is value-constructed. The second allocation is left
// uninitialized in both constructors. Both are operator-new[]'s Memory.dll allocation seam.
static void psmCtorParticleScratch(unsigned char* renderer, unsigned int recordsOffset,
                                   unsigned int scratchOffset)
{
    void* records = ::renddx9abi::engineAlloc(0x460);
    if (records)
        psmCtorZeroWords(static_cast<unsigned char*>(records), 0, 0x460 / 4);
    psmCtorField<void*>(renderer, recordsOffset) = records;
    psmCtorField<void*>(renderer, scratchOffset) = ::renddx9abi::engineAlloc(0x800);
}

// PE 10171740..10171779 uses x87 fsin/fcos and __ftol2's truncation, keeping the angle on
// the x87 stack between the calls. Preserve that precision instead of spilling through a
// modern compiler's double/SSE temporary at the signed-short quantization boundary.
static void psmCtorTrigPair(unsigned int index, short& sine, short& cosine)
{
    const float step = 0.0015339808305725455284f; // 1021EF18, float(2*pi/4096)
    const float amplitude = 32767.0f;
    unsigned short originalControl;
    unsigned short truncateControl;
    short sineValue;
    short cosineValue;
#if defined(_M_IX86)
    __asm {
        fild index
        fmul step
        fld st(0)
        fsin
        fmul amplitude
        fnstcw originalControl
        mov ax, originalControl
        or ax, 0C00h
        mov truncateControl, ax
        fldcw truncateControl
        fistp sineValue
        fldcw originalControl
        fcos
        fmul amplitude
        fldcw truncateControl
        fistp cosineValue
        fldcw originalControl
    }
#else
#error Particle renderer reconstruction requires the supported MSVC x86 target.
#endif
    sine = sineValue;
    cosine = cosineValue;
}

static void psmConstructParticleRenderer(unsigned char* renderer)
{
    renderer[0] = 0;
    psmCtorField<int>(renderer, 4) = -1;
    psmCtorPointerVector(renderer + 8);
    psmCtorField<void*>(renderer, 0x18) = 0;
    psmCtorZeroWords(renderer, 0x20, 256u * 3u);
    psmCtorZeroWords(renderer, 0xC20, 256u * 3u);
    psmCtorField<unsigned int>(renderer, 0x1824) = 0;
    psmCtorField<unsigned int>(renderer, 0x1828) = 0;
    psmCtorField<unsigned int>(renderer, 0x1C2C) = 0;
    psmCtorField<unsigned int>(renderer, 0x1C30) = 0;
    renderer[0x1C34] = 0;
    psmCtorZeroWords(renderer, 0x1C38, 6);
    psmCtorParticleMatrices(renderer + 0x1C58);
    psmCtorIdentity(renderer + 0x1D28);
    renderer[0x1D6C] = 0;
    psmCtorField<unsigned int>(renderer, 0x1D70) = 0;
    renderer[0x1D78] = 0;
    renderer[0x1D79] = 0;
    psmCtorField<unsigned int>(renderer, 0x1D80) = 0;
    psmCtorField<unsigned int>(renderer, 0x1D84) = 0;
    for (unsigned int i = 0; i < 4; ++i)
        psmCtorField<float>(renderer, 0x1D8C + 4u * i) = 0.5f;
    renderer[0x1D9C] = 1;
    renderer[0x1D9D] = 0;
    renderer[0x1D9E] = 0;
    psmCtorField<void*>(renderer, 0x1DC0) = 0;
    psmCtorField<void*>(renderer, 0x1DC8) = 0;
    psmCtorField<void*>(renderer, 0x1DCC) = 0;
    psmCtorPrimePointerVector(renderer + 8);
    psmCtorZeroWords(renderer, 0x182C, 256);

    const D3DVERTEXELEMENT9 declaration[] = {
        {0, 0x00, 2, 0, 0, 0}, {0, 0x0C, 1, 0, 5, 0},
        {0, 0x14, 2, 0, 5, 1}, {0, 0x20, 1, 0, 5, 2},
        {0, 0x28, 1, 0, 5, 3}, {0, 0x30, 6, 0, 5, 4},
        {0, 0x34, 7, 0, 5, 5}, {0, 0x3C, 6, 0, 5, 6}, D3DDECL_END()
    };
    psmCtorField<void*>(renderer, 0x1E0C) = dx9rend::vertexDeclarationCreate(declaration);
    if (!psmCtorField<void*>(renderer, 0x1E0C))
    {
        // PE 101713A2..10171407 emits Debug type 6. Its destructor 10074690 traps after
        // emission, but the renderer's per-type/module suppression configuration can skip it.
        // [TEMP] [CHOSEN, not recovered] The renderer Debug dispatcher is not bridged here.
        // [RETIRE-WHEN: particle-constructor-debug-dispatch] Preserve an explicit stop at that
        // error-only dependency rather than replacing the retail diagnostic with log-and-return.
        RD9_FRONTIER_AT("dice::hfe::geom::ParticleSystemRenderer::ParticleSystemRenderer "
                       "(Debug type 6, ParticleSystemRenderer.cpp:113: Failed to create declspec)",
                       renderer, 0);
        std::abort();
    }
    psmCtorField<void*>(renderer, 0x1DD0) =
        dx9rend::shaderManagerGetEffect(dx9rend::g_shaderManager, "mods/xpack/shaders/Particles");
    const PsmCtorParameter parameters[] = {
        {0x1DD4, "ViewMat"}, {0x1DD8, "ProjMat"}, {0x1DDC, "HemiMapInfo"},
        {0x1DE0, "HemiShadowAltitude"}, {0x1DEC, "Texture0"}, {0x1DE4, "Texture1"},
        {0x1DE8, "TemplateParameters"}, {0x1DF0, "fogRange"}, {0x1DF4, "fogColor"},
        {0x1E04, "WaterFogRange"}, {0x1E00, "WaterFogColor"}, {0x1E08, "CameraToWater"},
        {0x1DF8, "EffectSunColor"}, {0x1DFC, "EffectShadowColor"}
    };
    psmCtorParameters(renderer, 0x1DD0, parameters);
    psmCtorParticleScratch(renderer, 0x1DC8, 0x1DC0);
    for (unsigned int i = 0; i < 4096; ++i)
        psmCtorTrigPair(i, psmCtorField<short>(renderer, 0x1E14 + i * 4u),
                          psmCtorField<short>(renderer, 0x1E16 + i * 4u));
}

static void psmConstructMeshParticleRenderer(unsigned char* renderer)
{
    psmCtorField<int>(renderer, 0) = -1;
    psmCtorField<void*>(renderer, 4) = 0;
    psmCtorZeroWords(renderer, 8, 256u * 3u);
    psmCtorZeroWords(renderer, 0xC08, 256u * 3u);
    psmCtorField<unsigned int>(renderer, 0x180C) = 0;
    psmCtorField<unsigned int>(renderer, 0x1810) = 0;
    psmCtorField<unsigned int>(renderer, 0x1C14) = 0;
    renderer[0x1C18] = 0;
    psmCtorZeroWords(renderer, 0x1C1C, 6);
    psmCtorMeshMatrices(renderer + 0x1C3C);
    renderer[0x1D3C] = 0;
    psmCtorField<unsigned int>(renderer, 0x1D40) = 0;
    renderer[0x1D48] = 0;
    renderer[0x1D49] = 0;
    psmCtorField<unsigned int>(renderer, 0x1D50) = 0;
    psmCtorField<unsigned int>(renderer, 0x1D54) = 0;
    for (unsigned int i = 0; i < 4; ++i)
        psmCtorField<float>(renderer, 0x1D5C + 4u * i) = 0.5f;
    renderer[0x1D6E] = 0;
    renderer[0x1D6F] = 0;
    renderer[0x1D6C] = 1;
    renderer[0x1D6D] = 1;
    psmCtorZeroWords(renderer, 0x1D84, 3);
    psmCtorZeroWords(renderer, 0x1814, 256);
}

// The complete three-slot table at 1021F058. There is no virtual destructor. Lost and
// destroy are the same attested nullsub_15 (10037D40); reset10172B00 fills all three IBs.
class PsmTrailDeviceListener : public dx9rend::DeviceLostResetListener
{
public:
    void onDeviceLost() override {}
    void onDeviceReset() override
    {
        for (unsigned int sides = 1; sides <= 3; ++sides)
            psmFillTrailIndices(reinterpret_cast<unsigned char*>(this), sides);
    }
    void onDeviceDestroy() override {}
};
static_assert(sizeof(PsmTrailDeviceListener) == 4, "TrailRenderer's listener is one vptr");

static void psmConstructTrailRenderer(unsigned char* renderer)
{
    PsmTrailDeviceListener* const listener = new (renderer) PsmTrailDeviceListener;
    psmCtorField<int>(renderer, 4) = -1;
    psmCtorPointerVector(renderer + 8);
    psmCtorField<void*>(renderer, 0x18) = 0;
    psmCtorZeroWords(renderer, 0x1C, 64u * 3u);
    psmCtorField<unsigned int>(renderer, 0x320) = 0;
    psmCtorField<unsigned int>(renderer, 0x724) = 0;
    renderer[0x728] = 0;
    psmCtorZeroWords(renderer, 0x72C, 6);
    psmCtorMeshMatrices(renderer + 0x74C);
    psmCtorField<float>(renderer, 0x868) = 300.0f;
    renderer[0x84C] = 0;
    psmCtorField<unsigned int>(renderer, 0x850) = 0;
    renderer[0x858] = 0;
    psmCtorField<unsigned int>(renderer, 0x860) = 0;
    psmCtorField<unsigned int>(renderer, 0x864) = 0;
    for (unsigned int i = 0; i < 4; ++i)
        psmCtorField<float>(renderer, 0x874 + 4u * i) = 0.5f;
    renderer[0x884] = 1;
    psmCtorField<float>(renderer, 0x86C) =
        psmCtorField<float>(renderer, 0x868) * psmCtorField<float>(renderer, 0x868);
    renderer[0x885] = 1;
    renderer[0x886] = 0;
    renderer[0x887] = 0;
    psmCtorField<unsigned int>(renderer, 0x8AC) = 0;
    psmCtorField<void*>(renderer, 0x8B4) = 0;
    psmCtorZeroWords(renderer, 0x8D8, 15);
    psmCtorPrimePointerVector(renderer + 8);
    psmCtorZeroWords(renderer, 0x324, 256);
    psmCtorZeroWords(renderer, 0x8B8, 4);
    psmCtorField<void*>(renderer, 0x8D8) =
        dx9rend::shaderManagerGetEffect(dx9rend::g_shaderManager, "mods/xpack/shaders/Trail");
    const D3DVERTEXELEMENT9 declaration[] = {
        {0, 0x00, 2, 0, 0, 0}, {0, 0x0C, 2, 0, 3, 0},
        {0, 0x18, 2, 0, 3, 1}, {0, 0x24, 3, 0, 5, 0},
        {0, 0x34, 7, 0, 5, 1}, {0, 0x3C, 1, 0, 5, 2}, D3DDECL_END()
    };
    psmCtorField<void*>(renderer, 0x910) = dx9rend::vertexDeclarationCreate(declaration);
    const PsmCtorParameter parameters[] = {
        {0x8E4, "ViewMat"}, {0x8E8, "ProjMat"}, {0x8EC, "HemiMapInfo"},
        {0x8F0, "HemiShadowAltitude"}, {0x8F8, "Texture0"}, {0x8FC, "Texture1"},
        {0x900, "EyePos"}, {0x904, "TemplateParameters"}, {0x8F4, "FresnelOffset"},
        {0x8DC, "fogRange"}, {0x8E0, "fogColor"}, {0x908, "EffectSunColor"},
        {0x90C, "EffectShadowColor"}
    };
    psmCtorParameters(renderer, 0x8D8, parameters);
    dx9rend::registerDeviceLostResetListener(listener, 4);
}

static void psmConstructNsaParticleRenderer(unsigned char* renderer)
{
    renderer[0] = 0;
    psmCtorField<unsigned int>(renderer, 4) = 0;
    psmCtorField<int>(renderer, 8) = -1;
    psmCtorPointerVector(renderer + 0xC);
    psmCtorField<void*>(renderer, 0x1C) = 0;
    psmCtorField<unsigned int>(renderer, 0x420) = 0;
    renderer[0x424] = 0;
    psmCtorZeroWords(renderer, 0x428, 6);
    psmCtorField<unsigned int>(renderer, 0x444) = 0;
    psmCtorParticleMatrices(renderer + 0x448);
    renderer[0x518] = 0;
    psmCtorField<unsigned int>(renderer, 0x51C) = 0;
    renderer[0x520] = 0;
    psmCtorField<unsigned int>(renderer, 0x528) = 0;
    psmCtorField<unsigned int>(renderer, 0x52C) = 0;
    for (unsigned int i = 0; i < 4; ++i)
        psmCtorField<float>(renderer, 0x534 + 4u * i) = 0.5f;
    renderer[0x544] = 1;
    renderer[0x545] = 0;
    renderer[0x546] = 0;
    psmCtorField<void*>(renderer, 0x55C) = 0;
    psmCtorField<void*>(renderer, 0x564) = 0;
    psmCtorField<void*>(renderer, 0x568) = 0;
    psmCtorPrimePointerVector(renderer + 0xC);
    psmCtorZeroWords(renderer, 0x20, 256);

    const D3DVERTEXELEMENT9 declaration[] = {
        {0, 0x00, 2, 0, 0, 0}, {0, 0x0C, 1, 0, 5, 0},
        {0, 0x14, 2, 0, 5, 1}, {0, 0x20, 2, 0, 5, 2},
        {0, 0x2C, 1, 0, 5, 3}, {0, 0x34, 7, 0, 5, 4},
        {0, 0x3C, 7, 0, 5, 5}, {0, 0x44, 6, 0, 5, 6}, D3DDECL_END()
    };
    psmCtorField<void*>(renderer, 0x59C) = dx9rend::vertexDeclarationCreate(declaration);
    psmCtorField<void*>(renderer, 0x56C) = dx9rend::shaderManagerGetEffect(
        dx9rend::g_shaderManager, "mods/xpack/shaders/NonScreenAlignedParticles");
    const PsmCtorParameter parameters[] = {
        {0x570, "ViewMat"}, {0x574, "ProjMat"}, {0x578, "HemiMapInfo"},
        {0x57C, "HemiShadowAltitude"}, {0x588, "Texture0"}, {0x580, "Texture1"},
        {0x584, "TemplateParameters"}, {0x58C, "fogRange"}, {0x590, "fogColor"},
        {0x594, "EffectSunColor"}, {0x598, "EffectShadowColor"}
    };
    psmCtorParameters(renderer, 0x56C, parameters);
    psmCtorParticleScratch(renderer, 0x564, 0x55C);
}
