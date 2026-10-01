// BF2Audio reverb implementation; excluded from host images to keep DLL-owned state separate.
#if defined(BF2_IMAGE_BF2AUDIO)
#include "Reverb.h"
#include "OpenALProvider.h"
#include "SoundTrace.h"

#include "../../../RendDX9Abi/BfMemory.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace dice {
namespace hfe {

namespace {
const char* const kFile = "C:\\dice\\Projects\\BF2Branches\\Patch_1_50\\Code\\BF2\\Sound\\SoundEngine\\Reverb.cpp";
}

Reverb::Reverb()
{
    std::memset(&m_preset, 0, sizeof(m_preset));
    std::memset(&m_current, 0, sizeof(m_current));
    m_dirty = true;
}

// The preset file is plain text, one value per line, read with strtok on
// '\n' in exactly this order: two header lines, then for each of the 26 parameters a label line
// followed by its value line - except the two pan vectors, whose label is followed by three value
// lines. Whitespace-only lines vanish under strtok, so a blank line would shift everything; the
// shipped presets have none. A file of 2,048 bytes or more is refused - and on that path the
// handle is not closed, as shipped.
bool Reverb::load(const char* name)
{
    if (!g_soundFileIO)
        return false;
    void* handle = g_soundFileIO->open(name);
    if (!handle)
    {
        g_soundTrace->warning(sndFormat("Reverb preset file %s not found.", name), kFile, 31);
        return false;
    }
    const int size = g_soundFileIO->seek(handle, 0, 2);
    g_soundFileIO->seek(handle, 0, 0);
    if (size >= 2048)
        return false;
    char* text = (char*) renddx9abi::engineAlloc(size + 2);
    g_soundFileIO->read(handle, text, size);
    text[size]     = '\n';
    text[size + 1] = 0;
    EAXLISTENERPROPERTIES& p = m_preset;
    std::strtok(text, "\n");
    std::strtok(0, "\n");
    p.ulEnvironment          = (unsigned long) std::atol(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.flEnvironmentSize      = (float) std::atof(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.flEnvironmentDiffusion = (float) std::atof(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.lRoom                  = std::atol(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.lRoomHF                = std::atol(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.lRoomLF                = std::atol(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.flDecayTime            = (float) std::atof(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.flDecayHFRatio         = (float) std::atof(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.flDecayLFRatio         = (float) std::atof(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.lReflections           = std::atol(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.flReflectionsDelay     = (float) std::atof(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.vReflectionsPan.x      = (float) std::atof(std::strtok(0, "\n"));
                          p.vReflectionsPan.y      = (float) std::atof(std::strtok(0, "\n"));
                          p.vReflectionsPan.z      = (float) std::atof(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.lReverb                = std::atol(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.flReverbDelay          = (float) std::atof(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.vReverbPan.x           = (float) std::atof(std::strtok(0, "\n"));
                          p.vReverbPan.y           = (float) std::atof(std::strtok(0, "\n"));
                          p.vReverbPan.z           = (float) std::atof(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.flEchoTime             = (float) std::atof(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.flEchoDepth            = (float) std::atof(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.flModulationTime       = (float) std::atof(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.flModulationDepth      = (float) std::atof(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.flAirAbsorptionHF      = (float) std::atof(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.flHFReference          = (float) std::atof(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.flLFReference          = (float) std::atof(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.flRoomRolloffFactor    = (float) std::atof(std::strtok(0, "\n"));
    std::strtok(0, "\n"); p.ulFlags                = (unsigned long) std::atol(std::strtok(0, "\n"));
    std::memcpy(&m_current, &m_preset, sizeof(m_current));
    m_dirty = true;
    renddx9abi::engineFree(text);
    g_soundFileIO->close(handle);
    return true;
}

// 2000 * log10(volume) millibels (-10000 at or below 1e-5) on top of the
// preset's room level, clamped to EAX's [-10000, 0].
void Reverb::setRoomLevel(float volume)
{
    int mb;
    if (volume <= 0.0000099999997f)
        mb = -10000;
    else
        mb = (int) (std::log10(volume) * 2000.0f);
    int room = (int) m_preset.lRoom + mb;
    if (room >= -10000)
        room = room > 0 ? 0 : room;
    else
        room = -10000;
    if (room != m_current.lRoom)
    {
        m_current.lRoom = room;
        m_dirty = true;
    }
}

void Reverb::apply(ISoundProvider* provider)
{
    if (m_dirty)
    {
        provider->updateReverb(&m_current);
        m_dirty = false;
    }
}

} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_BF2AUDIO
