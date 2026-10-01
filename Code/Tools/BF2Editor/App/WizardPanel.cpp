// Code/Tools/BF2Editor/App/WizardPanel.cpp - see WizardPanel.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/WizardPanel.h"

#include "App/EditSession.h"
#include "Framework/EditorConsole.h"
#include "Framework/EditorLog.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <cstdio>
#include <cstring>

namespace bf2editor
{

namespace
{

const char* const kCategories[] = { "General", "Car", "Tank", "Plane", "Helicopter" };
const ImVec4 kWarning(1.0f, 0.45f, 0.4f, 1.0f);

// A console argument that may hold blanks.
std::string quoted(const std::string& text)
{
    return text.find_first_of(" \t") == std::string::npos && !text.empty() ? text : "\"" + text + "\"";
}

} // namespace

WizardPanel::WizardPanel()
    : m_category(0)
    , m_choice(-1)
    , m_failed(false)
{
    m_answer[0] = 0;
}

bool WizardPanel::run(const std::string& line)
{
    const ConsoleLineResult r = runConsoleLine(line);
    logLine(r.failed ? LogLevel::Warning : LogLevel::Info, "wizard: %s", r.output.c_str());
    m_message = r.output;
    m_failed = r.failed;
    return !r.failed;
}

void WizardPanel::draw(EditSession& session, bool* open)
{
    // Opened beside the Object templates panel the first time, never floating over the view.
    ImGuiWindow* neighbour = ImGui::FindWindowByName("Object templates");
    if (!neighbour)
        neighbour = ImGui::FindWindowByName("Properties");
    if (neighbour && neighbour->DockId != 0)
        ImGui::SetNextWindowDockID(neighbour->DockId, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Object wizard", open))
    {
        ImGui::End();
        return;
    }
    ObjectWizard& wizard = session.wizard();
    ImGui::SetNextItemWidth(130.0f);
    ImGui::Combo("##category", &m_category, kCategories, static_cast<int>(sizeof(kCategories) / sizeof(kCategories[0])));
    ImGui::SameLine();
    const bool waiting = wizard.waiting() != ObjectWizard::Wait::None;
    ImGui::BeginDisabled(waiting);
    if (ImGui::Button("Run Wizard"))
        run(std::string("EditorWizard.run ") + kCategories[m_category]);
    ImGui::SameLine();
    if (ImGui::SmallButton("Reload rules"))
        run("EditorWizard.load");
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("The retail editor's rule files (bf2editor/Content/ObjectEditor/Wizards), read again.");
    if (!m_message.empty())
        ImGui::TextColored(m_failed ? kWarning : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "%s", m_message.c_str());
    if (!wizard.rootName().empty())
        ImGui::TextDisabled("Object: %s", wizard.rootName().c_str());

    // The findings.
    const std::vector<ObjectWizard::Item>& items = wizard.items();
    const char* const groups[] = { "Errors", "Warnings", "Utils", "Other (never executed)" };
    for (int g = 0; g < 4; ++g)
    {
        bool any = false;
        for (size_t i = 0; i < items.size() && !any; ++i)
            any = (g < 3 ? items[i].group == g : items[i].group < 0);
        if (!any)
            continue;
        ImGui::SeparatorText(groups[g]);
        for (size_t i = 0; i < items.size(); ++i)
        {
            const ObjectWizard::Item& item = items[i];
            if (g < 3 ? item.group != g : item.group >= 0)
                continue;
            ImGui::PushID(static_cast<int>(i));
            bool ticked = item.ticked;
            ImGui::BeginDisabled(item.readOnly || waiting);
            if (ImGui::Checkbox("##tick", &ticked))
            {
                char line[48];
                std::snprintf(line, sizeof(line), "EditorWizard.tick %u %d", static_cast<unsigned>(i), ticked ? 1 : 0);
                run(line);
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::TextUnformatted(item.label.c_str());
            if (!item.status.empty())
            {
                ImGui::SameLine();
                ImGui::TextDisabled("(%s)", item.status.c_str());
            }
            ImGui::PopID();
        }
    }
    if (!items.empty())
    {
        ImGui::BeginDisabled(waiting);
        if (ImGui::Button("Execute"))
            run("EditorWizard.execute");
        ImGui::EndDisabled();
        ImGui::SetItemTooltip("Runs the ticked items: Errors, Warnings, then Utils. Each item's fix is one undoable edit "
                              "of the template files.");
    }

    // What Execute waits for.
    if (waiting)
    {
        ImGui::SeparatorText(wizard.promptTitle().c_str());
        ImGui::TextWrapped("%s", wizard.promptText().c_str());
        switch (wizard.waiting())
        {
        case ObjectWizard::Wait::Text:
        case ObjectWizard::Wait::Float:
            if (m_answer[0] == 0 && !wizard.promptPreset().empty())
                std::snprintf(m_answer, sizeof(m_answer), "%s", wizard.promptPreset().c_str());
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##answer", m_answer, sizeof(m_answer));
            if (ImGui::Button("OK") && run("EditorWizard.answer " + quoted(m_answer)))
                m_answer[0] = 0;
            break;
        case ObjectWizard::Wait::Choice:
        {
            const std::vector<std::string>& choices = wizard.promptChoices();
            for (size_t i = 0; i < choices.size(); ++i)
                if (ImGui::Selectable(choices[i].c_str(), m_choice == static_cast<int>(i)))
                    m_choice = static_cast<int>(i);
            if (ImGui::Button("OK") && m_choice >= 0 && m_choice < static_cast<int>(choices.size())
                && run("EditorWizard.answer " + quoted(choices[static_cast<size_t>(m_choice)])))
                m_choice = -1;
            break;
        }
        case ObjectWizard::Wait::Checklist:
        {
            const std::vector<std::string>& rows = wizard.promptChoices();
            std::vector<bool>& ticks = wizard.checklist();
            for (size_t i = 0; i < rows.size() && i < ticks.size(); ++i)
            {
                bool on = ticks[i];
                ImGui::PushID(static_cast<int>(i) + 1000);
                if (ImGui::Checkbox(rows[i].c_str(), &on))
                    ticks[i] = on;
                ImGui::PopID();
            }
            if (ImGui::Button("OK"))
                run("EditorWizard.checklistOk");
            break;
        }
        case ObjectWizard::Wait::None:
            break;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            run("EditorWizard.cancel");
            m_answer[0] = 0;
            m_choice = -1;
        }
    }
    const std::vector<std::string>& report = wizard.report();
    if (!report.empty())
    {
        ImGui::SeparatorText("Last execute");
        for (size_t i = 0; i < report.size(); ++i)
            ImGui::TextWrapped("%s", report[i].c_str());
    }
    ImGui::End();
}

} // namespace bf2editor
