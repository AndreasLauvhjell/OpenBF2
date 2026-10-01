// Edits .con/.tweak template properties in place without regenerating unrelated content.
// create/activeSafe/active select a template block; property names compare case-insensitively.
// Indexed properties identify repeated commands. New properties stay in their component section;
// untouched lines keep exact bytes/endings and additions use the most common newline style.
#ifndef BF2EDITOR_FRAMEWORK_TEMPLATEFILE_H
#define BF2EDITOR_FRAMEWORK_TEMPLATEFILE_H

#include "Framework/EditorConfig.h"

#include <cstddef>
#include <string>
#include <vector>

namespace bf2editor
{

struct TemplateBlock
{
    std::string className;      // "" for a block opened by ObjectTemplate.active
    std::string name;
    std::string opening;        // create, activeSafe or active, as spelt in the file
    size_t      first;          // the opening line
    size_t      end;            // one past the block's last line
};

struct TemplatePropertyLine
{
    size_t      line;
    std::string property;       // as spelt in the file, e.g. "armor.maxHitPoints"
    std::string value;          // the rest of the line after the separating blanks
};

// A child of a template's block: its addTemplate line and the setPosition and setRotation lines right after
// it (the engine applies them to the child just added). The stock files have no other shape: 8,811 children
// in the 4,624 stock files, each child's position before its rotation when it has both.
struct TemplateChildLines
{
    size_t      line;           // the addTemplate line
    size_t      end;            // one past the child's last line
    std::string child;          // the template it adds (the line's first word)
    int         positionLine;   // -1 for none
    std::string position;
    int         rotationLine;   // -1 for none
    std::string rotation;
};

// A component section of a template's block: `rem ---BeginComp:<C> ---`, the component's lines (the first
// is `ObjectTemplate.createComponent <C>`), `rem ---EndComp ---` - makeScript's shape. 2,642 of the 2,720
// stock createComponent lines open such a section; the rest are hand-written lines outside one.
struct TemplateComponentSection
{
    std::string component;      // as the BeginComp line spells it
    size_t      begin;          // the BeginComp line
    size_t      end;            // one past the EndComp line; 0 when the section has none (2 stock files)
};

class TemplateFile
{
public:
    TemplateFile();

    void parse(const std::string& text);
    std::string render() const;
    bool hasChanges() const;

    size_t lineCount() const { return m_lines.size(); }
    const std::string& line(size_t index) const { return m_lines[index].text; }
    const std::vector<TemplateBlock>& blocks() const { return m_blocks; }
    // The blocks of a template in this file (a file may open one template more than once), in file order.
    std::vector<size_t> blocksOf(const std::string& name) const;
    // The property lines of a template's last block in this file (the block an edit goes to).
    std::vector<TemplatePropertyLine> properties(const std::string& name) const;

    // A property of the template's last block set to `value`: `index` picks among lines of the same
    // property (-1: the property must appear at most once). False with `error` when the file has no block
    // of the template, or the property appears more than once and no index names one.
    bool setProperty(const std::string& name, const std::string& property, const std::string& value, int which,
                     std::string& error);
    // The property line taken out (index as for setProperty).
    bool removeProperty(const std::string& name, const std::string& property, int which, std::string& error);
    // One more line of a property that adds rather than sets (armor.addArmorEffect, fire.addBarrelName,
    // fire.addFireRate): after the block's last line of it, or where setProperty would put a new one.
    bool addProperty(const std::string& name, const std::string& property, const std::string& value, std::string& error);

