// [INFERRED] private split beside ParticleSystemManager.cpp. Complete BF2 1.50 RendDX9
// instruction streams read: 10172FF0, 10173170, 10173A00, 10173CC0 and 10173FA0.
// The dedicated ELF has no corresponding renderer. All offsets are private PE carriers.

// Walk the circular particle array in emission order. Even a rejected
// two-point trail or 65th group writes the temporary table before the admission check.
static void psmCollectTrailParticles(unsigned char* renderer, dx9rend::IRenderView* view,
    unsigned char* instance, unsigned char* descriptor)
{
    view->getViewMatrixPtr();
    view->getNearPlane();
    view->getViewMatrixPtr();
    if (psmDrawField<int>(descriptor, 8) == -1)
        psmDrawField<unsigned int>(descriptor, 8) = psmDrawField<unsigned int>(renderer, 0x854)++;
    ParticleSystemManager* manager = reinterpret_cast<ParticleSystemManager*>(g_ParticleSystemManager);
    const unsigned int lod = psmDrawField<unsigned int>(descriptor, 0x68);
    const float farDistance = manager->m_farDistances[lod];
    const float inverseFarRange = manager->m_inverseFarRanges[lod];
    unsigned int count = 0;
    unsigned int index = psmDrawField<unsigned int>(instance, 0x178)
        & psmDrawField<unsigned int>(instance, 0x17C);
    while (index != psmDrawField<unsigned int>(instance, 0x174))
    {
        unsigned char* particle = psmDrawField<unsigned char**>(instance, 0x14)[index];
        const double depth = static_cast<double>(psmDrawField<float>(particle, 0x14)) * psmDrawField<float>(renderer, 0x738)
            + static_cast<double>(psmDrawField<float>(particle, 0x18)) * psmDrawField<float>(renderer, 0x73C)
            + static_cast<double>(psmDrawField<float>(renderer, 0x734)) * psmDrawField<float>(particle, 0x10)
            + psmDrawField<float>(renderer, 0x740);
        if (!(depth > farDistance)) // PE test ah,41h includes unordered input in the unfaded arm.
            psmDrawField<float>(particle, 0x7C) = psmDrawField<float>(particle, 0x88);
        else
        {
            double fade = (depth - farDistance) * inverseFarRange;
            if (fade > 1) fade = 1;
            psmDrawField<float>(particle, 0x7C) = static_cast<float>((1 - fade) * (1 - fade)
                * psmDrawField<float>(particle, 0x88));
        }
        psmDrawField<unsigned short>(particle, 0x86) = psmDrawField<unsigned short>(descriptor, 4);
        psmDrawField<unsigned char**>(renderer, 0x18)[psmDrawField<unsigned int>(renderer, 0x31C) + count] = particle;
        index = (index + 1) & psmDrawField<unsigned int>(instance, 0x17C);
        ++count;
    }
    if (count > 2 && psmDrawField<unsigned int>(renderer, 0x320) < 64)
    {
        const unsigned int group = psmDrawField<unsigned int>(renderer, 0x320);
        psmDrawField<unsigned char*>(renderer, 0x1C + 12 * group) = instance;
        psmDrawField<unsigned int>(renderer, 0x20 + 12 * group) = psmDrawField<unsigned int>(renderer, 0x31C) + count - 1;
        psmDrawField<unsigned int>(renderer, 0x24 + 12 * group) = count;
        ++psmDrawField<unsigned int>(renderer, 0x320);
        psmDrawField<unsigned int>(renderer, 0x31C) += count;
    }
}

