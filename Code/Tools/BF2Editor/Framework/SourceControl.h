// Code/Tools/BF2Editor/Framework/SourceControl.h

// Source control behind an interface.
// NullSourceControl is registered and active by default; no Perforce dependency exists. The retail
// editor's ISourceControlManager (36 slots) and P4Wrapper.dll are a feature reference only (D3).

// The editor calls the active provider around every file it writes, creates or deletes:
//   openForEdit(path)  before overwriting an existing file
//   add(path)          after creating a new file
//   remove(path)       before deleting a file
// A provider that refuses returns ok == false with a message, and the editor does not touch the file.
#ifndef BF2EDITOR_FRAMEWORK_SOURCECONTROL_H
#define BF2EDITOR_FRAMEWORK_SOURCECONTROL_H

#include "Framework/EditorConfig.h"

#include <memory>
#include <string>
#include <vector>

namespace bf2editor
{

enum class ScmFileState
{
    NotControlled,      // no provider tracks this file (every file under NullSourceControl)
    Unchanged,          // tracked, not opened
    OpenForEdit,
    OpenForAdd,
    OpenForDelete,
    OutOfDate,          // tracked, and the workspace copy is older than the depot's
    Unknown             // the provider could not tell (for example, the server is unreachable)
};

struct ScmResult
{
    bool        ok;
    std::string message;    // why ok is false; may carry a note when ok is true
};

class ISourceControl
{
public:
    virtual ~ISourceControl() {}

    virtual const char*  name() const = 0;
    // Whether a workspace is connected. False means no file is under source control.
    virtual bool         isConnected() const = 0;
    virtual ScmFileState getState(const std::string& path) = 0;
    virtual ScmResult    openForEdit(const std::string& path) = 0;
    virtual ScmResult    add(const std::string& path) = 0;
    virtual ScmResult    remove(const std::string& path) = 0;
    // Discard local changes to a file.
    virtual ScmResult    revert(const std::string& path) = 0;
};

// No source control: files are edited in place.
//   openForEdit: succeeds for a writable or missing file; refuses a read-only file rather than
//                clearing the attribute (that is the user's decision, not the editor's).
//   add, remove: succeed; there is nothing to record.
//   revert:      refuses; without a provider there is no earlier version to restore.
class NullSourceControl : public ISourceControl
{
public:
    const char*  name() const override;
    bool         isConnected() const override;
    ScmFileState getState(const std::string& path) override;
    ScmResult    openForEdit(const std::string& path) override;
    ScmResult    add(const std::string& path) override;
    ScmResult    remove(const std::string& path) override;
    ScmResult    revert(const std::string& path) override;
};

typedef std::unique_ptr<ISourceControl> (*SourceControlFactory)();

// Registers a provider under a name (compared case-insensitively). "Null" is registered from the
// start. Returns false if the name is taken or the factory is null.
bool registerSourceControl(const std::string& name, SourceControlFactory factory);

// Makes the named provider active. Returns false, and keeps the current provider, when the name is
// unknown or its factory returns null.
bool selectSourceControl(const std::string& name);

// The active provider: NullSourceControl until selectSourceControl picks another.
ISourceControl& sourceControl();

// Registered provider names, in registration order.
std::vector<std::string> sourceControlNames();

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_SOURCECONTROL_H