    // The geometry template's lines: `GeometryTemplate.<property> <value>` lines, which a .tweak
    // keeps before its ObjectTemplate blocks and the game applies to the geometry its .con made. In file order.
    std::vector<TemplatePropertyLine> geometryLines() const;
    // Set in place (`which` as for setProperty), or added after the last geometry line - or, with none, after the
    // file's leading rem lines.
    bool setGeometryProperty(const std::string& property, const std::string& value, int which, std::string& error);
    bool removeGeometryProperty(const std::string& property, int which, std::string& error);
    // A child added to the template's last block: `ObjectTemplate.addTemplate <child>`, then
    // `ObjectTemplate.setPosition <position>` when one is given (it applies to the child just added), after
    // the block's last child line (addTemplate, setPosition, setRotation) - or, in a block without children,
    // where a new property goes.
    bool addChildLines(const std::string& name, const std::string& child, const std::string& position,
                       std::string& error);
    // The block's first `addTemplate <child>` line taken out, with the setPosition and setRotation lines right
    // after it (they belong to that child). False when the block has none.
    bool removeChildLines(const std::string& name, const std::string& child, std::string& error);
    // The children the template's last block adds, in file order.
    std::vector<TemplateChildLines> children(const std::string& name) const;
    // That child (an index into children()) taken out with its position and rotation lines.
    bool removeChildAt(const std::string& name, size_t which, std::string& error);
    // That child's position and rotation (x/y/z words): a line there has its value rewritten in place, its
    // spelling and blanks kept; a missing one is added - setPosition right after the addTemplate line,
    // setRotation after the position - unless its value is empty. An empty value leaves a line alone.
    bool setChildTransformAt(const std::string& name, size_t which, const std::string& position,
                             const std::string& rotation, std::string& error);
    // The component sections of the template's last block, in file order.
    std::vector<TemplateComponentSection> componentSections(const std::string& name) const;
    // A section for a new component - its BeginComp line, `ObjectTemplate.createComponent <C>`, its EndComp
    // line - after the block's last section, or, in a block without one, after the block's leading header
    // lines (the flag and networkable lines makeScript writes before the components; right after the
    // opening line when there are none).
    bool addComponentSection(const std::string& name, const std::string& component, std::string& error);
    // The component's section taken out; `lines` receives its property lines (createComponent excepted) as
    // `ObjectTemplate.<property> <value>`, in order. False when the block has no whole section of it.
    bool removeComponentSection(const std::string& name, const std::string& component,
                                std::vector<std::string>& lines, std::string& error);
    // The same, with a section for the new component put where the old one was.
    bool replaceComponentSection(const std::string& name, const std::string& oldComponent,
                                 const std::string& newComponent, std::vector<std::string>& lines, std::string& error);
    // A new block `ObjectTemplate.activeSafe <class> <name>` at the end of the file (after a blank line),
    // for a template whose lines this file does not hold yet.
    void appendBlock(const std::string& className, const std::string& name);
    // A new block `ObjectTemplate.create <class> <name>` and its property lines (each without the
    // ObjectTemplate. prefix) at the end of the file, after a blank line: a new template the file defines.
    void appendCreateBlock(const std::string& className, const std::string& name, const std::vector<std::string>& properties);
    // A template renamed through the file, in place: the name in its block openings (create, activeSafe,
    // active) and in `addTemplate` lines. Other property values that name it are left alone. False when no
    // line names it.
    bool renameTemplateLines(const std::string& from, const std::string& to);
    // The class named in the template's block openings (`create <class> <name>`, `activeSafe <class> <name>`)
    // set, in place. False when no opening of the template names a class.
    bool setBlockClass(const std::string& name, const std::string& className);

    // The value text of `property` in the template's last block, the first one when there are several.
    bool value(const std::string& name, const std::string& property, std::string& out) const;

    // "ObjectTemplate.<property> <value>" split; false for any other line.
    static bool splitPropertyLine(const std::string& text, std::string& property, std::string& value);

private:
    struct Line
    {
        std::string text;       // without its line end
        std::string end;        // "\r\n", "\n" or "" (the last line without one)
    };
    void index();
    int lastBlockOf(const std::string& name) const;
    std::string newline() const;
    // Where a new property line of the block goes (the line it is inserted before).
    size_t insertionPoint(const TemplateBlock& block, const std::string& property) const;
    // A line inserted before `at` with the file's line end (a file without a final line end keeps its lack
    // of one).
    void insertLine(size_t at, const std::string& text);
    // A component's section taken out; `at` receives the line it began on.
    bool takeComponentSection(const std::string& name, const std::string& component, std::vector<std::string>& lines,
                              size_t& at, std::string& error);
    void insertComponentLines(size_t at, const std::string& indentation, const std::string& component);

    std::vector<Line>          m_lines;
    std::vector<TemplateBlock> m_blocks;
    std::string                m_original;      // the text parsed, for hasChanges
    bool                       m_changed;
};

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_TEMPLATEFILE_H