static void psmBindTrailParameters(unsigned char* renderer)
{
    psmParticleParameter(renderer, 0x8E4)->setValue(renderer + 0x74C, 0);
    psmParticleParameter(renderer, 0x8E8)->setValue(renderer + 0x80C, 0);
    HemiMap* map = g_HemiMapManager->getBaseHemiMap();
    if (map)
    {
        psmParticleParameter(renderer, 0x8EC)->setValue(&g_HemiMapManager->getHemiMapInfo(map), 0);
        psmParticleParameter(renderer, 0x8F0)->setFloat(map->falloff);
        psmParticleParameter(renderer, 0x8FC)->setValue(map->texture, 0);
    }
    else
    {
        HemiMap* fallback = g_HemiMapManager->getBaseHemiMap(); // PE 100F1490 rereads the owner.
        psmParticleParameter(renderer, 0x8FC)->setValue(fallback ? fallback->texture : 0, 0);
    }
    psmParticleParameter(renderer, 0x908)->setValue(&g_LightManager->getEffectSunColor(), 0);
    psmParticleParameter(renderer, 0x90C)->setValue(&g_LightManager->getEffectShadowColor(), 0);
    ParticleSystemManager* manager = reinterpret_cast<ParticleSystemManager*>(g_ParticleSystemManager);
    if (!manager->m_shaderQuality)
    {
        const float scale = 1.0f / manager->m_value8CA4;
        const Vec3 color = { scale, scale, scale };
        psmParticleParameter(renderer, 0x908)->setValue(&color, 0);
    }
    psmParticleParameter(renderer, 0x8DC)->setValue(static_cast<unsigned char*>(dx9rend::g_raShaderConstants) + 0xA8, 0);
    psmParticleParameter(renderer, 0x8E0)->setValue(static_cast<unsigned char*>(dx9rend::g_raShaderConstants) + 0xBC, 0);
    manager = reinterpret_cast<ParticleSystemManager*>(g_ParticleSystemManager);
    static const char* const techniques[] = { "TrailShowFill", "TrailLow", "TrailMedium", "TrailHigh" };
    if (manager->m_shaderQuality <= 3)
        dx9rend::effectSetTechniqueByName(psmDrawField<void*>(renderer, 0x8D8), vc71::string(techniques[manager->m_shaderQuality]));
    dx9rend::vertexDeclarationBind(psmDrawField<void*>(renderer, 0x910));
    dx9rend::vertexBufferBindStream(psmDrawField<void*>(renderer, 0x8B4), 0);
    dx9rend::effectBegin(psmDrawField<void*>(renderer, 0x8D8));
    dx9rend::effectBeginPass(psmDrawField<void*>(renderer, 0x8D8), 0);
}

// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: particle-trail-draw-debug-dispatch]
// Error-only dependency on the retail Debug type-6 dispatcher, TrailRenderer.cpp:647.
static void psmTrailDrawDebug()
{
    std::fprintf(stderr, "[FRONTIER] dice::hfe::geom::TrailRenderer::draw "
        "(Debug type 6, TrailRenderer.cpp:647: LockFailed) reached\n");
    std::fflush(stderr);
    std::abort();
}

