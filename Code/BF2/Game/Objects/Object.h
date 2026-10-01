// Runtime world object: hierarchy, components, transforms and script persistence.
// Base update, input and message callbacks are empty extension points; the collision default accepts the event.
// [INFERRED] Header placement; runtime implementation is in Scene/Object/Object.cpp.

#ifndef GAME_OBJECTS_OBJECT_H     // [INFERRED] guard style
#define GAME_OBJECTS_OBJECT_H

#include <string>
#include <map>

#include "IObject.h"
#include "../../Core/SmartPtr.h"
#include "../../IO/Console/IConsoleSaveable.h"
#include "../../Math/Mat4.h"
#include "../../Math/LocalBoundingBox.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace io { class IStream; class PlayerInput; }
namespace world {

class CullerObject;
class CollisionFaceInfo;
class IPlayer;
class ObjectManager;
enum UpdateFreqencyType : int { UpdateFreqencyType_UNRECOVERED = 0 };   // [sic] dice's spelling
enum ObjectLodType : int      { ObjectLodType_UNRECOVERED = 0 };

IObject* getRootParent(const IObject*);
void setBoundRadiusIsDirty(IObject*);

// Finds by template name, not by the runtime object's name.
IObject* findObjectWithName(IObject* object, const std::string& name);

// Base runtime object implementation. Multiple interface pointers address different
// subobjects; use typed conversions. Keep packed state bytes separate and preserve
// field ordering where layout access depends on it.
class Object : public IObject, public io::IConsoleSaveable
{
public:
    explicit Object(const IObjectTemplate* objectTemplate);
    static IBase* create(unsigned int iid, IBase* outer);

    // IBase + identity
    virtual void  addRef();                                          // +0x00
    virtual int   getRef();                                          // +0x04
    virtual int   release();                                         // +0x08
    virtual void* queryInterface(unsigned int) const;                // +0x0C
    virtual unsigned int getClassID() const;                         // +0x10
    virtual ~Object();                                               // +0x14 +0x18, 442/454 B

    // lifecycle
    virtual void init();                                             // +0x20
    virtual void disableCachedObject();                              // +0x24
    virtual void enableCachedObject();                               // +0x28
    virtual void setObjectCacheID(unsigned int);                     // +0x30
    virtual void destroy(bool force);                                // +0x34
    virtual bool isObjectDestroyed();                                // +0x3C
    virtual void setObjectInDestroyList(bool);                       // +0x40
    virtual bool getObjectInDestroyList() const;                     // +0x44

    // components
    // setComponent returns bool (IObject.h had `void`) and is 1,686 B — it is not a map insert.
    virtual bool  setComponent(unsigned int classId, IBase*);        // +0x58
    // TWO ids — (classId, interfaceId). Confirms the shape python::wrapObject calls it with.
    virtual IBase* queryComponent(unsigned int classId,
                                  unsigned int iid);                 // +0x5C
    virtual void updateFlags(unsigned int set,
                             unsigned int clear);                    // +0x60

    // naming
    virtual const std::string& getName() const;                      // +0x64
    virtual void setName(const std::string&);                        // +0x68

    // transform
    virtual const Vec3& getAbsolutePosition() const;                 // +0x6C
    virtual void  setAbsolutePosition(const Vec3&);                  // +0x70
    virtual const Vec3& getAbsolutePositionSecondary() const;        // +0x74
    virtual void  setAbsolutePositionSecondary(const Vec3&);         // +0x78
    virtual const Vec3& getRotationSecondary();                      // +0x7C non-const
    virtual void  setRotationSecondary(const Vec3&);                 // +0x80
    virtual const Mat4& getAbsoluteTransformation() const;           // +0x84
    virtual const Mat4& getInvAbsoluteTransformation() const;        // +0x88
    virtual void  setAbsoluteTransformation(const Mat4&);            // +0x8C
    virtual const Mat4& getVisualAbsoluteTransformation() const;     // +0x90
    virtual const Vec3& getRelativePosition() const;                 // +0xE0
    virtual void  setRelativePosition(const Vec3&);                  // +0xE4
    virtual const Mat4& getRelativeTransformation() const;           // +0xE8
    virtual void  setRelativeTransformation(const Mat4&);            // +0xEC

    // bounds
    virtual float getVisualBoundingRadius() const;                   // +0x94
    virtual float getPhysicBoundingRadius() const;                   // +0x98
    virtual float getBoundingRadiusModifier() const;                 // +0x9C
    virtual LocalBoundingBox* getFullBoundingBox();                  // +0xA0
    virtual LocalBoundingBox* getMineBoundingBox();                  // +0xA4

    // the extension points — ALL empty stubs here
    virtual void handleVisualUpdate(float, float);                   // +0xA8 empty
    virtual void handleFrameUpdate(float);                           // +0xAC empty
    virtual void handleUpdate(float, unsigned int);                  // +0xB0 empty
    virtual bool handleCollision(IObject*, const Vec3&, const Vec3&, const Vec3&,
                                 int, int, CollisionFaceInfo*,
                                 CollisionFaceInfo*);                // +0xB4 9 params
    virtual void handlePlayerInput(IPlayer*, const io::PlayerInput&,
                                   float, unsigned int);             // +0x100 empty
    virtual void handleMessage(TemplateMessage, IPlayer*, bool);     // +0x104 empty

