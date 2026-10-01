// Implementation path attested by BF2 1.50 RendDX9.dll __FILE__ at 0x1021B300.
// PE-only client renderer: constructor10104D10, destructor10104E90, table1021B350.
// Names not present in symbols are [INFERRED] from the callers/field operations.
#ifndef BF2_GEOM_BUNDLEDMESH_H
#define BF2_GEOM_BUNDLEDMESH_H
#ifdef BF2_IMAGE_RENDDX9
#include "../../RendDX9Abi/Vc71String.h"
#include "../../RendDX9Abi/Vc71Vector.h"
#include "../Math/Mat4.h"
#include "GeometryInstanceData.h"
#include "../RendDX9/Ra/RaShader.h"
#include <cstddef>

namespace dice { namespace hfe { namespace geom {
class BundledMeshTemplate;
class BundledMeshMaterial;
struct BundledMeshLod;

// The flat, explicit PE interface follows StaticMesh/SkinnedMesh's shared renderer ABI.
// Every primary slot is accounted for, including the shipped shared empty/constant bodies.
class BundledMesh : public GeometryInstanceData
{
public:
    explicit BundledMesh(BundledMeshTemplate* objectTemplate);
    ~BundledMesh();
    int addRef();
    int getRef() const;
    int release();
    void* queryInterface(unsigned int iid);
    void setFov(float value);
    float getFov() const;
    void draw(void*,void*,void*);
    void drawShadowMap(void* light,void*,void*,void*,void*,void*);
    void* setShadowProjection(void* value);
    void* getShadowProjection() const;
    int getGeometryType() const;                          // +2C,10103FE0 (selected lod)
    BundledMeshTemplate* getTemplate() const;
    const void* getPartBounds(int part) const;             // +3C,10107130
    const void* getLocalBounds() const;
    float getBoundingRadius() const;
    const Mat4* getGeometryTransformation() const;
    void setPartTransformation(const Mat4* value,int part);// +4C,10104940
    void setGeometryTransformation(const Mat4* value);
    void setBoneTransforms(const Mat4* value,int count);    // +54,10103E00 (count discarded)
    unsigned int getPartCount() const;                    // +5C,10103DA0
    void setGeomIndex(int geom);                          // +6C,101049B0
    int getGeomIndex() const;
    int getGeomCount() const;
    void setLodIndex(int lod);
    int getLodIndex() const;                              // +7C,10107180
    int getLodCount() const;
    int setAnimationIndex(int value);
    int getAnimationIndex() const;
    int getMaterialCount() const;                        // +8C,10104470
    void setUvTranslation(int index,float x,float y);
    void setUvRotation(int index,float angle);
    void setUvTransformation(int index,const Mat4* value);
    void* setOwnerObject(void* value);                    // +9C,10103FF0
    void* refreshRootObject();                            // +A4,10104000
    bool castsDynamicShadow();                           // +A8,10104060
    void setShadowCasterFrame(unsigned int value);        // +AC,10104080
    unsigned int getShadowCasterFrame();                  // +B0,101040B0
    int setDword29C(int value);                           // +B4,101071A0, role unresolved
    void* setNextGeometry(void* value);                   // +C8,101071B0
    void* getNextGeometry() const;                        // +CC,101071C0
    void setTransparency(float value);                    // +F4,10107210

    void allocatePartTransforms();
    void initializeFromTemplate();
    void updateHeightOverTerrain();
    float animationPhase(const Vec3& camera);
    unsigned int animationIndex(const Vec3& camera);
    unsigned int materialFeatures(BundledMeshMaterial* material,bool additive,unsigned int caps);
    void updateLodSelection();

