// SCAFFOLDING TEST - not recovered game source and not part of the BF2 target.
// Exercises the independently reconstructable BundleTemplate::TemplateInfo and WeakPtr lifetime
// slice without pretending the still-unrecovered live Bundle/Object boundary is link-complete.

#include "../BF2/Game/Objects/Base/BundleTemplate.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <new>
#include <vector>

using dice::hfe::WeakPtr;
using dice::hfe::getRotationZXY;
using dice::hfe::setRotationZXY;
using dice::hfe::io::INonlinearStream;
using dice::hfe::io::IStream;
using dice::hfe::io::SeekOrigin;
using dice::hfe::world::BundleTemplate;
using dice::hfe::world::IObjectTemplate;

class MemoryStream : public INonlinearStream
{
public:
    explicit MemoryStream(const std::vector<unsigned char>& bytes)
        : m_bytes(bytes), m_position(0), m_refs(1) {}

    virtual void addRef() { ++m_refs; }
    virtual int getRef() { return m_refs; }
    virtual int release() { return --m_refs; }
    virtual void* queryInterface(unsigned int) const { return 0; }

    virtual unsigned int read(void* buffer, unsigned int size)
    {
        const unsigned int available = static_cast<unsigned int>(m_bytes.size() - m_position);
        const unsigned int count = (std::min)(available, size);
        if (count != 0)
            std::memcpy(buffer, &m_bytes[m_position], count);
        m_position += count;
        return count;
    }

    virtual unsigned int write(const void*, unsigned int) { return 0; }
    virtual int skip(unsigned int offset) { m_position += offset; return static_cast<int>(m_position); }
    virtual int getAvailableBytes() { return static_cast<int>(m_bytes.size() - m_position); }
    virtual IStream* clone() { return 0; }
    virtual bool canRead() { return true; }
    virtual bool canWrite() { return false; }
    virtual void flushWrite() {}
    virtual int seek(SeekOrigin, int offset) { m_position = offset; return offset; }
    virtual int getPosition() { return static_cast<int>(m_position); }
    virtual int getSize() { return static_cast<int>(m_bytes.size()); }

private:
    std::vector<unsigned char> m_bytes;
    std::size_t m_position;
    int m_refs;
};

static void appendInt(std::vector<unsigned char>& bytes, int value)
{
    const unsigned char* raw = reinterpret_cast<const unsigned char*>(&value);
    bytes.insert(bytes.end(), raw, raw + sizeof(value));
}

static int fail(const char* message)
{
    std::cerr << "FAIL: " << message << std::endl;
    return 1;
}

static bool near(float lhs, float rhs)
{
    return std::fabs(lhs - rhs) < 0.001f;
}

int main()
{
    BundleTemplate::TemplateInfo info("S_TestChild");

    if (info.m_templateName != "S_TestChild") return fail("template name");
    if (info.m_template.get() != 0) return fail("default weak pointer");
    if (info.m_inputId != -1) return fail("default input id");
    if (info.m_unknown50) return fail("default trailing flag");

    for (int i = 0; i != 16; ++i)
    {
        const float expected = (i == 0 || i == 5 || i == 10 || i == 15) ? 1.0f : 0.0f;
        if (info.m_transformation.m[i] != expected) return fail("identity transform");
    }

    typedef WeakPtr<IObjectTemplate> TemplateWeakPtr;
    TemplateWeakPtr::Ctrl* control =
        static_cast<TemplateWeakPtr::Ctrl*>(::operator new(sizeof(TemplateWeakPtr::Ctrl)));
    control->refCount = 1;
    control->target = reinterpret_cast<IObjectTemplate*>(control); // never dereferenced
    info.m_template.m_ctrl = control;

    {
        TemplateWeakPtr copy(info.m_template);
        if (control->refCount != 2) return fail("copy retain");
        copy = copy;
        if (control->refCount != 2) return fail("self-assignment retain/release order");
        if (copy.get() != control->target) return fail("weak target lookup");
    }

    if (control->refCount != 1) return fail("copy destruction release");
    info.m_template = TemplateWeakPtr();
    if (info.m_template.get() != 0) return fail("null assignment");

    dice::hfe::Mat4 transform;
    for (int i = 0; i != 16; ++i)
        transform.m[i] = static_cast<float>(100 + i);

    setRotationZXY(transform, 25.0f, -15.0f, 40.0f);
    if (transform.m[3] != 103.0f || transform.m[7] != 107.0f ||
        transform.m[11] != 111.0f || transform.m[12] != 112.0f ||
        transform.m[13] != 113.0f || transform.m[14] != 114.0f ||
        transform.m[15] != 115.0f)
    {
        return fail("rotation setter preserves non-rotation slots");
    }

    float z;
    float x;
    float y;
    getRotationZXY(transform, z, x, y);
    if (!near(z, 25.0f) || !near(x, -15.0f) || !near(y, 40.0f))
        return fail("ZXY round trip");

    setRotationZXY(transform, 0.0f, 0.0f, 0.0f);
    getRotationZXY(transform, z, x, y);
    if (!near(z, 0.0f) || !near(x, 0.0f) || !near(y, 0.0f))
        return fail("identity rotation");

    std::vector<unsigned char> encoded;
    appendInt(encoded, 3);
    encoded.push_back('a');
    encoded.push_back(0);
    encoded.push_back('b');
    MemoryStream exact(encoded);
    std::string decoded("old");
    if (!dice::hfe::io::read(&exact, decoded) || decoded.size() != 3 ||
        decoded[0] != 'a' || decoded[1] != 0 || decoded[2] != 'b')
    {
        return fail("length-prefixed string exact read");
    }

    encoded.clear();
    appendInt(encoded, 0);
    MemoryStream empty(encoded);
    decoded = "old";
    if (!dice::hfe::io::read(&empty, decoded) || !decoded.empty())
        return fail("zero-length string");

    encoded.clear();
    appendInt(encoded, -4);
    MemoryStream negative(encoded);
    decoded = "old";
    if (!dice::hfe::io::read(&negative, decoded) || !decoded.empty())
        return fail("negative-length string");

    encoded.clear();
    appendInt(encoded, 3);
    encoded.push_back('a');
    encoded.push_back('b');
    MemoryStream shortPayload(encoded);
    decoded = "old";
    if (dice::hfe::io::read(&shortPayload, decoded) || !decoded.empty())
        return fail("short string payload");

    encoded.clear();
    MemoryStream noLength(encoded);
    decoded = "old";
    if (!dice::hfe::io::read(&noLength, decoded) || !decoded.empty())
        return fail("unchecked length read");

    std::cout << "bundle template core: 32 checks, 0 failures" << std::endl;
    return 0;
}
