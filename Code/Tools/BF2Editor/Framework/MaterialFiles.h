// Edits material definitions and pair-response settings in place without the engine.
// Preserves untouched bytes; new blocks follow ascending material/pair IDs and engine property ordering.
// Optional properties follow writer defaults, with numbers formatted to six significant digits.
#ifndef BF2EDITOR_FRAMEWORK_MATERIALFILES_H
#define BF2EDITOR_FRAMEWORK_MATERIALFILES_H

#include "Framework/EditorConfig.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace bf2editor
{

// A float as the engine's writers print one (default ostream: %g, precision 6).
std::string materialNumber(float value);

// A text file kept as lines with their own line ends.
class MaterialTextLines
{
public:
    MaterialTextLines();
    void parse(const std::string& text);
    std::string render() const;
    bool hasChanges() const { return render() != m_original; }
    const std::string& original() const { return m_original; }

protected:
    struct Line
    {
        std::string text;
        std::string end;        // "\r\n", "\n", or "" for a last line without one
    };
    // The words of a line, blank-separated (no quote handling: callers read the rest themselves).
    static std::vector<std::string> wordsOf(const std::string& text);
    // The rest of a line after its first `count` words, trimmed.
    static std::string restAfter(const std::string& text, size_t count);
    std::string newline() const;    // the file's own style, CRLF when it has none yet
    void insert(size_t at, const std::string& text);
    void erase(size_t at);

    std::vector<Line> m_lines;
    std::string       m_original;
    bool              m_dirty;      // lines changed since the blocks were last indexed
};

class MaterialDefineFile : public MaterialTextLines
{
public:
    // The material properties, in the writer's line order.
    static const std::vector<std::string>& properties();
    static bool isProperty(const std::string& property);

    std::vector<int> ids() const;
    bool has(int id) const;
    // A property's text as the file has it (a name without its quotes); the default when the writer left
    // the line out (false for a material that is not there or a property that does not exist).
    bool value(int id, const std::string& property, std::string& out) const;
    // Sets a property in place, adding the line in the writer's order or taking it out when the value is the
    // writer's default for an optional line (and minDamageLoss / maxDamageLoss with damageLoss off).
    bool setValue(int id, const std::string& property, const std::string& value, std::string& error);
    // A new material block in id order: name, type and the writer's defaults for friction, elasticity and
    // resistance.
    bool add(int id, const std::string& name, int type, std::string& error);
    bool remove(int id, std::string& error);
    // Lines that break the writer's rules (block or line order, a default written, an unknown property):
    // one sentence each; empty for a file the engine could have written.
    std::vector<std::string> violations() const;

private:
    struct Block
    {
        int    id;
        size_t first;           // the Material.active line
        size_t end;             // one past its last line (the blank line included)
    };
    void index();
    void ensureIndexed() const { if (m_dirty) const_cast<MaterialDefineFile*>(this)->index(); }
    int blockOf(int id) const;
    // The line of `property` inside block b, npos when none.
    size_t lineOf(size_t b, const std::string& property) const;
    std::vector<Block>  m_blocks;
    std::map<int, size_t> m_byId;   // id -> block (the first, should a file repeat one)
};

class MaterialCellFile : public MaterialTextLines
{
public:
    // The three template kinds.
    enum Kind { Effect = 0, Decal = 1, Sound = 2 };
    static const char* kindName(Kind kind);
    static bool kindFromName(const std::string& name, Kind& kind);

    std::vector<std::pair<int, int> > cells() const;
    bool has(int a, int b) const;
    // damageMod: false when the cell is not there; `out` empty when the cell sets none (unset, < 0).
    bool damageMod(int a, int b, std::string& out) const;
    // A template by kind and index: false when the cell is not there; `out` empty when it sets none.
    bool templateAt(int a, int b, Kind kind, int index, std::string& out) const;
    // The indexes a cell sets for a kind, ascending as written.
    std::vector<int> indexes(int a, int b, Kind kind) const;

    // A new cell block in (a, b) order; false when it is there already.
    bool add(int a, int b, std::string& error);
    bool remove(int a, int b, std::string& error);
    // Every cell whose attacker or defender is `id` taken out (a material removed); how many.
    int removeCellsOf(int id);
    // damageMod in place; a negative value (unset) takes the line out.
    bool setDamageMod(int a, int b, const std::string& value, std::string& error);
    // A template in place; an empty name takes the line out.
    bool setTemplate(int a, int b, Kind kind, int index, const std::string& name, std::string& error);
    std::vector<std::string> violations() const;

private:
    struct Block
    {
        int    a, b;
        size_t first;           // the createCell line
        size_t end;
    };
    void index();
    void ensureIndexed() const { if (m_dirty) const_cast<MaterialCellFile*>(this)->index(); }
    int blockOf(int a, int b) const;
    // A cell line's sort key inside its block: damageMod 0, then (kind + 1) * 100000 + index; -1 when the
    // line is not a cell line.
    long lineKey(size_t line) const;
    std::vector<Block> m_blocks;
    std::map<std::pair<int, int>, size_t> m_byPair;
};

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_MATERIALFILES_H
