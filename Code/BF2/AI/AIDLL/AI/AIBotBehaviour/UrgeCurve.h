// Urgency curve interfaces and constant, linear and inverse implementations.
// [CHOSEN, not recovered] The related curve implementations are grouped in this translation unit.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_URGECURVE_H       // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_URGECURVE_H

namespace dice {
namespace hfe {
namespace ai {

// Base interface for evaluating behavior urgency.
class UrgeCurve
{
public:
    virtual ~UrgeCurve();
    virtual float calculate(float) = 0;     // slot 4, `__cxa_pure_virtual` in the shipped vtable
};

// `operator new(8)` in `BotBehaviourManager::createUCConstant`.
// +0 vptr
// +4 float m_value [INFERRED] name; the constructor's only store, `calculate`'s only read
// `calculate` is eleven bytes: `mov eax, this; fld [eax+4]; retn`. It does not clamp - the constant
// is returned exactly as given, negative values included. The other two curves do clamp. Preserved.
class UCConstant : public UrgeCurve
{
public:
    explicit UCConstant(float value);
    virtual ~UCConstant();

    virtual float calculate(float);

private:
    float m_value;
};

// `operator new(12)` in `createUCLinear`.
// +0 vptr
// +4 float m_slope [INFERRED] first constructor argument
// +8 float m_offset [INFERRED] second constructor argument
// calculate(x) = max(m_slope * x + m_offset, 0.0f)
class UCLinear : public UrgeCurve
{
public:
    UCLinear(float slope, float offset);
    virtual ~UCLinear();

    virtual float calculate(float);

private:
    float m_slope;
    float m_offset;
};

// Inverse urgency curve with numerator, offset and slope parameters.
class UCXInverse : public UrgeCurve
{
public:
    UCXInverse(float numerator, float offset, float slope, float bias);
    virtual ~UCXInverse();

    virtual float calculate(float);

private:
    float m_numerator;
    float m_offset;
    float m_slope;
    float m_bias;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_URGECURVE_H
