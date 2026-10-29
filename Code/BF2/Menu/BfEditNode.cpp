// [INFERRED] path/names. Bounded real BfEditNode storage/properties/update;
// no complete Node descriptor or edit-node factory is published.
#include "BfEditNode.h"
#if !DEDICATED
#include "BfEditData.h"
#include "DifFontMetadata.h"
#include "Font.h"
#include "../External/BFMeme/Action.h"
#include "../External/BFMeme/Effect.h"
#include "../External/BFMeme/Pointer.h"
#include "../External/BFMeme/Stream.h"
#include "../External/BFMeme/Style.h"
#include "../External/BFMeme/System.h"
#include "../External/BFMeme/TypeDescriptor.h"
#include <cmath>
#include <cstring>
#include <new>
#include <string>
namespace dice { namespace meme {
BfEditNode::BfEditNode()
    : m_editData(nullptr), m_string(nullptr), m_selectAction(nullptr), m_font(0),
      m_maxCharacters(-1), m_firstVisible(0), m_cursor(0), m_blinkTime(-0.4f),
      m_focused(false), m_pendingInput(0), m_ignoreClick(false), m_flag39(false),
      m_slowBlink(false), m_skipCharacter(false), m_limitWidth(false), m_outline(false),
      m_color{0.0f, 0.0f, 1.0f, 1.0f} {}
unsigned int BfEditNode::getObjectSize() { return sizeof(BfEditNode); }
bool BfEditNode::streamProperties(System* system, Stream* stream)
{
    if (!Node::streamProperties(system, stream)) return false;
    if (!stream->streamFont(system, &m_font, "Font")) return false;
    if (!stream->streamData(system, m_string, "String", nullptr)) return false;
    if (!stream->streamInt(&m_maxCharacters, "Max characters(-1 = no limit)")) return false;
    if (!stream->streamAction(system, m_selectAction, "Select action", nullptr)) return false;
    // Stream the pointer itself, retaining modifications made before a failure.
    if (!stream->streamData(system, reinterpret_cast<Data*&>(m_editData),
                            "Editbox data", BfEditData::getClassType())) return false;
    if (m_editData) {
        m_editData->m_activeNode = this;
        m_editData->registerNode(this); // reload the live member; duplicates retained
    }
    return stream->streamBool(&m_focused, "Focus") != false;
}
bool BfEditNode::updateSelf(System* system, const NodeState* state, const Rect* rect,
                          float delta)
{
    if (!Node::updateSelf(system, state, rect, delta)) return false;
    // Original stores the float but compares the still-live x87 sum. Double
    // holds the exact sum of these two float operands for this comparison.
    const double time = static_cast<double>(delta) + m_blinkTime;
    m_blinkTime = static_cast<float>(time);
    if (!m_slowBlink) {
        if (time >= static_cast<double>(0.4f)) m_blinkTime = -0.4f;
    } else if (time >= static_cast<double>(0.8f)) m_blinkTime = -0.8f;
    if (m_pendingInput == 1) {
        onEvent1(system, rect, state, m_pendingKey, 1, 1);
        m_pendingInput = 0; // callback may mutate it; the shipped clear follows
        return true;
    }
    if (m_pendingInput == 2) {
        onEvent4(system, rect, state, m_pendingCharacter, 1, 1);
        m_pendingInput = 0;
    }
    return true;
}
void BfEditNode::updateVisibleStart(System* system, const Rect* rect)
{
    if (!m_string) return;
    if (m_cursor <= m_firstVisible) {
        m_firstVisible = m_cursor;
        return;
    }
    float remaining = rect->width;
    for (int index = m_cursor - 1; index >= m_firstVisible; --index) {
        GlyphMetrics metrics;
        const int font = m_font;
        const int character = m_string->getCharacter(system, index);
        system->getGlyphMetrics(character, font, &metrics); // result deliberately discarded
        // Right + width + left, preserving the actual evaluation order.
        const double width = (static_cast<double>(metrics.rightBearing) + metrics.width) + metrics.leftBearing;
        const double next = static_cast<double>(remaining) - width;
        remaining = static_cast<float>(next);
        if (next <= 0.0) {
            m_firstVisible = index + 1;
            return;
        }
    }
}
bool BfEditNode::onEvent4(System* system, const Rect* rect, const NodeState*,
                         int character, int, int) // 7AB9B0; final two integers unused
{
    if (!m_focused || !m_string || character == 13 || character == 27 ||
        character > 256 || character == 22) return false;
    if (m_slowBlink && m_skipCharacter) {
        m_skipCharacter = false;
        return false;
    }
    m_skipCharacter = !m_skipCharacter;
    const int length = m_string->getStringLength(system);
    if (m_maxCharacters > 0 && length + 1 > m_maxCharacters) return false;
    const int capacity = length + 2;
    char* buffer = static_cast<char*>(::operator new(capacity)); // 447EF0; no null guard
    m_string->getString(system, buffer, capacity);
    std::memmove(buffer + m_cursor + 1, buffer + m_cursor, length - m_cursor + 1);
    buffer[m_cursor] = static_cast<char>(character); // negative and 256 accepted; low byte retained
    if (m_limitWidth) {
        float remaining = rect->width;
        for (int index = 0; index < length + 1; ++index) {
            GlyphMetrics metrics;
            system->getGlyphMetrics(static_cast<signed char>(buffer[index]), m_font, &metrics);
            const double width = (static_cast<double>(metrics.rightBearing) + metrics.width) + metrics.leftBearing;
            const double next = static_cast<double>(remaining) - width;
            remaining = static_cast<float>(next);
            if (next <= 0.0) return false; // Shipped buffer leak at7ABAB1 ->7ABAFE; preserve it.
        }
    }
    m_string->setString(system, buffer); // callback result is ignored; member reloaded below
    ::operator delete(buffer);
    buffer = nullptr;
    if (length != m_string->getStringLength(system)) {
        ++m_cursor;
        updateVisibleStart(system, rect);
    }
    return true;
}
TypeDescriptor* BfEditNode::getClassType()                 // 7AC340, singletonA20EFC
{
    static ClassTypeDescriptor<BfEditNode, Node> type("dice::meme::BfEditNode");
    return &type;
}
TypeDescriptor* BfEditNode::getType() { return getClassType(); }

// Draws the visible text from the first visible character up to a carriage return or the
// right edge, then the two-unit cursor while it is focused and in the visible blink phase.
// Glyph quads are clipped to `bounds`; the shipped body never reads `clip`. A failed glyph
// draw ends the paint without a cursor. Hidden edit data suppresses glyphs, not the cursor.
void BfEditNode::paintSelf(System* system, const NodeState*, const Rect* bounds, const Rect*,
                           RenderContext* context)
{
    if (!m_string) return;
    float x = bounds->x;
    const float baseY = bounds->y;
    const float right = bounds->x + bounds->width;
    const float fontHeight = system->getFontHeight(m_font);
    Rect glyph = *bounds;
    int index = m_firstVisible;
    int elementIndex = 0;
    bool cursorDrawn = false;
    hfe::menu::Font* const font = reinterpret_cast<hfe::menu::Font*>(m_font);
    const std::string fontName(font->getName()); // retail has no null guard
    float fontScale = 1.0f;
    if (fontName.find(".dif", 0, 4) != std::string::npos)
        fontScale = static_cast<hfe::menu::DifFont*>(font)->getMetadata()->getScale();
    const float reciprocalScale = static_cast<float>(1.0 / fontScale);

    const auto drawCursor = [&]() {
        glyph.height = fontHeight;
        glyph.y = baseY;
        glyph.width = 2.0f;
        Quad cursor(&glyph, nullptr);
        drawContextTexture(context, system, 0, &cursor);
    };
    const auto cursorVisible = [&]() {
        return index == m_cursor && m_blinkTime >= 0.0f && m_focused;
    };

    for (int character = m_string->getCharacter(system, index); character;) {
        if (character == 13) {
            if (!cursorDrawn) index = m_cursor; // forces the trailing cursor below
            break;
        }
        GlyphMetrics metrics; // retail ignores failure
        system->getGlyphMetrics(character, m_font, &metrics);
        glyph.width = metrics.width;
        glyph.height = metrics.height;
        if (fontScale == 1.0f) {
            glyph.x = x;
            glyph.y = metrics.verticalOffset + baseY;
        } else {
            x = fontScale * x;
            const float scaledY = fontScale * baseY;
            const double roundedX = std::floor(!(std::fmod(x, 1.0) >= 0.5)
                                                   ? static_cast<double>(x)
                                                   : static_cast<double>(x) + 1.0);
            x = static_cast<float>(roundedX * reciprocalScale);
            glyph.x = x;
            const double roundedY = std::floor(!(std::fmod(scaledY, 1.0) >= 0.5)
                                                   ? static_cast<double>(scaledY)
                                                   : static_cast<double>(scaledY) + 1.0);
            glyph.y = static_cast<float>((roundedY + metrics.verticalOffset) * reciprocalScale);
        }
        if (!(x < right)) break;

        if (!m_editData || !m_editData->m_hideText) {
            if (m_outline) {
                const Color outlineColor = {0.15000001f, 0.15000001f, 0.15000001f, 1.0f};
                const float glyphX = glyph.x, glyphY = glyph.y;
                Quad outline(&glyph, bounds, &outlineColor);
                translateQuad(&outline, 1.0f, 1.0f, -0.001f);
                drawContextCharacter(context, system, character, m_font, elementIndex, &outline);
                setQuadBounds(&outline, glyphX + 1.0f, glyphY, glyph.width, glyph.height);
                drawContextCharacter(context, system, character, m_font, elementIndex, &outline);
                setQuadBounds(&outline, glyphX + 1.0f, glyphY - 1.0f, glyph.width, glyph.height);
                drawContextCharacter(context, system, character, m_font, elementIndex, &outline);
                setQuadBounds(&outline, glyphX, glyphY - 1.0f, glyph.width, glyph.height);
                drawContextCharacter(context, system, character, m_font, elementIndex, &outline);
                setQuadBounds(&outline, glyphX - 1.0f, glyphY - 1.0f, glyph.width, glyph.height);
                drawContextCharacter(context, system, character, m_font, elementIndex, &outline);
                setQuadBounds(&outline, glyphX - 1.0f, glyphY, glyph.width, glyph.height);
                drawContextCharacter(context, system, character, m_font, elementIndex, &outline);
                setQuadBounds(&outline, glyphX - 1.0f, glyphY + 1.0f, glyph.width, glyph.height);
                drawContextCharacter(context, system, character, m_font, elementIndex, &outline);
                setQuadBounds(&outline, glyphX, glyphY + 1.0f, glyph.width, glyph.height);
                drawContextCharacter(context, system, character, m_font, elementIndex, &outline);
            }
            Quad quad(&glyph, bounds, &m_color);
            if (!drawContextCharacter(context, system, character, m_font, elementIndex, &quad))
                return;
        }
        if (cursorVisible()) {
            drawCursor();
            cursorDrawn = true;
        }
        ++elementIndex;
        ++index;
        x = static_cast<float>(((static_cast<double>(metrics.rightBearing) + metrics.width)
                                + metrics.leftBearing) + x);
        character = m_string->getCharacter(system, index);
    }
    if (cursorVisible()) {
        glyph.x = x;
        drawCursor();
    }
}

// Handles pointer and editing keys for the focused box. Only index 1 is accepted. Key 18 runs
// the select action and drops focus; a click inside the bounds takes focus and locks the
// pointer, one outside drops focus, unlocks and forwards the click to the system pointer.
// Editing keys need focus and text. Returns false when the event is refused or focus is lost.
bool BfEditNode::onEvent1(System* system, const Rect* rect, const NodeState* state,
                          int key, int index, int second)
{
    if (index != 1) return false;
    const auto loseFocus = [&]() {
        m_focused = false;
        system->unlockPointer(); // no zero guard; an unfocused miss also unlocks
        pointerEvent1(system, static_cast<Pointer*>(system->getPointerObject()), 1, 1, second);
    };
    if (key == 18) {
        if (m_selectAction) {
            const NodeInputEvent event = {1, 0, 0.0f, 0.0f, 0};
            m_selectAction->execute(system, rect, state, &event);
            if (!m_focused) return false;
            m_focused = false;
            system->unlockPointer();
        }
    } else if (key == 1 && !m_ignoreClick) {
        const float* const pointer = system->getPointerPosition();
        const float pointerX = pointer[0];
        const float pointerY = pointer[1];
        const float right = rect->width + rect->x;
        const float bottom = rect->height + rect->y;
        const bool inside = pointerX > rect->x && pointerX < right &&
                            pointerY > rect->y && pointerY < bottom;
        if (!inside) {
            loseFocus();
            return false;
        }
        if (!m_focused) {
            m_focused = true;
            system->lockPointer();
        }
    }
    if (!m_focused || !m_string) return false;

    const int length = m_string->getStringLength(system);
    switch (key) {
    case 1:
        if (m_ignoreClick) {
            m_ignoreClick = false;
            break;
        }
        {
            float offset = rect->x - system->getPointerPosition()[0];
            if (offset < 0.0f) offset *= -1.0f;
            float remaining = rect->width;
            int position = 0;
            for (; position < length; ++position) {
                GlyphMetrics metrics;
                const int font = m_font;
                const int character = m_string->getCharacter(system, position);
                system->getGlyphMetrics(character, font, &metrics); // result discarded
                const double next = static_cast<double>(remaining) -
                    ((static_cast<double>(metrics.rightBearing) + metrics.width) + metrics.leftBearing);
                remaining = static_cast<float>(next);
                if (next <= static_cast<double>(rect->width) - offset) {
                    m_cursor = position;
                    updateVisibleStart(system, rect);
                    break;
                }
            }
            if (position == length) {
                m_cursor = position;
                updateVisibleStart(system, rect);
            }
        }
        break;
    case 5:
        if (m_cursor > 0) --m_cursor;
        break;
    case 7:
        if (m_cursor < length) ++m_cursor;
        break;
    case 9:
        m_cursor = 0;
        updateVisibleStart(system, rect);
        break;
    case 10:
        m_cursor = length - 1; // shipped end key stops before the last character
        updateVisibleStart(system, rect);
        break;
    case 14:
        if (m_cursor < length) {
            char* buffer = static_cast<char*>(::operator new(length + 1)); // no null guard
            m_string->getString(system, buffer, length + 1);
            std::memmove(buffer + m_cursor, buffer + m_cursor + 1, length - m_cursor);
            m_string->setString(system, buffer);
            ::operator delete(buffer);
            buffer = nullptr;
        }
        break;
    case 15:
        if (m_cursor > 0) {
            char* buffer = static_cast<char*>(::operator new(length + 1)); // no null guard
            m_string->getString(system, buffer, length + 1);
            std::memmove(buffer + m_cursor - 1, buffer + m_cursor, length - m_cursor + 1);
            m_string->setString(system, buffer);
            ::operator delete(buffer);
            buffer = nullptr;
            --m_cursor;
        }
        break;
    case 16:
        return true; // no visible-start refresh
    case 17:
        if (m_ignoreClick) m_ignoreClick = false;
        break;
    default:
        break;
    }
    updateVisibleStart(system, rect);
    return true;
}
static_assert(sizeof(BfEditNode) == 80, "native BfEditNode storage");
} }
#endif
