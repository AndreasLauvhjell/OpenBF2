// [TEMP] Component checks for the actual question storage/selection bodies.
// Display/reply collaborators remain uncalled /FORCE dependencies, never fixture bodies.
#include "../BF2/Menu/HudQuestionManager.h"
#include "../BF2/Core/System.h"
#include <cstddef>
#include <cstdio>
#include <limits>
#include <vector>

namespace dice { namespace hfe { namespace hud {
struct HudQuestionFixture {
    static std::vector<HudQuestion*>& rows(HudQuestionManager& m, int i) { return m.m_questions[i]; }
    static double duration(HudQuestionManager& m, int i) { return m.m_durations[i]; }
    static double delay(HudQuestionManager& m) { return m.m_displayDelay; }
};
} } }
using namespace dice::hfe;
using namespace dice::hfe::hud;
static unsigned checks, failures;
static void check(bool value, const char* message) {
    ++checks;
    if (!value) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); }
}

int main() {
    static_assert(sizeof(void*) == 4, "x86 fixture required");
    static_assert(sizeof(HudQuestion) == 32, "retail POD allocation");
    static_assert(offsetof(HudQuestion, createdAt) == 8, "question timestamp");
    static_assert(offsetof(HudQuestion, remainingTime) == 16, "question countdown");
    HudQuestionManager manager;
    const double defaults[] = {20,20,30,20,-1,15,-1};
    for (int i = 0; i != 7; ++i) {
        check(manager.getCount(i) == 0, "empty category");
        check(HudQuestionFixture::duration(manager, i) == defaults[i], "duration default");
    }
    check(HudQuestionFixture::delay(manager) == 0, "initial display delay");
    check(manager.selectQuestion() == nullptr, "empty selection");
    manager.setDuration(-100, 4.5);
    manager.setDuration(100, -1);
    check(HudQuestionFixture::duration(manager, 0) == 4.5, "duration lower clamp");
    check(HudQuestionFixture::duration(manager, 6) == -1, "duration upper clamp");

    WorldPref::resetWorldTime(50.0);
    manager.addQuestion(-4, 7, false);
    HudQuestion* first = HudQuestionFixture::rows(manager, 0)[0];
    check(first->category == 0 && first->id == 7, "add clamp and id");
    check(first->createdAt == 50 && first->remainingTime == 4.5, "add captures clock and duration");
    check(manager.getCount(-8) == 1, "count lower clamp");
    manager.addQuestion(88, 8, false);
    HudQuestion* sixth = HudQuestionFixture::rows(manager, 6)[0];
    check(sixth->category == 6 && manager.getCount(88) == 1, "add/count upper clamp");
    check(manager.selectQuestion() == first, "equal timestamp keeps first despite shorter duration");
    WorldPref::resetWorldTime(51.0);
    manager.addQuestion(0, 9, false);
    HudQuestion* replacement = HudQuestionFixture::rows(manager, 0)[0];
    check(manager.getCount(0) == 1 && replacement != first, "add replaces category rows");
    check(first->id == 7 && first->remainingTime == 4.5, "replaced record is retained");
    check(manager.selectQuestion() == replacement, "newer equal-duration first record");
    manager.clear();
    delete first; delete sixth; delete replacement;
    for (int i = 0; i != 7; ++i)
        check(HudQuestionFixture::rows(manager,i).capacity() == 0, "clear releases all backing allocations");

    HudQuestion a = {0, 1, 40, 9, {}};
    HudQuestion b = {1, 2, 45, 10, {}};
    HudQuestion c = {2, 3, 46, 8, {}};
    HudQuestion invalid = {3, -1, 50, -1, {}};
    HudQuestionFixture::rows(manager,0).push_back(&a);
    HudQuestionFixture::rows(manager,1).push_back(&b);
    HudQuestionFixture::rows(manager,2).push_back(&c);
    HudQuestionFixture::rows(manager,3).push_back(nullptr);
    HudQuestionFixture::rows(manager,3).push_back(&invalid);
    check(manager.selectQuestion() == &c, "selection requires newer age and shorter countdown");
    c.remainingTime = 11;
    check(manager.selectQuestion() == &a, "newer candidate with larger countdown rejected");
    a.remainingTime = -1;
    check(manager.selectQuestion() == &b, "minus-one selected duration permits newer longer duration");
    a.createdAt = -949;
    b.createdAt = -949;
    c.createdAt = -949;
    check(manager.selectQuestion() == nullptr, "age exactly1000 fails strict threshold");
    a.createdAt = 70;
    check(manager.selectQuestion() == &a, "future timestamp remains eligible");
    a.remainingTime = std::numeric_limits<double>::quiet_NaN();
    check(manager.selectQuestion() == nullptr, "NaN duration fails comparisons");
    a.remainingTime = 1;
    a.createdAt = std::numeric_limits<double>::quiet_NaN();
    check(manager.selectQuestion() == nullptr, "NaN timestamp fails comparison");
    a.id = -2;
    a.createdAt = 70;
    check(manager.selectQuestion() == nullptr, "negative id is excluded");
    manager.clear();

    HudQuestion x = {2, 77, 0, 10, {}};
    HudQuestion y = {2, 77, 0, 20, {}};
    HudQuestion z = {2, 88, 0, 30, {}};
    auto& rows = HudQuestionFixture::rows(manager,2);
    rows.push_back(&x); rows.push_back(&x); rows.push_back(&z);
    manager.removeQuestion(&x, false);
    check(rows.size() == 2 && rows[0] == &x && rows[1] == &z, "single removal erases first equal pointer only");
    manager.removeQuestion(&y, false);
    check(rows.size() == 2, "absent pointer does not match by id");
    manager.removeQuestion(nullptr, false);
    check(rows.size() == 2, "null single removal guard");
    rows.clear(); rows.push_back(&x); rows.push_back(&y); rows.push_back(&z);
    manager.removeQuestions(2, 77, false, 4);
    check(rows.size() == 2 && rows[0] == &y && rows[1] == &z,
          "retail id-removal iterator skips adjacent shifted match");
    check(HudQuestionFixture::delay(manager) == 0, "no redisplay leaves delay untouched");
    manager.removeQuestions(2, -1, true, 1.25f);
    check(rows.empty() && rows.capacity() == 0, "negative id releases category allocation");
    check(HudQuestionFixture::delay(manager) == 1.25, "positive redisplay delay stored as double");
    manager.update(0.25f);
    check(HudQuestionFixture::delay(manager) == 1.0, "delay decremented by frame delta");
    manager.removeQuestions(2, -1, false, 80);
    check(HudQuestionFixture::delay(manager) == 1.0, "false redisplay preserves active delay");
    manager.removeQuestions(99, -1, true, 9.5f);
    check(HudQuestionFixture::delay(manager) == 9.5, "empty category still schedules display");

    x.remainingTime = 10; y.remainingTime = -1; z.remainingTime = 0;
    rows.push_back(&x); rows.push_back(&y); rows.push_back(nullptr); rows.push_back(&z);
    manager.update(0.5f);
    check(x.remainingTime == 9.5, "positive question countdown ticks");
    check(y.remainingTime == -1 && z.remainingTime == 0, "nonpositive countdowns remain unchanged");
    check(HudQuestionFixture::delay(manager) == 9.0, "delay ticks after question walk");
    manager.update(-0.5f);
    check(x.remainingTime == 10 && HudQuestionFixture::delay(manager) == 9.5, "negative delta increases positive timers");
    manager.clearCategory(2);
    check(rows.capacity() == 0 && x.id == 77, "clear leaves borrowed records intact");
    check(HudQuestionFixture::delay(manager) == 9.5, "clear category preserves display delay");
    manager.clear();
    check(HudQuestionFixture::duration(manager,0) == 4.5, "clear does not restore duration settings");
    {
        HudQuestionManager inner;
        HudQuestionFixture::rows(inner, 1).push_back(&b);
    }
    check(b.id == 2, "destructor leaves pointed-to records intact");
    std::printf("hud questions: %u/%u checks passed\n", checks-failures, checks);
    return failures ? 1 : 0;
}
