// Code/Tools/BF2Editor/Framework/DdsImage.cpp - see DdsImage.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/DdsImage.h"

#include <cstring>

namespace bf2editor
{

namespace
{

const unsigned kFourCcDxt1 = 0x31545844;            // "DXT1"
const unsigned kFourCcDxt3 = 0x33545844;
const unsigned kFourCcDxt5 = 0x35545844;
const unsigned kPixelFourCc = 0x4;
const unsigned kPixelRgb = 0x40;

unsigned u32(const unsigned char* p)
{
    return static_cast<unsigned>(p[0]) | (static_cast<unsigned>(p[1]) << 8) | (static_cast<unsigned>(p[2]) << 16)
         | (static_cast<unsigned>(p[3]) << 24);
}

// One channel by its mask, widened to 8 bits; `none` when the mask is empty.
unsigned channel(unsigned value, unsigned mask, unsigned none)
{
    if (!mask)
        return none;
    unsigned shift = 0;
    while (!((mask >> shift) & 1u))
        ++shift;
    const unsigned max = mask >> shift;
    const unsigned c = (value & mask) >> shift;
    return (c * 255u + max / 2u) / max;
}

unsigned expand565(unsigned c)
{
    const unsigned r = (c >> 11) & 31u, g = (c >> 5) & 63u, b = c & 31u;
    return ((r * 255u + 15u) / 31u << 16) | ((g * 255u + 31u) / 63u << 8) | ((b * 255u + 15u) / 31u);
}

unsigned mix(unsigned a, unsigned b, unsigned wa, unsigned wb, unsigned total)
{
    unsigned out = 0;
    for (int s = 0; s < 24; s += 8)
        out |= ((((a >> s) & 255u) * wa + ((b >> s) & 255u) * wb) / total) << s;
    return out;
}

// A DXT colour block's four colours (rgb only); `threeColour` gives the 1-bit-alpha mode of DXT1.
void blockColours(const unsigned char* block, bool allowThree, unsigned colours[4], bool& transparentLast)
{
    const unsigned c0 = block[0] | (block[1] << 8);
    const unsigned c1 = block[2] | (block[3] << 8);
    colours[0] = expand565(c0);
    colours[1] = expand565(c1);
    transparentLast = false;
    if (c0 > c1 || !allowThree)
    {
        colours[2] = mix(colours[0], colours[1], 2, 1, 3);
        colours[3] = mix(colours[0], colours[1], 1, 2, 3);
    }
    else
    {
        colours[2] = mix(colours[0], colours[1], 1, 1, 2);
        colours[3] = 0;
        transparentLast = true;
    }
}

} // namespace

bool DdsImage::read(const std::string& bytes, std::string& error)
{
    const unsigned char* const b = reinterpret_cast<const unsigned char*>(bytes.data());
    if (bytes.size() < 128 || std::memcmp(b, "DDS ", 4) != 0)
    {
        error = "not a DDS file";
        return false;
    }
    height = static_cast<int>(u32(b + 12));
    width = static_cast<int>(u32(b + 16));
    const unsigned flags = u32(b + 80), fourCc = u32(b + 84), bits = u32(b + 88);
    const unsigned rMask = u32(b + 92), gMask = u32(b + 96), bMask = u32(b + 100), aMask = u32(b + 104);
    if (width <= 0 || height <= 0 || width > 8192 || height > 8192)
    {
        error = "an implausible size";
        return false;
    }
    pixels.assign(static_cast<size_t>(width) * static_cast<size_t>(height), 0u);
    const unsigned char* data = b + 128;
    const size_t available = bytes.size() - 128;
    if (flags & kPixelFourCc)
    {
        const bool dxt1 = fourCc == kFourCcDxt1;
        if (!dxt1 && fourCc != kFourCcDxt3 && fourCc != kFourCcDxt5)
        {
            error = "a compressed format other than DXT1/3/5";
            return false;
        }
        const int blocksX = (width + 3) / 4, blocksY = (height + 3) / 4;
        const size_t blockBytes = dxt1 ? 8 : 16;
        if (available < static_cast<size_t>(blocksX) * blocksY * blockBytes)
        {
            error = "the file ends early";
            return false;
        }
        for (int by = 0; by < blocksY; ++by)
        {
            for (int bx = 0; bx < blocksX; ++bx)
            {
                const unsigned char* block = data + (static_cast<size_t>(by) * blocksX + bx) * blockBytes;
                unsigned alpha[16];
                for (int i = 0; i < 16; ++i)
                    alpha[i] = 255u;
                if (fourCc == kFourCcDxt3)
                {
                    for (int i = 0; i < 16; ++i)
                        alpha[i] = ((block[i / 2] >> ((i & 1) * 4)) & 15u) * 17u;
                    block += 8;
                }
                else if (fourCc == kFourCcDxt5)
                {
                    const unsigned a0 = block[0], a1 = block[1];
                    unsigned table[8] = { a0, a1, 0, 0, 0, 0, 0, 255u };
                    if (a0 > a1)
                        for (int i = 2; i < 8; ++i)
                            table[i] = (a0 * (8 - i) + a1 * (i - 1)) / 7;
                    else
                    {
                        for (int i = 2; i < 6; ++i)
                            table[i] = (a0 * (6 - i) + a1 * (i - 1)) / 5;
                        table[6] = 0;
                        table[7] = 255u;
                    }
                    unsigned long long index = 0;
                    for (int i = 0; i < 6; ++i)
                        index |= static_cast<unsigned long long>(block[2 + i]) << (8 * i);
                    for (int i = 0; i < 16; ++i)
                        alpha[i] = table[(index >> (3 * i)) & 7u];
                    block += 8;
                }
                unsigned colours[4];
                bool transparentLast = false;
                blockColours(block, dxt1, colours, transparentLast);
                const unsigned indices = u32(block + 4);
                for (int i = 0; i < 16; ++i)
                {
                    const int x = bx * 4 + (i & 3), y = by * 4 + (i >> 2);
                    if (x >= width || y >= height)
                        continue;
                    const unsigned k = (indices >> (2 * i)) & 3u;
                    const unsigned a = (dxt1 && transparentLast && k == 3) ? 0u : alpha[i];
                    pixels[static_cast<size_t>(y) * width + x] = (a << 24) | colours[k];
                }
            }
        }
        return true;
    }
    if (!(flags & kPixelRgb) || (bits != 16 && bits != 24 && bits != 32))
    {
        error = "a pixel format other than 16-, 24- or 32-bit RGB";
        return false;
    }
    const size_t step = bits / 8;
    if (available < static_cast<size_t>(width) * height * step)
    {
        error = "the file ends early";
        return false;
    }
    for (size_t i = 0; i < pixels.size(); ++i)
    {
        const unsigned char* p = data + i * step;
        unsigned v = 0;
        for (size_t k = 0; k < step; ++k)
            v |= static_cast<unsigned>(p[k]) << (8 * k);
        pixels[i] = (channel(v, aMask, 255u) << 24) | (channel(v, rMask, 0u) << 16) | (channel(v, gMask, 0u) << 8)
                  | channel(v, bMask, 0u);
    }
    return true;
}

DdsImage DdsImage::scaled(int w, int h) const
{
    DdsImage out;
    out.width = w;
    out.height = h;
    out.pixels.assign(static_cast<size_t>(w) * static_cast<size_t>(h), 0u);
    if (width <= 0 || height <= 0)
        return out;
    for (int y = 0; y < h; ++y)
    {
        float sy = (static_cast<float>(y) + 0.5f) * static_cast<float>(height) / static_cast<float>(h) - 0.5f;
        if (sy < 0.0f)
            sy = 0.0f;
        const int y0 = static_cast<int>(sy);
        const int y1 = y0 + 1 < height ? y0 + 1 : y0;
        const float fy = sy - static_cast<float>(y0);
        for (int x = 0; x < w; ++x)
        {
            float sx = (static_cast<float>(x) + 0.5f) * static_cast<float>(width) / static_cast<float>(w) - 0.5f;
            if (sx < 0.0f)
                sx = 0.0f;
            const int x0 = static_cast<int>(sx);
            const int x1 = x0 + 1 < width ? x0 + 1 : x0;
            const float fx = sx - static_cast<float>(x0);
            const unsigned p00 = pixels[static_cast<size_t>(y0) * width + x0], p01 = pixels[static_cast<size_t>(y0) * width + x1];
            const unsigned p10 = pixels[static_cast<size_t>(y1) * width + x0], p11 = pixels[static_cast<size_t>(y1) * width + x1];
            unsigned v = 0;
            for (int s = 0; s < 32; s += 8)
            {
                const float top = ((p00 >> s) & 255u) * (1.0f - fx) + ((p01 >> s) & 255u) * fx;
                const float bottom = ((p10 >> s) & 255u) * (1.0f - fx) + ((p11 >> s) & 255u) * fx;
                const unsigned c = static_cast<unsigned>(top * (1.0f - fy) + bottom * fy + 0.5f);
                v |= (c > 255u ? 255u : c) << s;
            }
            out.pixels[static_cast<size_t>(y) * w + x] = v;
        }
    }
    return out;
}

std::string DdsImage::write() const
{
    unsigned char header[128];
    std::memset(header, 0, sizeof(header));
    std::memcpy(header, "DDS ", 4);
    const auto put = [&header](int offset, unsigned value) {
        header[offset] = static_cast<unsigned char>(value);
        header[offset + 1] = static_cast<unsigned char>(value >> 8);
        header[offset + 2] = static_cast<unsigned char>(value >> 16);
        header[offset + 3] = static_cast<unsigned char>(value >> 24);
    };
    put(4, 124);                                    // dwSize
    put(8, 0x100Fu);                                // caps, height, width, pitch, pixel format
    put(12, static_cast<unsigned>(height));
    put(16, static_cast<unsigned>(width));
    put(20, static_cast<unsigned>(width) * 4u);     // pitch
    put(28, 1);                                     // one level
    put(76, 32);                                    // the pixel format: A8R8G8B8
    put(80, kPixelRgb | 0x1u);
    put(88, 32);
    put(92, 0x00FF0000u);
    put(96, 0x0000FF00u);
    put(100, 0x000000FFu);
    put(104, 0xFF000000u);
    put(108, 0x1000u);                              // a texture
    std::string out(reinterpret_cast<const char*>(header), sizeof(header));
    out.reserve(out.size() + pixels.size() * 4);
    for (size_t i = 0; i < pixels.size(); ++i)
    {
        const unsigned v = pixels[i];
        const char texel[4] = { static_cast<char>(v), static_cast<char>(v >> 8), static_cast<char>(v >> 16),
                                static_cast<char>(v >> 24) };
        out.append(texel, 4);
    }
    return out;
}

} // namespace bf2editor
