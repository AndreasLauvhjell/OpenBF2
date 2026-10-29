// Debug text writer backed by a Font instance, and the manager that flushes every writer at the
// end of each scene. Unimplemented drawing slots remain frontiers.
// [INFERRED] Source placement.
// [RETIRE-WHEN: RW-58] Confirm the original source file before removing the placement uncertainty.

// The proportional and fixed-width writers share this class; the loaded font selects the appearance.

#ifdef BF2_IMAGE_RENDDX9

#include "../../RendDX9Abi/HfeAbi.h"
#include "../../RendDX9Abi/ModuleCommon.h"
#include "../../RendDX9Abi/BfMemory.h"
#include "../../RendDX9Abi/Frontier.h"

#include <new>

namespace dice {
namespace hfe {
namespace geom {

// Class and interface IDs used by module registration and queryInterface.
extern const unsigned int IID_IDebugTextWriter        = 80043u;
extern const unsigned int CID_DebugTextWriter         = 81041u;
extern const unsigned int IID_IDebugTextWriterManager = 80044u;
extern const unsigned int CID_DebugTextWriterManager  = 81042u;

// The DebugTextWriterManager global, dword_10247C58. Declared in ModuleGeomMisc.cpp.
extern IBase* g_DebugTextWriterManager;

// FontSystem/FontSystem.cpp - the constructor builds one directly, not through the registry, and
// the typed forwards onto its virtuals.
IBase*       createFontPortedDirect();
bool         fontPortedLoad(IBase* font, const vc71::string& path);              // +0x10
void         fontPortedDrawText(IBase* font, float x, float y, const vc71::string& text); // +0x18
int          fontPortedGetHeight(IBase* font);                                   // +0x1C
int          fontPortedGetTextWidth(IBase* font, const vc71::string& text);      // +0x24
void         fontPortedSetColour(IBase* font, unsigned int colour);              // +0x30
unsigned int fontPortedGetColour(IBase* font);                                   // +0x34

namespace {

// One queued line: position in pixels, text, colour and alignment. 0x2C bytes on the engine heap.
struct QueuedText
{
    int           x;          // +0x00
    int           y;          // +0x04
    vc71::string  text;       // +0x08
    unsigned int  colour;     // +0x24
    int           alignment;  // +0x28  1 right-aligns the text on x
};

static_assert(sizeof(QueuedText) == 0x2C, "the queue entry is allocated as 0x2C bytes");

// One queued block of lines. The render and clear paths read this layout; nothing recovered fills
// the block queue yet (its writer slots +0x28/+0x2C are frontiers).
struct QueuedBlock
{
    int                 x;            // +0x00
    int                 y;            // +0x04
    vc71::vector_layout lines;        // +0x08  vector<string>, 0x1C-byte elements from +0x0C
    int                 lineSpacing;  // +0x18  added to the font height + 1
    unsigned int        colour;       // +0x1C
    int                 alignment;    // +0x20  1 right-aligns each line on x
    int                 bottomUp;     // +0x24  non-zero ends the block at y instead of starting it
};

// Appends one element pointer to a VC7.1 vector of pointers, growing by half again like the
// shipped vector insert.
void pushPointer(vc71::vector_layout& v, void* element)
{
    const unsigned int size     = v.first ? static_cast<unsigned int>(v.last - v.first) / 4u : 0u;
    const unsigned int capacity = v.first ? static_cast<unsigned int>(v.end - v.first) / 4u : 0u;
    if (!v.first || size >= capacity)
    {
        unsigned int grown = capacity + (capacity >> 1);
        if (grown < size + 1u)
            grown = size + 1u;

        char* const block = static_cast<char*>(::renddx9abi::engineAlloc(grown * 4u));
        for (unsigned int i = 0; i < size * 4u; ++i)
            block[i] = v.first[i];
        if (v.first)
            ::renddx9abi::engineFree(v.first);

        v.first = block;
        v.last  = block + size * 4u;
        v.end   = block + grown * 4u;
    }

    *reinterpret_cast<void**>(v.last) = element;
    v.last += 4;
}

// Frees a vector's storage and leaves it empty.
void releaseStorage(vc71::vector_layout& v)
{
    if (v.first)
        ::renddx9abi::engineFree(v.first);
    v.first = 0;
    v.last  = 0;
    v.end   = 0;
}

} // anonymous namespace


// dice::hfe::geom::DebugTextWriter - 0x34 bytes (52), vtable 0x1020CD10 with 19 slots.

// The slot count is read: the table runs 0x1020CD10..0x1020CD58 and the next dword is 0.

class DebugTextWriter : public IBase
{
public:
    DebugTextWriter();
    ~DebugTextWriter();                                       // not a slot; release() calls it