    // +000..02F: inherited GeometryInstanceData, including the two frame-word aliases.
    int m_refCount;                                      // +030
    dx9rend::Ra::RaObjectParameters m_parameters;          // +034..257
    BundledMeshTemplate* m_template;                      // +258
    const Mat4** m_partTransforms;                        // +25C
    Mat4* m_inverseTransform;                             // +260, lazily allocated by draw
    float m_distanceToCamera;                            // +264
    float m_fov;                                         // +268
    float m_heightOverTerrain;                           // +26C
    float m_shadowSunHeight;                             // +270
    unsigned char m_underRoof;                           // +274
    unsigned char m_pad275[3];
    void* m_heightmapCluster;                             // +278
    Vec2 m_horizontalForward;                           // +27C
    BundledMeshLod* m_lodRecord;                         // +284
    vc71::vector_layout* m_lodMaterials;                  // +288 vector<BundledMeshMaterial*>
    unsigned int m_geomIndex;                            // +28C
    unsigned int m_lodIndex;                             // +290
    int m_animationIndex;                               // +294
    unsigned int m_shaderLod;                            // +298, ctor untouched
    int m_dword29C;                                      // +29C
    unsigned int m_dword2A0,m_dword2A4;                  // +2A0,+2A4, unresolved
    Mat4* m_uvMatrices;                                  // +2A8 eight matrices, lazy
    void* m_ownerObject;                                 // +2AC
    unsigned int m_shadowCasterFrame;                    // +2B0
    void* m_shadowProjection;                            // +2B4
    void* m_nextGeometry;                                // +2B8
    vc71::vector_layout m_lights;                        // +2BC
    vc71::vector_layout m_lightRecords;                  // +2CC dx9rend::Ra::RaLightRecord
    unsigned char m_castsShadow;                        // +2DC
    unsigned char m_animationExplicit;                   // +2DD
    unsigned char m_lodExplicit;                         // +2DE
    unsigned char m_pad2DF;
    unsigned int m_lodFrame;                             // +2E0
    unsigned int m_heightFrame;                          // +2E4
    float m_shadowFade;                                  // +2E8
    unsigned char m_pad2EC[4];
    double m_shadowFadeTime;                             // +2F0
};
static_assert(sizeof(BundledMesh)==0x2F8,"10104D10 allocation");
#define BF2_BM_OFFSET(member,offset) static_assert(offsetof(BundledMesh,member)==offset,#member)
BF2_BM_OFFSET(m_name,0x4); BF2_BM_OFFSET(m_refCount,0x30);
BF2_BM_OFFSET(m_parameters,0x34); BF2_BM_OFFSET(m_template,0x258);
BF2_BM_OFFSET(m_partTransforms,0x25C); BF2_BM_OFFSET(m_inverseTransform,0x260);
BF2_BM_OFFSET(m_distanceToCamera,0x264); BF2_BM_OFFSET(m_fov,0x268);
BF2_BM_OFFSET(m_heightOverTerrain,0x26C); BF2_BM_OFFSET(m_shadowSunHeight,0x270);
BF2_BM_OFFSET(m_underRoof,0x274); BF2_BM_OFFSET(m_heightmapCluster,0x278);
BF2_BM_OFFSET(m_horizontalForward,0x27C); BF2_BM_OFFSET(m_lodRecord,0x284);
BF2_BM_OFFSET(m_lodMaterials,0x288); BF2_BM_OFFSET(m_geomIndex,0x28C);
BF2_BM_OFFSET(m_lodIndex,0x290); BF2_BM_OFFSET(m_animationIndex,0x294);
BF2_BM_OFFSET(m_shaderLod,0x298); BF2_BM_OFFSET(m_dword29C,0x29C);
BF2_BM_OFFSET(m_uvMatrices,0x2A8); BF2_BM_OFFSET(m_ownerObject,0x2AC);
BF2_BM_OFFSET(m_shadowCasterFrame,0x2B0); BF2_BM_OFFSET(m_shadowProjection,0x2B4);
BF2_BM_OFFSET(m_nextGeometry,0x2B8); BF2_BM_OFFSET(m_lights,0x2BC);
BF2_BM_OFFSET(m_lightRecords,0x2CC); BF2_BM_OFFSET(m_castsShadow,0x2DC);
BF2_BM_OFFSET(m_lodFrame,0x2E0); BF2_BM_OFFSET(m_heightFrame,0x2E4);
BF2_BM_OFFSET(m_shadowFade,0x2E8); BF2_BM_OFFSET(m_shadowFadeTime,0x2F0);
#undef BF2_BM_OFFSET
extern void* const g_bundledMeshVtable[62];
const Mat4& bundledMeshZeroMatrix();                       // 10079290, shared zero matrix
bool bundledMeshTraceAdditive(BundledMesh*,const Vec3& origin,const Vec3& direction,
                              void*& hitObject);
} } }
#endif
#endif
