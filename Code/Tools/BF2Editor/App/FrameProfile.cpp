// Code/Tools/BF2Editor/App/FrameProfile.cpp - see FrameProfile.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/FrameProfile.h"

#include "imgui.h"

#include <cfloat>
#include <cstdio>

namespace bf2editor
{

namespace
{

struct PhaseSeries
{
    const char*          name;
    float FramePhases::* member;
};

const PhaseSeries kPhases[] = {
    { "total",  &FramePhases::total },
    { "world",  &FramePhases::world },
    { "ui",     &FramePhases::ui },
    { "render", &FramePhases::render },
    { "idle",   &FramePhases::idle },
};

struct PlotSource
{
    const FramePhases* frames;      // oldest first, `count` of them
    float FramePhases::* member;
};

float plotValue(void* data, int index)
{
    const PlotSource* source = static_cast<const PlotSource*>(data);
    return source->frames[index].*(source->member);
}

} // namespace

FrameProfile::FrameProfile()
    : m_next(0)
    , m_count(0)
    , m_paused(false)
{
}

void FrameProfile::push(const FramePhases& phases)
{
    if (m_paused)
        return;
    m_frames[m_next] = phases;
    m_next = (m_next + 1) % kFrames;
    if (m_count < kFrames)
        ++m_count;
}

const FramePhases& FrameProfile::at(int age) const
{
    return m_frames[(m_next - 1 - age + 2 * kFrames) % kFrames];
}

void FrameProfile::draw(bool* open)
{
    if (!ImGui::Begin("Profiler", open))
    {
        ImGui::End();
        return;
    }
    ImGui::Checkbox("Pause", &m_paused);
    ImGui::SameLine();
    ImGui::TextDisabled("the last %d frames, in milliseconds", m_count);
    if (m_count == 0)
    {
        ImGui::End();
        return;
    }

    // Oldest first, for the plots.
    static FramePhases ordered[kFrames];
    for (int i = 0; i < m_count; ++i)
        ordered[i] = at(m_count - 1 - i);

    if (ImGui::BeginTable("phases", 4, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingFixedFit))
    {
        ImGui::TableSetupColumn("phase");
        ImGui::TableSetupColumn("last");
        ImGui::TableSetupColumn("average");
        ImGui::TableSetupColumn("worst");
        ImGui::TableHeadersRow();
        for (size_t p = 0; p < sizeof(kPhases) / sizeof(kPhases[0]); ++p)
        {
            float sum = 0.0f, worst = 0.0f;
            for (int i = 0; i < m_count; ++i)
            {
                const float value = ordered[i].*(kPhases[p].member);
                sum += value;
                if (value > worst)
                    worst = value;
            }
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(kPhases[p].name);
            ImGui::TableNextColumn();
            ImGui::Text("%.2f", ordered[m_count - 1].*(kPhases[p].member));
            ImGui::TableNextColumn();
            ImGui::Text("%.2f", sum / static_cast<float>(m_count));
            ImGui::TableNextColumn();
            ImGui::Text("%.2f", worst);
        }
        ImGui::EndTable();
    }

    const float width = ImGui::GetContentRegionAvail().x;
    for (size_t p = 0; p < sizeof(kPhases) / sizeof(kPhases[0]); ++p)
    {
        PlotSource source = { ordered, kPhases[p].member };
        char label[32];
        std::snprintf(label, sizeof(label), "##%s", kPhases[p].name);
        char overlay[48];
        std::snprintf(overlay, sizeof(overlay), "%s %.2f ms", kPhases[p].name, ordered[m_count - 1].*(kPhases[p].member));
        ImGui::PlotLines(label, &plotValue, &source, m_count, 0, overlay, 0.0f, p == 0 ? 33.4f : FLT_MAX,
                         ImVec2(width, p == 0 ? 60.0f : 40.0f));
    }
    ImGui::End();
}

} // namespace bf2editor