// The view/target parameters are unused. The selected group is consumed
// in reverse order, with a separate prebuilt index buffer for each number of ribbon planes.
static bool psmDrawTrailGroup(unsigned char* renderer, dx9rend::IRenderView*, unsigned int, unsigned int group)
{
    unsigned int& remaining = psmDrawField<unsigned int>(renderer, 0x24 + 12 * group);
    const unsigned int count = remaining;
    if (!count) return false;
    unsigned char* instance = psmDrawField<unsigned char*>(renderer, 0x1C + 12 * group);
    const unsigned int end = psmDrawField<unsigned int>(renderer, 0x20 + 12 * group);
    unsigned char* descriptor = reinterpret_cast<unsigned char*>(
        reinterpret_cast<IParticleSystemInstance*>(instance)->getTemplate());
    const unsigned int verticesPerParticle = 2 * psmDrawField<unsigned int>(descriptor, 0x220);
    dx9rend::indexBufferBind(psmDrawField<void*>(renderer, 0x8B8 + 4 * psmDrawField<unsigned int>(descriptor, 0x220)));
    psmParticleParameter(renderer, 0x904)->setValue(descriptor + 0xAC, 0);
    psmParticleParameter(renderer, 0x8F8)->setValue(psmDrawField<void*>(descriptor, 0x21C), 0);
    const float fresnel = psmDrawField<unsigned int>(descriptor, 0x220) == 1 ? 1.0f : 0.0f;
    psmParticleParameter(renderer, 0x8F4)->setValue(&fresnel, 0);
    const unsigned int offset = psmDrawField<unsigned int>(renderer, 0x8B0);
    unsigned char* output = static_cast<unsigned char*>(dx9rend::vertexBufferLock(
        psmDrawField<void*>(renderer, 0x8B4), offset, count * verticesPerParticle, offset ? 6144 : 10240));
    if (!output) psmTrailDrawDebug();
    unsigned int vertices = 0;
    for (unsigned int i = 0; i < count; ++i)
    {
        unsigned char* particle = psmDrawField<unsigned char**>(renderer, 0x18)[end - i];
        for (unsigned int plane = 0; plane < psmDrawField<unsigned int>(descriptor, 0x220); ++plane)
        {
            for (unsigned int side = 0; side < 2; ++side)
            {
                unsigned char* vertex = output + vertices++ * 68;
                ::memcpy(vertex, particle + 0x10, 12);
                ::memcpy(vertex + 0xC, particle + 0x1C + 24 * plane + 12 * side, 12);
                ::memcpy(vertex + 0x18, particle + 0x64, 12);
                psmDrawField<float>(vertex, 0x24) = static_cast<float>(static_cast<double>(
                    psmDrawField<float>(particle, 0x70)) + psmDrawField<float>(particle, 0x74));
                psmDrawField<unsigned int>(vertex, 0x28) = psmDrawField<unsigned int>(particle, 0x78);
                psmDrawField<unsigned int>(vertex, 0x2C) = psmDrawField<unsigned int>(instance, 0x18C);
                psmDrawField<unsigned int>(vertex, 0x30) = psmDrawField<unsigned int>(particle, 0x7C);
                psmDrawField<unsigned int>(vertex, 0x34) = psmDrawField<unsigned int>(instance, 0x180);
                psmDrawField<unsigned int>(vertex, 0x38) = psmDrawField<unsigned int>(instance, 0x184);
                psmDrawField<unsigned int>(vertex, 0x3C) = psmDrawField<unsigned int>(particle, 0x80);
                psmDrawField<float>(vertex, 0x40) = side ? -1.0f : 1.0f;
            }
        }
        --remaining;
        ++psmDrawField<unsigned int>(renderer, 0x724);
    }
    dx9rend::vertexBufferUnlock(psmDrawField<void*>(renderer, 0x8B4));
    psmCountParticleDraw(0x8C50);
    dx9rend::rendererDrawIndexedPrimitive(2 * psmDrawField<unsigned int>(descriptor, 0x220) * (count - 1),
        4, psmDrawField<int>(renderer, 0x8B0), 0, vertices, 0);
    psmDrawField<unsigned int>(renderer, 0x8B0) += vertices;
    psmDrawField<unsigned int>(renderer, 0x20 + 12 * group) = end - count;
    return true;
}