    virtual void  addRef();                                   // +0x00  0x1000CB20 (folded)
    virtual int   getRef();                                   // +0x04  0x101A9A10 (folded)
    virtual int   release();                                  // +0x08  0x1009BD00
    virtual void* queryInterface(unsigned int iid) const;     // +0x0C  0x1009BCD0   read

    virtual bool setFont(const renddx9abi::HostString& path); // +0x10  0x1009B160   read
    virtual bool isEnabled();                                 // +0x14  0x1009B1E0   read
    virtual void setEnabled(bool enabled);                    // +0x18  0x1009B1F0   read
    virtual unsigned int getColour();                         // +0x1C  0x1009B210   read
    virtual void setColour(unsigned int colour);              // +0x20  0x1009B220   read
    virtual void slot_0x24();
    virtual void slot_0x28();
    virtual void slot_0x2C();
    // Host-callable string slots take HostString and convert once at this boundary (RW-129).
    virtual void drawTextColoured(int x, int y, unsigned int colour,
                                  const renddx9abi::HostString& text, int alignment); // +0x30  0x1009B830
    virtual void slot_0x34();
    virtual void slot_0x38();
    virtual void slot_0x3C();
    virtual void drawText(int x, int y, const renddx9abi::HostString& text,
                          int alignment);                     // +0x40  0x1009B190   read
    virtual void render();                                    // +0x44  0x1009B5F0   read
    virtual void clear();                                     // +0x48  0x1009B960   the last slot

private:
    int                 m_refCount;    // +0x04  ctor: 1
    IBase*              m_font;        // +0x08  a geom::Font::Ported, 0xC58 bytes - see below
    vc71::vector_layout m_texts;       // +0x0C  vector<QueuedText*>
    vc71::vector_layout m_blocks;      // +0x1C  vector<QueuedBlock*>
    unsigned char       m_enabled;     // +0x2C  ctor: 1; render skips to clear while 0
    unsigned char       m_fontLoaded;  // +0x2D  ctor: 0; setFont writes 1 on success, 0 on failure
    unsigned char       m_pad2E[2];
    unsigned int        m_colour;      // +0x30  ctor: 0xFF000000 - opaque black, argb
};

static_assert(sizeof(DebugTextWriter) == 0x34,
              "RefFactory_geom_DebugTextWriter allocates 0x34 - see 0x1007CE00");


// dice::hfe::geom::DebugTextWriterManager - 0x14 bytes, vtable 0x1020CD80 with 7 slots. Holds a
// VC7.1 std::list of writers and flushes each one at the end of the scene.

class DebugTextWriterManager : public IBase
{
public:
    DebugTextWriterManager();
    ~DebugTextWriterManager();                                // not a slot; release() calls it

    virtual void  addRef();                                   // +0x00  0x1000CB20 (folded)
    virtual int   getRef();                                   // +0x04  0x101A9A10 (folded)
    virtual int   release();                                  // +0x08  0x1009BFA0
    virtual void* queryInterface(unsigned int iid) const;     // +0x0C  0x1009BF70

    virtual void addWriter(DebugTextWriter* writer);          // +0x10  0x1009BEE0   read
    virtual void removeWriter(DebugTextWriter* writer);       // +0x14  0x1009BE80   read
    virtual void renderAll();                                 // +0x18  0x1009BE40   read

private:
    // A VC7.1 list node: next, previous, value.
    struct Node
    {
        Node*            next;
        Node*            prev;
        DebugTextWriter* writer;
    };

