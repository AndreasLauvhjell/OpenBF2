// Analyzes selected object trees with wizard rules and executes selected resolutions as undoable edits.
// CheckValue probes engine state; ObjectTemplate resolutions edit documents, other commands use the console.
// Execution order is Errors twice, Warnings, then Utils. Prompts pause execution; cancellation ends the item.
// Duplicate group labels select the last entry. Rules load once per session, and Include retains matching nodes;
// these intentional editor choices avoid duplicate loading and the retail invalid-erase behavior.
#ifndef BF2EDITOR_APP_OBJECTWIZARD_H
#define BF2EDITOR_APP_OBJECTWIZARD_H

#include "Framework/EditorConfig.h"

#include "Framework/WizardRules.h"

#include <string>
#include <vector>

namespace dice { namespace hfe { namespace world { class IObject; } } }

namespace bf2editor
{

class EditSession;

class ObjectWizard
{
public:
    explicit ObjectWizard(EditSession& session);

    // The rules from the retail editor's install beside the game (bf2editor/Content/ObjectEditor/Wizards),
    // fresh each call.
    bool load(std::string& result);
    bool loaded() const { return m_loaded; }
    const WizardRules& rules() const { return m_rules; }

    // An analysis of the selected object's tree for a category.
    bool run(const std::string& category, std::string& result);
    const std::string& rootName() const { return m_rootName; }

    struct Item
    {
        int                          rule;
        dice::hfe::world::IObject*   object;
        std::string                  label;
        int                          group;      // 0 Errors, 1 Warnings, 2 Utils, -1 another type
        bool                         ticked;
        bool                         readOnly;
        std::string                  status;
    };
    const std::vector<Item>& items() const { return m_items; }
    bool tick(size_t index, bool on, std::string& result);

    // Execute, and what it waits for.
    enum class Wait { None, Text, Choice, Float, Checklist };
    bool execute(std::string& result);
    Wait waiting() const { return m_wait; }
    const std::string& promptTitle() const { return m_promptTitle; }
    const std::string& promptText() const { return m_promptText; }
    const std::string& promptPreset() const { return m_promptPreset; }
    const std::vector<std::string>& promptChoices() const { return m_choices; }
    std::vector<bool>& checklist() { return m_checklist; }
    bool answer(const std::string& text, std::string& result);
    bool cancel(std::string& result);
    bool checklistDone(std::string& result);
    // A short account of the run so far (Execute's own log).
    const std::vector<std::string>& report() const { return m_report; }

    // The level closes: every object pointer goes.
    void forget();

private:
    ObjectWizard(const ObjectWizard&);
    ObjectWizard& operator=(const ObjectWizard&);

    typedef dice::hfe::world::IObject Object;

    // A line the wizard issues, through the capture test; a Wizard command handled, a console line run in
    // the engine (an analysis line) or by the translator (a resolution line). Returns the console's answer.
    std::string issue(const std::string& line, bool resolution);
    std::string engine(const std::string& line);
    void command(const std::string& name, const std::vector<std::string>& arguments, bool resolution);

    // Analysis (§4).
    void analyze(int rule, Object* root);
    bool filter(int rule, int op, const std::string& className, Object* node);
    void collect(int rule, Object* node, std::vector<Object*>& out, bool test);
    std::string templateName(Object* node) const;

    // Execute's state machine.
    void step();
    bool startItem(size_t item);
    void finishItem(bool ran);
    void translate(const std::string& line);
    void flushPending();
    void openBatch();
    void closeBatch();
    bool objectsAlive() const;
    // False, with Execute stopped and everything forgotten, when the object looked at is gone (deleted, or its
    // creation undone) while Execute waited: nothing may touch its tree any more.
    bool stillThere(std::string& result);

    EditSession&                       m_session;
    WizardRules                        m_rules;
    bool                               m_loaded;
    std::vector<std::vector<Object*> > m_results;     // per rule (+0x10C)
    std::vector<Object*>               m_target;      // per rule (+0x118)
    std::vector<Item>                  m_items;
    Object*                            m_root;
    unsigned                           m_rootId;
    std::string                        m_rootName;
    std::string                        m_category;

    // Execute.
    bool                               m_executing;
    int                                m_pass;          // 0..3: Errors, Errors again, Warnings, Utils
    size_t                             m_next;          // the next item of the pass
    int                                m_item;          // the item running, -1 between items
    std::vector<Object*>               m_objects;       // the objects its resolution runs on
    size_t                             m_objectAt;
    size_t                             m_lineAt;
    bool                               m_listAsked;
    Wait                               m_wait;
    std::string                        m_promptTitle;
    std::string                        m_promptText;
    std::string                        m_promptPreset;
    std::vector<std::string>           m_choices;
    int                                m_floatIndex;
    std::vector<bool>                  m_checklist;
    std::vector<std::string>           m_report;
    // The translator's context for the item running.
    std::string                        m_active;        // the template ObjectTemplate lines act on
    std::string                        m_pendingClass;  // a create waiting for its addTemplate
    std::string                        m_pendingName;
    std::vector<std::pair<std::string, std::string> > m_pendingLines;
    std::string                        m_childParent;   // the child the last addTemplate added
    std::string                        m_childName;
    bool                               m_batchOpen;
    int                                m_edits;
};

} // namespace bf2editor

#endif // BF2EDITOR_APP_OBJECTWIZARD_H