// Empty/disabled input leaves all retained state untouched.
static void psmPrepareTrailRenderer(unsigned char* renderer, dx9rend::IRenderView* view,
    ParticleSystemInstanceList* instances, float deltaTime)
{
    const unsigned int count = instances->size();
    if (!count || !renderer[0x884]) return;
    psmDrawField<unsigned int>(renderer, 0x85C) = 0;
    renderer[0x84C] = 0;
    psmDrawClearPointerVector(renderer + 8);
    psmDrawField<unsigned int>(renderer, 0x724) = 0;
    psmDrawField<unsigned int>(renderer, 0x31C) = 0;
    psmDrawField<unsigned int>(renderer, 0x320) = 0;
    psmDrawField<float>(renderer, 0x730) = deltaTime;
    const Mat4* matrix = view->getInverseViewMatrixPtr(); // primary slot +68
    for (unsigned int i = 0; i < 4; ++i)
        psmDrawField<float>(renderer, 0x734 + 4 * i) = matrix->m[2 + 4 * i];
    for (unsigned int i = 0; i < count; ++i)
    {
        if (instances->first[i]->reserved01() != 2) continue;
        unsigned char* instance = reinterpret_cast<unsigned char*>(instances->first[i]);
        if (!instance[0x140] && !psmDrawField<unsigned int>(instance, 0x28)) continue;
        const float* bounds = reinterpret_cast<float*>(instance + 0xF8);
        Vec3 center;
        center.x = static_cast<float>((static_cast<double>(bounds[0]) + bounds[3]) * 0.5);
        center.y = static_cast<float>((static_cast<double>(bounds[4]) + bounds[1]) * 0.5);
        const float sumZ = bounds[5] + bounds[2];
        center.z = sumZ * 0.5f;
        const float halfX = (bounds[3] - bounds[0]) * 0.5f;
        const double halfY = (static_cast<double>(bounds[4]) - bounds[1]) * 0.5;
        const float deltaZ = bounds[5] - bounds[2];
        const double halfZ = static_cast<double>(deltaZ) * 0.5;
        const float radius = static_cast<float>(std::sqrt(halfZ * halfZ
            + static_cast<double>(halfX) * halfX + halfY * halfY));
        if (!view->getFrustumPtr()->inside(center, radius)) continue;
        void* owner = psmDrawField<void*>(instance, 0x10);
        if (owner && psmOwnerRootDisabled(owner)) continue;             // IObject m_flags bit 0 (RW-266)
        psmDrawField<unsigned int>(instance, 0x120) =
            static_cast<dx9rend::IRenderer*>(dx9rend::g_rendererBase)->getFrameOrdinal();
        instance[0x156] = 1;
        if (psmDrawField<unsigned int>(instance, 0x28) >= 2)
        {
            psmCollectTrailParticles(renderer, view, instance, psmDrawField<unsigned char*>(instance, 0x2C));
            if (renderer[0x886])
            {
                // [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: particle-trail-debug-bounds]
                std::fprintf(stderr, "[FRONTIER] dice::hfe::geom::TrailRenderer::prepare "
                    "(debug bounding mesh, PE 10173BA0) reached\n");
                std::fflush(stderr);
                std::abort();
            }
        }
    }
    const unsigned int threshold = static_cast<unsigned int>(static_cast<int>(
        static_cast<double>(psmDrawField<unsigned int>(renderer, 0x8AC)) * 0.85000002f));
    renderer[0x728] = psmDrawField<unsigned int>(renderer, 0x724) > threshold;
}

// The active wrapper starts with true and ORs group results into it,
// including empty groups. The eligibility flag is retained on both early exits.
static bool psmDrawTrailRenderer(unsigned char* renderer, dx9rend::IRenderView* view)
{
    if (!renderer[0x884] || !renderer[0x858]) return false;
    // [CHOSEN, not recovered] As with existing draw wrappers, profiler registration/timing
    // is omitted. All render state, repeated view calls and geometry ordering are retained.
    dx9rend::stateManagerPush();
    if (!renderer[0x84C])
    {
        psmDrawField<Mat4>(renderer, 0x80C) = *view->getProjectionMatrixPtr();
        const Mat4* projection = view->getProjectionMatrixPtr();
        const Mat4* inverse = view->getInverseViewMatrixPtr();
        psmDrawField<Mat4>(renderer, 0x7CC).fullMult(*inverse, *projection);
        renderer[0x84C] = 1;
    }
    psmDrawField<Mat4>(renderer, 0x74C) = *view->getInverseViewMatrixPtr();
    psmParticleParameter(renderer, 0x900)->setValue(view->getViewMatrixPtr()->m + 12, 0);
    bool drawn = true;
    if (renderer[0x858])
    {
        psmDrawField<unsigned int>(renderer, 0x8B0) = 0;
        psmBindTrailParameters(renderer);
        for (unsigned int i = 0; i < psmDrawField<unsigned int>(renderer, 0x320); ++i)
            drawn = psmDrawTrailGroup(renderer, view, psmDrawField<unsigned int>(renderer, 0x85C), i) || drawn;
        dx9rend::effectEnd(psmDrawField<void*>(renderer, 0x8D8));
        renderer[0x858] = 0;
    }
    dx9rend::stateManagerPop();
    return drawn;
}
