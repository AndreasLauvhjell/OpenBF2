// [INFERRED] class/path. HUD owner +100: PE ctor 788540 / dtor 7883C0.
// Complete nonvirtual storage, with native STL. This is not a factory publication;
#ifndef MENU_HUDQUESTIONMANAGER_H
#define MENU_HUDQUESTIONMANAGER_H
#include "../Core/BuildConfig.h"
#if !DEDICATED
#include <vector>
namespace dice { namespace hfe { namespace hud {
// [INFERRED] record name/fields. PE78933B allocates 32 bytes and writes only
// +00..+17. No vptr, constructor call, or use of the trailing eight bytes appears
// in the question family. Clearing/removing records does not delete them.
struct HudQuestion {
    int category;
    int id;
    double createdAt;
    double remainingTime;
    unsigned char unknown18[8]; // [UNRESOLVED], deliberately uninitialized
};
class HudQuestionManager {
public:
    HudQuestionManager();
    ~HudQuestionManager(); // 7883C0, frees vectors, not their pointees
    void setDuration(int category, double duration);
    int getCount(int category);
    HudQuestion* selectQuestion();
    void removeQuestion(HudQuestion* question, bool redisplay);
    void clearCategory(int category);
    void clear();
    void update(float elapsed); // 789020, expiry/redisplay dependencies below
    // 789140, int/int/bool/float proven by the full body and 7A221B.
    // Clamps category 0..6, clears all for negative id, otherwise removes matching
    // ids; optional redisplay depends on selection7880D0 and dispatch787630.
    void removeQuestions(int category, int id, bool redisplay, float delay);
    void addQuestion(int category, int id, bool redisplay);
    void reply(bool affirmative); // 7891D0; selects then dispatches below
    void replyAll(bool affirmative); // 7891F0; silent drain of selected questions
    void replyCategory(int category, bool affirmative, bool silent, bool redisplay);

    void displayQuestion(HudQuestion* question);
    void expireQuestion(HudQuestion* question);
    void replyQuestion(HudQuestion*, bool affirmative, bool silent, bool redisplay);
private:
    friend struct HudQuestionFixture;
    void removeMatchingQuestions(int category, int id);
    std::vector<HudQuestion*> m_questions[7]; // PE 0..6F: 16-byte VC7.1 vectors
    double m_durations[7]; // PE 70..A7: {20,20,30,20,-1,15,-1}
    double m_displayDelay; // PE A8, constructor 0; 7891AA assigns the supplied delay
};
} } }
#endif
#endif
