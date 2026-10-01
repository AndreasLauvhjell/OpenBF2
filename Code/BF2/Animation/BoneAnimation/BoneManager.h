// [INFERRED] Source placement.

// Registry of bone names and their numeric indices.


#ifndef ANIMATION_BONEANIMATION_BONEMANAGER_H     // [INFERRED] guard style
#define ANIMATION_BONEANIMATION_BONEMANAGER_H

#include <string>
#include <vector>

namespace dice {
namespace anim {

class BoneManager
{
public:
    // The guarded function-local static.
    static BoneManager& getInstance();


    // Interns name case-insensitively and returns its index. Skeleton templates narrow the result to a short.

    int addBoneName(const std::string& name);

    // The same linear scan without the append: -1 when the name is unknown.
    int getBoneNameIndex(const std::string& name) const;


    // Requires a nonnegative index. Only the upper bound is checked.

    const std::string& getBoneName(short index) const;

    // genuinely empty in this build - what survives is a bare loop over the
    // names with nothing in its body, which is what a release build leaves of a debug print.
    void debugInfo() const;

private:
    BoneManager();

    /* +0x00 */ std::vector<std::string> m_names;      // the intern table
    /* +0x0C */ std::string m_invalidName;             // "Invalid bone name index"
};

} // namespace anim
} // namespace dice

#endif // ANIMATION_BONEANIMATION_BONEMANAGER_H