    int           m_refCount;  // +0x04  ctor: 1
    unsigned char m_alloc[4];  // +0x08  std::allocator, padded
    Node*         m_head;      // +0x0C  sentinel node, linked to itself when empty
    unsigned int  m_size;      // +0x10
};

static_assert(sizeof(DebugTextWriterManager) == 0x14,
              "RefFactory_geom_DebugTextWriterManager allocates 0x14 - see 0x1007CE50");


// geom_DebugTextWriter_ctor, 0x1009B7C0.

DebugTextWriter::DebugTextWriter()
    : m_refCount(1)
    , m_font(0)
    , m_enabled(1)
    , m_fontLoaded(0)
    , m_colour(0xFF000000u)     // the ctor stores -16777216
{
    m_texts.alloc  = 0;
    m_texts.first  = 0;
    m_texts.last   = 0;
    m_texts.end    = 0;
    m_blocks.alloc = 0;
    m_blocks.first = 0;
    m_blocks.last  = 0;
    m_blocks.end   = 0;
    m_pad2E[0] = 0;
    m_pad2E[1] = 0;

    // The original does
    //       v2 = bf_malloc(0xC58); if (v2) v3 = geom_Font_Ported_ctor(v2); else v3 = 0;
    //       *(this + 2) = v3;
    m_font = createFontPortedDirect();

    // Registers with the manager when it already exists; the singleton list creates the manager
    // before both writers.
    if (g_DebugTextWriterManager)
        static_cast<DebugTextWriterManager*>(g_DebugTextWriterManager)->addWriter(this);
}

// Drops the queued text, releases the font and unregisters from the manager.
DebugTextWriter::~DebugTextWriter()
{
    clear();
    m_font->release();
    if (g_DebugTextWriterManager)
        static_cast<DebugTextWriterManager*>(g_DebugTextWriterManager)->removeWriter(this);
    releaseStorage(m_blocks);
    releaseStorage(m_texts);
}

void DebugTextWriter::addRef()
{
    ++m_refCount;
}

int DebugTextWriter::getRef()
{
    return m_refCount;
}

int DebugTextWriter::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
    {
        this->~DebugTextWriter();
        ::renddx9abi::engineFree(this);
    }
    return remaining;
}

// Two ids, like the other leaf classes in this image.
void* DebugTextWriter::queryInterface(unsigned int iid) const
{
    if (iid == RD9_IID_IBASE || iid == IID_IDebugTextWriter)
        return const_cast<DebugTextWriter*>(this);
    return 0;
}


// Loads the requested font and stores its ready state. Returns false when no font exists or loading fails.

bool DebugTextWriter::setFont(const renddx9abi::HostString& path)
{
    RD9_TRACE("dice::hfe::geom::DebugTextWriter::setFont", 0x10);

    if (!m_font)
    {
        // Cannot happen now that the constructor builds one; kept because the original tolerates a
        // failed allocation here and this is that arm.
        m_fontLoaded = 0;
        return false;
    }

    const bool ok = fontPortedLoad(m_font, renddx9abi::toVc71String(path));

    m_fontLoaded = ok ? 1 : 0;
    return ok;
}

bool DebugTextWriter::isEnabled()
{
    return m_enabled != 0;
}

// Disabling drops whatever is queued.
void DebugTextWriter::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!enabled)
        clear();
}

unsigned int DebugTextWriter::getColour()
{
    return m_colour;
}

// Sets the colour later drawText calls queue with.
void DebugTextWriter::setColour(unsigned int colour)
{
    m_colour = colour;
}

// Queues one line for the next render; nothing is drawn until then.
void DebugTextWriter::drawTextColoured(int x, int y, unsigned int colour,
                                       const renddx9abi::HostString& text, int alignment)
{
    void* const storage = ::renddx9abi::engineAlloc(sizeof(QueuedText));
    QueuedText* entry = 0;
    if (storage)
    {
        entry = static_cast<QueuedText*>(storage);
        entry->y = y;
        entry->x = x;
        new (&entry->text) vc71::string(renddx9abi::toVc71String(text));
        entry->colour    = colour;
        entry->alignment = alignment;
    }
    pushPointer(m_texts, entry);
}

