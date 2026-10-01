
namespace dice {
namespace hfe {
namespace ai {

// This unsupported ammunition query emits the shipped fatal diagnostic.
int AIObjectUnit::getAmmo() const
{
    Debug(DebugTypeError, "AI/AIInterface/BFKnowledge/AIObjectUnit.inl", 24, "")
        << "Does not work, is this really used?";
    return 0;
}

} // namespace ai
} // namespace hfe
} // namespace dice
