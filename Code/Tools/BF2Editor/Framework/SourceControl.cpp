// Code/Tools/BF2Editor/Framework/SourceControl.cpp - see SourceControl.h. Editor-only code (D3, D4).
#include "Framework/EditorConfig.h"
#include "Framework/SourceControl.h"

#include <windows.h>

#include <cstring>
#include <utility>

namespace bf2editor
{

// NullSourceControl

const char* NullSourceControl::name() const
{
    return "Null";
}

bool NullSourceControl::isConnected() const
{
    return false;
}

ScmFileState NullSourceControl::getState(const std::string& /*path*/)
{
    return ScmFileState::NotControlled;
}

ScmResult NullSourceControl::openForEdit(const std::string& path)
{
    const DWORD attributes = GetFileAttributesA(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        const DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
            return ScmResult{ true, std::string() };            // a new file: nothing to open
        return ScmResult{ false, "cannot read the attributes of '" + path + "'" };
    }
    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
        return ScmResult{ false, "'" + path + "' is a directory, not a file" };
    if (attributes & FILE_ATTRIBUTE_READONLY)
        return ScmResult{ false, "'" + path + "' is read-only and no source control provider is "
                                 "active to open it for edit; clear the attribute to edit it in place" };
    return ScmResult{ true, std::string() };
}

ScmResult NullSourceControl::add(const std::string& /*path*/)
{
    return ScmResult{ true, std::string() };
}

ScmResult NullSourceControl::remove(const std::string& /*path*/)
{
    return ScmResult{ true, std::string() };
}

ScmResult NullSourceControl::revert(const std::string& path)
{
    return ScmResult{ false, "no source control provider is active, so there is no earlier version of '"
                             + path + "' to revert to" };
}

// The provider registry

namespace
{

std::unique_ptr<ISourceControl> createNull()
{
    return std::unique_ptr<ISourceControl>(new NullSourceControl());
}

struct Registry
{
    std::vector<std::pair<std::string, SourceControlFactory> > factories;
    std::unique_ptr<ISourceControl> active;

    Registry()
    {
        factories.push_back(std::make_pair(std::string("Null"), &createNull));
        active = createNull();
    }
};

Registry& registry()
{
    static Registry instance;
    return instance;
}

bool sameName(const std::string& a, const std::string& b)
{
    return a.size() == b.size() && _stricmp(a.c_str(), b.c_str()) == 0;
}

} // namespace

bool registerSourceControl(const std::string& name, SourceControlFactory factory)
{
    if (name.empty() || !factory)
        return false;
    Registry& r = registry();
    for (size_t i = 0; i < r.factories.size(); ++i)
    {
        if (sameName(r.factories[i].first, name))
            return false;
    }
    r.factories.push_back(std::make_pair(name, factory));
    return true;
}

bool selectSourceControl(const std::string& name)
{
    Registry& r = registry();
    for (size_t i = 0; i < r.factories.size(); ++i)
    {
        if (!sameName(r.factories[i].first, name))
            continue;
        std::unique_ptr<ISourceControl> provider = r.factories[i].second();
        if (!provider)
            return false;
        r.active = std::move(provider);
        return true;
    }
    return false;
}

ISourceControl& sourceControl()
{
    return *registry().active;
}

std::vector<std::string> sourceControlNames()
{
    const Registry& r = registry();
    std::vector<std::string> names;
    for (size_t i = 0; i < r.factories.size(); ++i)
        names.push_back(r.factories[i].first);
    return names;
}

} // namespace bf2editor