// Queues one line in the writer's current colour.
void DebugTextWriter::drawText(int x, int y, const renddx9abi::HostString& text, int alignment)
{
    drawTextColoured(x, y, m_colour, text, alignment);
}

// Draws every queued line and block through the font, restores the font's colour and clears the
// queues. A font that failed to load is retried with the proportional font; while it still fails
// the queues are kept.
void DebugTextWriter::render()
{
    if (!m_fontLoaded)
    {
        m_fontLoaded = fontPortedLoad(m_font, vc71::string("Fonts/BF1942.font")) ? 1 : 0;
        if (!m_fontLoaded)
            return;
    }

    const unsigned int savedColour = fontPortedGetColour(m_font);

    for (char* it = m_texts.first; it != m_texts.last; it += 4)
    {
        const QueuedText* const entry = *reinterpret_cast<QueuedText**>(it);
        int offset = 0;
        if (entry->alignment == 1)
            offset = -fontPortedGetTextWidth(m_font, entry->text);
        fontPortedSetColour(m_font, entry->colour);
        const float x = static_cast<float>(entry->x + offset);
        fontPortedDrawText(m_font, static_cast<float>(static_cast<int>(x)),
                           static_cast<float>(entry->y), entry->text);
    }

    for (char* it = m_blocks.first; it != m_blocks.last; it += 4)
    {
        const QueuedBlock* const block = *reinterpret_cast<QueuedBlock**>(it);
        const int step = fontPortedGetHeight(m_font) + 1 + block->lineSpacing;

        int start = 0;
        if (block->bottomUp)
        {
            const int count = block->lines.first
                ? static_cast<int>((block->lines.last - block->lines.first) / sizeof(vc71::string))
                : 0;
            start = -(count * step);
        }

        int lineOffset = 0;
        for (char* line = block->lines.first; line != block->lines.last;
             line += sizeof(vc71::string))
        {
            const vc71::string& text = *reinterpret_cast<const vc71::string*>(line);
            float offset = 0.0f;
            if (block->alignment == 1)
                offset = -static_cast<float>(fontPortedGetTextWidth(m_font, text));
            fontPortedSetColour(m_font, block->colour);
            const int y = lineOffset + start + block->y;
            fontPortedDrawText(m_font, static_cast<float>(block->x) + offset,
                               static_cast<float>(y), text);
            lineOffset += step;
        }
    }

    fontPortedSetColour(m_font, savedColour);
    clear();
}

// Destroys every queued line and block and frees both queues' storage.
void DebugTextWriter::clear()
{
    for (char* it = m_texts.first; it != m_texts.last; it += 4)
    {
        QueuedText* const entry = *reinterpret_cast<QueuedText**>(it);
        if (entry)
        {
            entry->text.~string();
            ::renddx9abi::engineFree(entry);
        }
    }

    for (char* it = m_blocks.first; it != m_blocks.last; it += 4)
    {
        QueuedBlock* const block = *reinterpret_cast<QueuedBlock**>(it);
        if (block)
        {
            if (block->lines.first)
            {
                for (char* line = block->lines.first; line != block->lines.last;
                     line += sizeof(vc71::string))
                    reinterpret_cast<vc71::string*>(line)->~string();
                ::renddx9abi::engineFree(block->lines.first);
            }
            block->lines.first = 0;
            block->lines.last  = 0;
            block->lines.end   = 0;
            ::renddx9abi::engineFree(block);
        }
    }

    releaseStorage(m_texts);
    releaseStorage(m_blocks);
}

