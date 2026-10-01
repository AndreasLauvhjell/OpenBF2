// Samples a supplied curve function into a table for later evaluation.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_CUSTOMCURVE_H       // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_CUSTOMCURVE_H

namespace dice {
namespace hfe {
namespace ai {

// Static curve table with no per-instance state. Construction initializes the shared
// table and destruction releases it.
class SCurve
{
public:
    SCurve();                          // - calls init, nothing else
    ~SCurve();
    static void  init();
    static float calculate(float x);   // - the only one with a NaN guard
    static float* curveValues;
};

class JCurve
{
public:
    JCurve();
    ~JCurve();
    static void  init();
    static float calculate(float x);   // - extrapolates above 1
    static float* curveValues;
};

class DecleiningSlopeCurve             // dice's spelling
{
public:
    DecleiningSlopeCurve();
    ~DecleiningSlopeCurve();
    static void  init();
    static float calculate(float x);
    static float* curveValues;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_CUSTOMCURVE_H
