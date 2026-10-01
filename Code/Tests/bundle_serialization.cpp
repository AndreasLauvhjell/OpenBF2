// SCAFFOLDING TEST - not recovered game source and not part of the BF2 target.
// Exercises BundleTemplate's recovered wire format through a minimal derived harness. The class
// supplies only still-unreconstructed pure/virtual runtime hooks; none is used by this test.

#include "../BF2/Game/Objects/Base/BundleTemplate.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

using dice::hfe::IBase;
using dice::hfe::Mat4;
using dice::hfe::io::INonlinearStream;
using dice::hfe::io::IStream;
using dice::hfe::io::SeekOrigin;
using dice::hfe::world::BundleTemplate;
using dice::hfe::world::IObject;
using dice::hfe::world::ObjectTemplate;

class MemoryStream : public INonlinearStream
{
public:
    MemoryStream() : m_position(0), m_refs(1) {}
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
        if (count)
            std::memcpy(buffer, &m_bytes[m_position], count);
        m_position += count;
        return count;
    }
    virtual unsigned int write(const void* buffer, unsigned int size)
    {
        const unsigned char* first = static_cast<const unsigned char*>(buffer);
        m_bytes.insert(m_bytes.end(), first, first + size);
        m_position += size;
        return size;
    }
    virtual int skip(unsigned int offset) { m_position += offset; return static_cast<int>(m_position); }
    virtual int getAvailableBytes() { return static_cast<int>(m_bytes.size() - m_position); }
    virtual IStream* clone() { return 0; }
    virtual bool canRead() { return true; }
    virtual bool canWrite() { return true; }
    virtual void flushWrite() {}
    virtual int seek(SeekOrigin, int offset) { m_position = offset; return offset; }
    virtual int getPosition() { return static_cast<int>(m_position); }
    virtual int getSize() { return static_cast<int>(m_bytes.size()); }

    const std::vector<unsigned char>& bytes() const { return m_bytes; }

private:
    std::vector<unsigned char> m_bytes;
    std::size_t m_position;
    int m_refs;
};

class TestBundle : public BundleTemplate
{
public:
    void setBundleFlag(bool value) { m_explodePartAtDestroy = value; }
    bool getBundleFlag() const { return m_explodePartAtDestroy; }

    virtual IObject* createObject() const { return 0; }
    virtual bool makeScript(IStream*) { return false; }
    virtual ObjectTemplate* addSoundTemplate(const std::string&) { return 0; }
    virtual void addBundleChilds(IObject*) const {}
    virtual void addDriverDecals(IObject*) const {}
    virtual void setPhysicsNodeComponent(IObject*) const {}
    virtual void setResponsePhysicsComponent(IObject*) const {}
};

static int fail(const char* message)
{
    std::cerr << "FAIL: " << message << std::endl;
    return 1;
}

int main()
{
    TestBundle source;
    source.setNetworkableInfo("net");
    source.updateFlags(0x1234, 0);
    source.setBundleFlag(true);
    source.addTemplate("Child");
    source.setInputId(7, 0);
    dice::hfe::Math::Vec3 position = {1.0f, 2.0f, 3.0f};
    source.setPosition(position, 0);

    MemoryStream output;
    if (!source.writeBinary(&output))
        return fail("writeBinary return");

    // ObjectTemplate prefix: string length+bytes, flags. Bundle: bool, count, then child record.
    if (output.bytes().size() != 4 + 3 + 4 + 1 + 4 + 4 + 5 + 4 + 64)
        return fail("wire size");

    TestBundle target;
    MemoryStream input(output.bytes());
    if (!target.readBinary(&input))
        return fail("readBinary return");
    if (target.getNetworkableInfo() != "net" || target.getFlags() != 0x1234)
        return fail("base prefix round trip");
    if (!target.getBundleFlag() || target.getNoTemplates() != 1 ||
        target.getTemplate(0) != "Child")
    {
        return fail("bundle record round trip");
    }
    if (target.getPosition(0).x != 1.0f || target.getPosition(0).y != 2.0f ||
        target.getPosition(0).z != 3.0f)
    {
        return fail("matrix round trip");
    }

    // Read appends instead of clearing. A duplicate S_ name reselects and overwrites the existing
    // entry through the shared active index rather than appending a second record.
    TestBundle duplicateSource;
    duplicateSource.addTemplate("S_Child");
    duplicateSource.setInputId(99, 0);
    MemoryStream duplicateBytes;
    duplicateSource.writeBinary(&duplicateBytes);

    TestBundle duplicateTarget;
    duplicateTarget.addTemplate("S_Child");
    MemoryStream duplicateInput(duplicateBytes.bytes());
    duplicateTarget.readBinary(&duplicateInput);
    if (duplicateTarget.getNoTemplates() != 1)
        return fail("S_ duplicate selection");

    std::cout << "bundle serialization: 8 checks, 0 failures" << std::endl;
    return 0;
}