#define RD9_DTW_FRONTIER(method, slotOffset)                                        \
    void DebugTextWriter::method()                                                  \
    {                                                                               \
        RD9_FRONTIER("dice::hfe::geom::DebugTextWriter::" #method, slotOffset);     \
    }

RD9_DTW_FRONTIER(slot_0x24, 0x24)
RD9_DTW_FRONTIER(slot_0x28, 0x28)
RD9_DTW_FRONTIER(slot_0x2C, 0x2C)
RD9_DTW_FRONTIER(slot_0x34, 0x34)
RD9_DTW_FRONTIER(slot_0x38, 0x38)
RD9_DTW_FRONTIER(slot_0x3C, 0x3C)

#undef RD9_DTW_FRONTIER

// RefFactory_geom_DebugTextWriter, 0x1007CE00. Same shape and same __fastcall convention as the
// other three factories in this image.
IBase* __fastcall createDebugTextWriter(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;

    DebugTextWriter* self = new DebugTextWriter();

    void* itf = self->queryInterface(iid);
    if (!itf)
        self->release();
    return static_cast<IBase*>(itf);
}


// geom_DebugTextWriterManager_ctor, 0x1009BEA0.

DebugTextWriterManager::DebugTextWriterManager()
    : m_refCount(1)
    , m_size(0)
{
    m_alloc[0] = m_alloc[1] = m_alloc[2] = m_alloc[3] = 0;
    m_head = static_cast<Node*>(::renddx9abi::engineAlloc(sizeof(Node)));
    m_head->next = m_head;
    m_head->prev = m_head;
}

// Frees the list nodes; the writers themselves are not owned.
DebugTextWriterManager::~DebugTextWriterManager()
{
    Node* node = m_head->next;
    while (node != m_head)
    {
        Node* const next = node->next;
        ::renddx9abi::engineFree(node);
        node = next;
    }
    ::renddx9abi::engineFree(m_head);
    m_head = 0;
    m_size = 0;
}

void DebugTextWriterManager::addRef()
{
    ++m_refCount;
}

int DebugTextWriterManager::getRef()
{
    return m_refCount;
}

int DebugTextWriterManager::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
    {
        this->~DebugTextWriterManager();
        ::renddx9abi::engineFree(this);
    }
    return remaining;
}

void* DebugTextWriterManager::queryInterface(unsigned int iid) const
{
    if (iid == RD9_IID_IBASE || iid == IID_IDebugTextWriterManager)
        return const_cast<DebugTextWriterManager*>(this);
    return 0;
}

// Appends a writer; null is ignored.
void DebugTextWriterManager::addWriter(DebugTextWriter* writer)
{
    if (!writer)
        return;

    Node* const node = static_cast<Node*>(::renddx9abi::engineAlloc(sizeof(Node)));
    node->next   = m_head;
    node->prev   = m_head->prev;
    node->writer = writer;
    ++m_size;
    m_head->prev = node;
    node->prev->next = node;
}

// Removes every entry for a writer; null is ignored.
void DebugTextWriterManager::removeWriter(DebugTextWriter* writer)
{
    if (!writer)
        return;

    Node* node = m_head->next;
    while (node != m_head)
    {
        Node* const next = node->next;
        if (node->writer == writer)
        {
            node->prev->next = next;
            next->prev = node->prev;
            ::renddx9abi::engineFree(node);
            --m_size;
        }
        node = next;
    }
}

// Draws each enabled writer's queue; a disabled writer's queue is dropped instead.
void DebugTextWriterManager::renderAll()
{
    for (Node* node = m_head->next; node != m_head; node = node->next)
    {
        if (node->writer->isEnabled())
            node->writer->render();
        else
            node->writer->clear();
    }
}

// RefFactory_geom_DebugTextWriterManager, 0x1007CE50.
IBase* __fastcall createDebugTextWriterManager(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;

    DebugTextWriterManager* self = new DebugTextWriterManager();

    void* itf = self->queryInterface(iid);
    if (!itf)
        self->release();
    return static_cast<IBase*>(itf);
}

// The renderer's end-of-scene flush: rendererEndScene calls this on the published manager.
void debugTextWriterManagerRenderAll(IBase* manager)
{
    static_cast<DebugTextWriterManager*>(manager)->renderAll();
}

// Typed forwards for the renderer's frame-rate overlay.
void debugTextWriterSetColour(IBase* writer, unsigned int colour)
{
    static_cast<DebugTextWriter*>(writer)->setColour(colour);
}

void debugTextWriterDrawText(IBase* writer, int x, int y, const vc71::string& text, int alignment)
{
    static_cast<DebugTextWriter*>(writer)->drawText(x, y, renddx9abi::toHostString(text), alignment);
}

} // namespace geom
} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_RENDDX9