    // geometry / LOD
    virtual void updateGeometry();                                   // +0xB8
    virtual int getUpdateFrequency() const;                          // +0xBC
    virtual UpdateFreqencyType getUpdateFrequencyType() const;       // +0xC0
    virtual void setUpdateFrequencyType(UpdateFreqencyType);         // +0xC4
    virtual int  getMaterial() const;                                // +0xD0
    // [sic] capital S — and Object overrides it; IObject's version is the stub.
    virtual void SetActiveLod(ObjectLodType);                        // +0xD4

    // scene graph
    virtual IObject* getChild() const;                               // +0xF0
    virtual void addChild(IObject*);                                 // +0xF4
    virtual void removeChild(IObject*);                              // +0xF8
    virtual void resetCachedRootParents();                           // +0xFC
    virtual void addSibling(IObject*);                               // +0x15C
    virtual void removeSibling(IObject*);                            // +0x160
    virtual void setParent(IObject*);                                // +0x164 does work

    // small per-object fields (all 15-17 B — trivial get/set)
    // Note the char types: input, child and layer ids are single bytes, not ints.
    virtual char getInputId() const;                                 // +0x108
    virtual void setInputId(char);                                   // +0x10C
    virtual char getChildId() const;                                 // +0x110
    virtual void setChildId(char);                                   // +0x114
    virtual unsigned char getAlpha();                                // +0x118
    virtual unsigned char getLayer();                                // +0x11C
    virtual void setLayer(unsigned char);                            // +0x120
    virtual unsigned char getGroup();                                // +0x124
    virtual void setGroup(unsigned char);                            // +0x128
    virtual void setIsOvergrowth(bool);                              // +0x12C
    virtual bool getIsOvergrowth();                                  // +0x130
    // It always reads GSGameMode and compares gpm_coop before storing; true then destroys the
    // object when the external AI exists OR the mode is coop. No same-value early-out.
    virtual void setNotInAI(bool);       // +0x134 / +0x130
    virtual bool getNotInAI() const;                                 // +0x138
    virtual void setLightSourceMask(int);                            // +0x13C
    virtual int  getLightSourceMask() const;                         // +0x140
    virtual bool getDisableChildren();                               // +0x144
    virtual void setDisableChildren(bool);                           // +0x148 does work
    // Writes the runtime object's .con representation; ignores write failures and returns true.
    virtual bool makeScript(io::IStream*);                           // +0x168
    virtual char getLayerId() const;                                 // +0x16C
    virtual void setLayerId(char);                                   // +0x170
    virtual void updateFlagsRecursive(unsigned int set,
                                      unsigned int clear);           // +0x174
    virtual void initNetworkableComp();                              // +0x178 empty
    virtual void analyseNetworkableObject();                         // +0x17C empty

    static void internalDestroyObject(IObject*);
    static ObjectManager* mObjectManager;

protected:
    typedef std::map<unsigned int, SmartPtr<IBase> > BaseMap;
    typedef std::map<unsigned int, SmartPtr<IObjectComponent> > ComponentMap;

    int m_refCount;                              // ELF/PE +0x84
    Mat4 m_absoluteTransformation;               // +0x88
    Mat4* m_invertedAbsoluteTransformation;      // +0xC8 owned
    Mat4* m_relativeTransformation;              // +0xCC owned for children; shared identity at root
    Vec3 m_absolutePositionSecondary;            // +0xD0
    Vec3 m_rotationSecondary;                    // +0xDC
    float m_visualBoundingRadius;                 // +0xE8
    float m_physicBoundingRadius;                 // +0xEC
    BaseMap m_bases;                              // +0xF0 in the shipped ELF STL
    ComponentMap m_components;                   // +0xFC in the shipped ELF STL
    std::string m_name;                           // +0x108
    int m_updateFrequency;                        // +0x10C
    UpdateFreqencyType m_updateFrequencyType;     // +0x110
    unsigned int m_cacheFlags;                    // +0x114
    LocalBoundingBox* m_fullBoundingBox;          // +0x118 owned
    LocalBoundingBox* m_mineBoundingBox;          // +0x11C owned
    signed char m_inputId;                        // +0x120 1 sentinel
    signed char m_childId;                        // +0x121 1 sentinel
    signed char m_layerId;                        // +0x122 1 sentinel
    unsigned char m_layer;                        // +0x123
    unsigned char m_group;                        // +0x124
    bool m_isObjectDestroyed;                     // +0x125
    bool m_unknown126;                            // +0x126 [UNRESOLVED]
    bool m_isOvergrowth;                          // +0x127
    bool m_notInAI;                               // +0x128
    bool m_disableChildren;                       // +0x129
    bool m_objectInDestroyList;                   // +0x12A

private:
    bool buildFullBoundingBox();
    bool buildMineBoundingBox();
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_OBJECT_H
