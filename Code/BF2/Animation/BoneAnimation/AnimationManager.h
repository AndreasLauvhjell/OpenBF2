// [INFERRED] Source placement.

// Caches bone animations by name and supports deferred loading.


#ifndef ANIMATION_BONEANIMATION_ANIMATIONMANAGER_H     // [INFERRED] guard style
#define ANIMATION_BONEANIMATION_ANIMATIONMANAGER_H

#include <map>
#include <string>

#include "../../IO/File/IFileSystem.h"     // hfe::NoCaseStringCompare - the map's comparator
#include "../../IO/File/FileChangeManager.h"

namespace dice {
namespace anim {

class BoneAnimation;

// Animation cache and file-change listener. Preserve the inherited interface order.
class AnimationManager : public hfe::io::IFileChangeListener
{
public:
    // IBase reference/query interface.
    virtual void  addRef();                                       // `mov eax,1` - void in the interface
    virtual int   getRef();                                       // 1
    virtual int   release();                                      // 0 - no count, no delete
    virtual void* queryInterface(unsigned int) const;             // 0 - answers nothing, not even IBase

    // name attested - the map's _Rb_tree mangling spells it
    // `dice::anim::AnimationManager::AnimationHolder`, so it is nested here. Eight bytes: the node
    // is 28 and the key string is 4.
    struct AnimationHolder
    {
        AnimationHolder() : wanted(false), animation(0) { }
        AnimationHolder(bool w, BoneAnimation* a) : wanted(w), animation(a) { }

        /* +0x00 */ bool wanted;               // [CHOSEN] name; the mark bit
        /* +0x04 */ BoneAnimation* animation;
    };
    typedef std::map<std::string, AnimationHolder, hfe::NoCaseStringCompare> AnimationMap;

    AnimationManager();
    virtual ~AnimationManager();                                  // D1/D0 at vt +0x14/+0x18

    // the choke point. Get-or-create by name; the create half skips the file while deferred.
    BoneAnimation* getAnimation(const std::string& name);

    int  getNumAnimationsToLoad();
    void postLoad();

    // seven bytes, `return 0` - it does nothing. See the block above.
    int  release(BoneAnimation* animation);
    // five bytes - empty. The manager registers as a FileChangeManager listener (its destructor
    // removes itself) but hot-reload does nothing on this build. IFileChangeListener's one slot.
    virtual void onFileChange(const std::string& path, unsigned int what); // vt +0x10

    // inline - AnimationSystemTemplateManager::hide() stores 0 straight into +0x10 from another
    // TU with no call, the same evidence that put ValueHolderManager::setActive in a header.
    void setActiveAnimation(BoneAnimation* a) { m_activeAnimation = a; }
    BoneAnimation* getActiveAnimation() const { return m_activeAnimation; }

    void setActiveAnimationFile(const std::string& name);
    const std::string& getActiveAnimationFile();
    const std::string& getName(BoneAnimation* animation);

    // [TEMP] [CHOSEN, not recovered] Looks up a cached animation's registered name for diagnostics.
    // [RETIRE-WHEN: bone-staging-index] Remove after validating animation lifetime and staging indices.
    const std::string* debugFindRegisteredName(const BoneAnimation* animation) const;
    int debugPostLoadCalls() const;

    const std::string& getShortName(BoneAnimation* animation);    // [FRONTIER]
    std::string listAnimations() const;
    // [UNRESOLVED] getAnimations takes its vector by value; no guessed output-reference API is exposed.

private:
    // Byte offsets for the x86 layout.
    /* +0x00 */ // the vptr - IFileChangeListener's, see the class note
    /* +0x04 */ AnimationMap m_animations;     // header ptr +0x04, count +0x08, comparator +0x0C
    /* +0x10 */ BoneAnimation* m_activeAnimation;   // ctor 0
    /* +0x14 */ bool m_deferLoading;           // ctor true. [CHOSEN] name, same role as its sibling's
};

// Created directly by AnimModule::init.
extern AnimationManager* animationManager;

} // namespace anim
} // namespace dice

#endif // ANIMATION_BONEANIMATION_ANIMATIONMANAGER_H
