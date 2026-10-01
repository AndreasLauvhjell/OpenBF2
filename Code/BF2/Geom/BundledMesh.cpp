// Path attested: BF2 1.50 RendDX9.dll string1021B300 names Geom/BundledMesh.cpp.
// PE-only renderer. No dedicated ELF body exists.
#ifdef BF2_IMAGE_RENDDX9
#include "BundledMesh.h"
#include "BundledMeshTemplate.h"
#include "BundledMeshMaterial.h"
#include "BundledMeshStateManager.h"
#include "StaticMesh.h"
#include "StaticMeshRenderer.h"
#include "XpLightManager.h"
#include "../../RendDX9Abi/BfMemory.h"
#include "../../RendDX9Abi/Frontier.h"
#include "../../RendDX9Abi/PluginEntry.h"
#include "../RendDX9/RendDX9Base.h"
#include "../RendDX9/ShaderParameter.h"
#if BF2_PLUGIN_ABI_NATIVE
#include "../Game/Objects/IObject.h"
#include "../Game/Objects/IObjectTemplate.h"
#include "../Game/Objects/ObjectLayoutAccess.h"
#endif
#include <cfloat>
#include <cmath>
#include <cstring>
#include <new>

namespace dice { namespace hfe {
namespace dx9rend {
extern void* g_rendererBase;
extern IBase* g_Renderer;
void vertexBufferBindStream(void*,unsigned int);
void indexBufferBind(void*);
void vertexDeclarationBind(void*);
// Shared 10005820 wrapper. Parameter names follow its actual body, not IDA's glued calls.
void rendererDrawIndexedPrimitive(unsigned int,unsigned int,int,unsigned int,unsigned int,unsigned int);
}
namespace geom {
extern LightManager* g_LightManager;
namespace {
template<class R,class... A> R dispatch(void* self,unsigned int offset,A... args)
{
    typedef R(__fastcall* Fn)(void*,void*,A...);
    return reinterpret_cast<Fn>((*static_cast<void***>(self))[offset/4])(self,0,args...);
}
template<class T> T& field(void* p,unsigned int offset)
{ return *reinterpret_cast<T*>(static_cast<unsigned char*>(p)+offset); }

// A host object's geometry: IObject's +0x38 field. Typed in the native flavour (RW-266); the
// retail-ABI flavour keeps the raw offset, which IObject's retail-aligned prefix shares.
void* objectGeometryField(void* object)
{
#if BF2_PLUGIN_ABI_NATIVE
    return world::object_detail::LayoutAccess::geometry(static_cast<world::IObject*>(object));
#else
    return field<void*>(object,0x38);
#endif
}

// 100A6170, including its 2^-46 squared-length threshold. Unordered comparisons
// take the same x87 refusal/normalization arms; the callers discard the status code.
int normalizeXZ(Vec2& value)
{
    const float lengthSquared=value.x*value.x+value.y*value.y;
    if (!(std::fabs(static_cast<double>(lengthSquared)-1.0)<FLT_EPSILON))
    {
        if (std::fabs(lengthSquared)<1.4210854715202004e-14f)
        { value.x=0; value.y=0; return -20001; }
        const double factor=1.0/std::sqrt(static_cast<double>(lengthSquared));
        value.x=static_cast<float>(factor*value.x);
        value.y=static_cast<float>(factor*value.y);
    }
    return 0;
}
void allocateUvMatrices(BundledMesh& self)
{
    if (!self.m_uvMatrices)
    {
        self.m_uvMatrices=static_cast<Mat4*>(::renddx9abi::engineAlloc(8*sizeof(Mat4)));
        if (self.m_uvMatrices)
            for (int i=0;i<8;++i) new(self.m_uvMatrices+i) Mat4;
        for (int i=0;i<8;++i) self.m_uvMatrices[i]=Mat4::getIdentity();
    }
}
void uvRangeDiagnostic(int index,unsigned int line)
{
    ::renddx9abi::logf("Geom/BundledMesh.cpp:%u TBM: UVMatrixIndex is out of range %d",line,index);
}
}

const Mat4& bundledMeshZeroMatrix()
{
    // four zero rows, distinct from the identity used by getTransformation.
    static Mat4 result;
    static const bool initialized=[](){ std::memset(result.m,0,sizeof(result.m)); return true; }();
    (void)initialized;
    return result;
}

BundledMesh::BundledMesh(BundledMeshTemplate* objectTemplate)
    : GeometryInstanceData(g_bundledMeshVtable)
{
    // Explicit fields retain the constructor's untouched allocator/padding words.
    m_refCount=1;
    dx9rend::Ra::raObjectParametersConstruct(&m_parameters);
    m_template=objectTemplate; m_partTransforms=0; m_distanceToCamera=0; m_fov=-1;
    m_heightOverTerrain=0; m_shadowSunHeight=0; m_underRoof=0; m_heightmapCluster=0;
    m_horizontalForward.x=0; m_horizontalForward.y=0;
    m_lodRecord=0; m_lodMaterials=0; m_geomIndex=0; m_lodIndex=0; m_animationIndex=0;
    m_dword29C=-1; m_dword2A0=0; m_dword2A4=0; m_uvMatrices=0; m_ownerObject=0;
    m_shadowCasterFrame=0; m_shadowProjection=0; m_nextGeometry=0;
    m_lights.first=m_lights.last=m_lights.end=0;
    m_lightRecords.first=m_lightRecords.last=m_lightRecords.end=0;
    m_shadowFadeTime=0; m_castsShadow=0; m_animationExplicit=0; m_lodExplicit=0;
    m_lodFrame=0; m_heightFrame=0; m_shadowFade=1; m_inverseTransform=0;
    if (m_template->m_loaded) initializeFromTemplate();
    m_heightmapCluster=classManager->getSingleton(renddx9abi::HostString("HeightmapCluster"));
}
BundledMesh::~BundledMesh()
{
    // No template reference/count decrement exists in this body.
    m_vptr=g_bundledMeshVtable;
    if (m_inverseTransform) ::renddx9abi::engineFree(m_inverseTransform);
    ::renddx9abi::engineFree(m_partTransforms);
    ::renddx9abi::engineFree(m_uvMatrices);
    vc71::setEraseValue(m_template->m_instances,this);
    if (m_lightRecords.first) ::renddx9abi::engineFree(m_lightRecords.first);
    m_lightRecords.first=m_lightRecords.last=m_lightRecords.end=0;
    if (m_lights.first) ::renddx9abi::engineFree(m_lights.first);
    m_lights.first=m_lights.last=m_lights.end=0;
    dx9rend::Ra::raObjectParametersDestroy(&m_parameters);
    // The base-class vptr restore is not observable: like StaticMesh, this is a flat ABI object.
}
void* bundledMeshConstruct(void* storage,BundledMeshTemplate* templ)
{ return new(storage) BundledMesh(templ); }
void bundledMeshInitTemplate(void* instance)
{ static_cast<BundledMesh*>(instance)->initializeFromTemplate(); }
void* bundledMeshDestroyFlags(void* instance,unsigned char flags)
{
    static_cast<BundledMesh*>(instance)->~BundledMesh();
    if (flags&1) ::renddx9abi::engineFree(instance);
    return instance;
}
int BundledMesh::addRef() { return ++m_refCount; }
int BundledMesh::getRef() const { return m_refCount; }
int BundledMesh::release()
{
    const int remaining=--m_refCount;
    if (!remaining) dispatch<void*>(this,0x34,1u);
    return remaining;
}
void* BundledMesh::queryInterface(unsigned int iid)
{ return iid==1u || iid==1227874558u || iid==2915824892u ? this : 0; }
void BundledMesh::setFov(float value) { m_fov=value; }
float BundledMesh::getFov() const { return m_fov; }
void* BundledMesh::setShadowProjection(void* value) { return m_shadowProjection=value; }
void* BundledMesh::getShadowProjection() const { return m_shadowProjection; }
int BundledMesh::getGeometryType() const { return m_lodRecord->renderKind; }
BundledMeshTemplate* BundledMesh::getTemplate() const { return m_template; }
const void* BundledMesh::getPartBounds(int part) const { return m_template->m_partBounds[part]; }
const void* BundledMesh::getLocalBounds() const
{
    // Exact initialized bytes at102396F0: zero bounds with dirty=1; only one code xref.
    static MeshBounds empty={};
    static const bool initialized=[](){ empty.dirty=1; return true; }();
    (void)initialized;
    return m_lodRecord ? &m_lodRecord->bounds : &empty;
}
float BundledMesh::getBoundingRadius() const
{
    if (!m_lodRecord) return 0;
    if (m_lodRecord->bounds.dirty) meshBoundsFinalize(m_lodRecord->bounds);
    return m_lodRecord->bounds.radius;
}
const Mat4* BundledMesh::getGeometryTransformation() const
{ return m_partTransforms ? m_partTransforms[0] : &Mat4::getIdentity(); }
void BundledMesh::allocatePartTransforms()
{
    if (!m_partTransforms)
    {
        m_partTransforms=static_cast<const Mat4**>(::renddx9abi::engineAlloc(4*m_template->m_maxPartCount));
        for (unsigned int i=0;i<m_template->m_maxPartCount;++i)
            m_partTransforms[i]=&bundledMeshZeroMatrix();
    }
}
void BundledMesh::setPartTransformation(const Mat4* value,int part)
{
    // Signed comparison; the shipped setter has no negative-index guard.
    if (m_template->m_loaded && part<static_cast<int>(m_template->m_maxPartCount))
    { allocatePartTransforms(); m_partTransforms[part]=value; }
}
void BundledMesh::setGeometryTransformation(const Mat4* value)
{
    if (m_template->m_loaded)
    {
        allocatePartTransforms(); m_partTransforms[0]=value;
        m_horizontalForward.x=m_partTransforms[0]->m[8];
        m_horizontalForward.y=m_partTransforms[0]->m[10];
        normalizeXZ(m_horizontalForward);
    }
}
void BundledMesh::setBoneTransforms(const Mat4* value,int)
{
    if (!m_partTransforms) return;
    if (value)
        for (unsigned int i=1;i<m_template->m_maxPartCount;++i) m_partTransforms[i]=value+i;
    else
        for (unsigned int i=1;i<m_template->m_maxPartCount;++i) m_partTransforms[i]=&bundledMeshZeroMatrix();
}
unsigned int BundledMesh::getPartCount() const { return m_lodRecord ? m_lodRecord->partCount : 0; }
void BundledMesh::setGeomIndex(int geom)
{
    if (!m_template->m_loaded || !m_template->m_geoms.first) return;
    const unsigned int count=static_cast<unsigned int>((m_template->m_geoms.last-m_template->m_geoms.first)/32);
    if (static_cast<unsigned int>(geom)>=count || geom<0) return;
    if (m_geomIndex!=static_cast<unsigned int>(geom)) { m_lodIndex=0; m_lodExplicit=0; }
    m_geomIndex=geom;
    vc71::vector_layout& materials=reinterpret_cast<vc71::vector_layout*>(m_template->m_materials.first)[geom];
    const unsigned int lods=materials.first ? static_cast<unsigned int>((materials.last-materials.first)/16) : 0;
    if (m_lodIndex>=lods-1) m_lodIndex=lods-1;
    m_lodMaterials=reinterpret_cast<vc71::vector_layout*>(materials.first)+m_lodIndex;
    m_lodRecord=reinterpret_cast<BundledMeshLod*>(
        reinterpret_cast<BundledMeshGeom*>(m_template->m_geoms.first)[geom].lods.first)+m_lodIndex;
}
int BundledMesh::getGeomIndex() const { return static_cast<int>(m_geomIndex); }
int BundledMesh::getGeomCount() const
{ return m_template->m_geoms.first ? static_cast<int>((m_template->m_geoms.last-m_template->m_geoms.first)/32) : 0; }
void BundledMesh::setLodIndex(int lod)
{
    if (lod==-1) { m_lodExplicit=0; m_lodIndex=0; }
    else { m_lodIndex=lod; m_lodExplicit=1; }
    dispatch<void>(this,0x6C,static_cast<int>(m_geomIndex));
}
int BundledMesh::getLodIndex() const { return static_cast<int>(m_lodIndex); }
int BundledMesh::getLodCount() const
{
    const vc71::vector_layout& lods=reinterpret_cast<BundledMeshGeom*>(m_template->m_geoms.first)[m_geomIndex].lods;
    return lods.first ? static_cast<int>((lods.last-lods.first)/72) : 0;
}
int BundledMesh::setAnimationIndex(int value)
{
    if (value==-1) { m_animationExplicit=0; return m_animationIndex=0; }
    m_animationExplicit=1; return m_animationIndex=value;
}
int BundledMesh::getAnimationIndex() const { return m_animationIndex; }
int BundledMesh::getMaterialCount() const
{ return m_lodMaterials && m_lodMaterials->first ? static_cast<int>((m_lodMaterials->last-m_lodMaterials->first)/4) : 0; }
void BundledMesh::setUvTranslation(int index,float x,float y)
{
    // All three setters really use OR, admitting many out-of-range indices (retail defect).
    if (index>=0 || index<static_cast<int>(m_template->m_dword10C+1))
    { allocateUvMatrices(*this); m_uvMatrices[index].m[8]=x; m_uvMatrices[index].m[9]=y; }
    else uvRangeDiagnostic(index,1423);
}
void BundledMesh::setUvRotation(int index,float angle)
{
    if (index>=0 || index<static_cast<int>(m_template->m_dword10C+1))
    {
        const float c=static_cast<float>(std::cos(angle)),s=static_cast<float>(std::sin(angle));
        allocateUvMatrices(*this);
        m_uvMatrices[index].m[0]=c; m_uvMatrices[index].m[1]=-s;
        m_uvMatrices[index].m[4]=s; m_uvMatrices[index].m[5]=c;
    }
    else uvRangeDiagnostic(index,1443);
}
void BundledMesh::setUvTransformation(int index,const Mat4* value)
{
    if (index>=0 || index<static_cast<int>(m_template->m_dword10C+1))
    { allocateUvMatrices(*this); m_uvMatrices[index]=*value; }
    else uvRangeDiagnostic(index,1467);
}
void* BundledMesh::setOwnerObject(void* value) { return m_ownerObject=value; }
void* BundledMesh::refreshRootObject()
{
    void* root=staticMeshRootParent(m_ownerObject);
#if BF2_PLUGIN_ABI_NATIVE
    // The native MSVC table also places getFlags at +44; prefer its typed host contract.
    m_castsShadow=root && (static_cast<world::IObject*>(root)->getTemplate()->getFlags()&0x800) ? 1 : 0;
#else
    m_castsShadow=root && (dispatch<unsigned int>(field<void*>(root,0x14),0x44)&0x800) ? 1 : 0;
#endif
    return root;
}
bool BundledMesh::castsDynamicShadow()
{ dispatch<void*>(this,0xA4); return m_castsShadow!=0; }
void BundledMesh::setShadowCasterFrame(unsigned int value)
{ if (m_ownerObject && m_ownerObject==dispatch<void*>(this,0xA4)) m_shadowCasterFrame=value; }
unsigned int BundledMesh::getShadowCasterFrame()
{
    void* root=dispatch<void*>(this,0xA4);
    if (root && root!=m_ownerObject && objectGeometryField(root))
        return dispatch<unsigned int>(objectGeometryField(root),0xB0);
    return m_shadowCasterFrame;
}
int BundledMesh::setDword29C(int value) { return m_dword29C=value; }
void* BundledMesh::setNextGeometry(void* value) { return m_nextGeometry=value; }
void* BundledMesh::getNextGeometry() const { return m_nextGeometry; }
void BundledMesh::setTransparency(float value) { m_parameters.transparency.value=value; }
void BundledMesh::initializeFromTemplate()
{
    if (m_template->m_geoms.first && static_cast<unsigned int>((m_template->m_geoms.last-m_template->m_geoms.first)/32)>=2)
        m_geomIndex=1;
    dispatch<void>(this,0x6C,static_cast<int>(m_geomIndex));
    if (m_template->m_maxPartCount==1 && m_ownerObject)
#if BF2_PLUGIN_ABI_NATIVE
        dispatch<void>(this,0x50,&static_cast<world::IObject*>(m_ownerObject)->getAbsoluteTransformation());
#else
        dispatch<void>(this,0x50,dispatch<const Mat4*>(m_ownerObject,0x80));
#endif
}
float BundledMesh::animationPhase(const Vec3& camera)
{
    Vec2 direction={camera.x-m_partTransforms[0]->m[12],camera.z-m_partTransforms[0]->m[14]};
    const Vec2 side={m_partTransforms[0]->m[0],m_partTransforms[0]->m[2]};
    normalizeXZ(direction);
    double result=0.25-(direction.y*m_horizontalForward.y+direction.x*m_horizontalForward.x)*0.249975f;
    if (side.y*direction.y+side.x*direction.x<0) result=1.0-result;
    return static_cast<float>(result);
}
unsigned int BundledMesh::animationIndex(const Vec3& camera)
{
    const unsigned int count=m_template->m_u2;
    const unsigned int index=static_cast<unsigned int>(static_cast<int>(animationPhase(camera)*static_cast<double>(count)));
    return index>=count ? m_template->m_u2-1 : index;
}
unsigned int BundledMesh::materialFeatures(BundledMeshMaterial* material,bool additive,unsigned int caps)
{
    const BundledMeshState& state=*material->m_state;
    unsigned int bits=0;
    if(state.m_animated)bits=2; if(state.m_normalMap)bits|=4; if(state.m_extraMap)bits|=8;
    if(state.m_envMap)bits|=0x10; if(state.m_cockpit)bits|=0x20;
    if(!additive && !state.m_noHemiLight)bits|=0x40;
    if(!bits || bits==0x20)bits|=1;
    if(!additive && dispatch<bool>(this,0xA8))
    {
        const unsigned int frame=dispatch<unsigned int>(this,0xB0);
        if(frame==dx9rend::rendererFrameOrdinal() && !state.m_cockpit)bits|=0x80;
    }
    if(state.m_colorMapGloss)bits|=0x100; if(state.m_feature39)bits|=0x200; if(state.m_alphaTest)bits|=0x400;
    if(caps>=3 && !dispatch<bool>(dx9rend::g_Renderer,0xAC) && (bits&0x80))bits|=0x1000;
    if(additive)bits|=0x800;
    return bits;
}
void BundledMesh::updateLodSelection()
{
    const unsigned int frame=dx9rend::rendererFrameOrdinal();
    if(m_lodFrame==frame || m_lodExplicit)return;
    m_lodFrame=frame;
    void* view=dispatch<void*>(dx9rend::g_rendererBase,0x48);
    BundledMeshGeom& geom=reinterpret_cast<BundledMeshGeom*>(m_template->m_geoms.first)[m_geomIndex];
    MeshBounds& bounds=reinterpret_cast<BundledMeshLod*>(geom.lods.first)->bounds;
    if(bounds.dirty)meshBoundsFinalize(bounds);
    const float radius=bounds.radius;
    const float threshold=dispatch<float>(view,0x2C)*m_distanceToCamera-radius;
    const unsigned int count=geom.distances.first ? static_cast<unsigned int>((geom.distances.last-geom.distances.first)/4):0;
    const float* distances=reinterpret_cast<const float*>(geom.distances.first);
    unsigned int selected=0;
    for(unsigned int i=0;i<count;++i)
        if(distances[i]*field<float>(dx9rend::g_Renderer,0x84C)*field<float>(dx9rend::g_Renderer,0x8B4)<threshold)selected=i+1;
    if(selected!=m_lodIndex) { m_lodIndex=selected; dispatch<void>(this,0x6C,static_cast<int>(m_geomIndex)); }
    m_shaderLod=0;
    if(field<unsigned char>(dx9rend::g_Renderer,0x708))
        for(int i=3;i>=0;--i)
            if(threshold>field<float>(dx9rend::g_Renderer,0x72C+4*i)) { m_shaderLod=i+1; break; }
}

void BundledMesh::drawShadowMap(void* light,void*,void*,void*,void*,void*)
{
    // The profiling-only bracket follows the existing mesh policy;
    // it is [CHOSEN, not recovered] instrumentation debt, not a rendering fallback.
    if(!m_template->m_loaded || !m_partTransforms)return;
    updateLodSelection();
    const int originalGeom=dispatch<int>(this,0x70);
    int shadowGeom=originalGeom;
    if(dispatch<int>(this,0x74)==3 && originalGeom==0)shadowGeom=1;
    const int originalLod=dispatch<int>(this,0x7C);
    if(originalGeom!=shadowGeom)dispatch<void>(this,0x6C,shadowGeom);
    int shadowLod=g_LightManager->getBundledMeshShadowLod();
    const int currentLod=dispatch<int>(this,0x7C);
    if(shadowLod<currentLod)shadowLod=currentLod;
    const int lodCount=dispatch<int>(this,0x80);
    if(currentLod!=shadowLod)dispatch<void>(this,0x78,lodCount-1<shadowLod ? lodCount-1 : shadowLod);
    Mat4 combined;
    const unsigned int materialCount=m_lodMaterials->first ? static_cast<unsigned int>((m_lodMaterials->last-m_lodMaterials->first)/4):0;
    dx9rend::vertexDeclarationBind(m_template->m_vertexDeclaration);
    dx9rend::vertexBufferBindStream(m_template->m_vertexBuffer,0);
    dx9rend::indexBufferBind(m_template->m_indexBuffer);
    // 1001E3A0 returns a borrowed parameter, unlike the counted 1001E420 owner.
    dx9rend::ShaderParameter* unpack=dx9rend::effectGetShaderParameterBorrowedTyped(g_bundledShadowEffect,vc71::string("POSUNPACK"));
    const float scale[4]={m_template->m_decompressScale.x,m_template->m_decompressScale.y,m_template->m_decompressScale.z,1};
    unpack->setValue(scale,0);
    if(light && field<unsigned char>(light,0x495))
    {
        combined=Mat4::getIdentity();
        combined.fullMult(field<Mat4>(light,0x238),field<Mat4>(light,0x278));
        dx9rend::effectGetShaderParameterBorrowedTyped(g_bundledShadowEffect,vc71::string("vpLightMat"))->setValue(&combined,0);
        dx9rend::effectGetShaderParameterBorrowedTyped(g_bundledShadowEffect,vc71::string("vpLightTrapezMat"))->setValue(&combined,0);
        void* d3dx=field<void*>(g_bundledShadowEffect,8);
        typedef void(__stdcall* Commit)(void*);
        reinterpret_cast<Commit>((*static_cast<void***>(d3dx))[0x104/4])(d3dx);
    }
    else
    {
        dx9rend::effectGetShaderParameterBorrowedTyped(g_bundledShadowEffect,vc71::string("vpLightMat"))->setValue(&field<Mat4>(m_shadowProjection,0x238),0);
        dx9rend::effectGetShaderParameterBorrowedTyped(g_bundledShadowEffect,vc71::string("vpLightTrapezMat"))->setValue(&field<Mat4>(m_shadowProjection,0x278),0);
    }
    for(unsigned int i=0;i<materialCount;++i)
    {
        BundledMeshMaterial* material=reinterpret_cast<BundledMeshMaterial**>(m_lodMaterials->first)[i];
        if(material->m_kind==1 || material->m_kind==2)continue;
        g_bundledOneBoneSkinning->setMatrixPointerArray(reinterpret_cast<const void* const*>(m_partTransforms),m_lodRecord->partCount);
        // Keep the wrapper's declared argument order: primitive count, type, base,
        // minimum index, vertex count, then start index.
        dx9rend::rendererDrawIndexedPrimitive(material->m_primitiveCount,m_template->m_primitiveType,
            material->m_vertexStart,0,material->m_vertexCount,material->m_indexStart);
        field<unsigned int>(dispatch<void*>(dx9rend::g_rendererBase,0x6C),0x24C)+=material->m_primitiveCount;
    }
    if(originalGeom!=shadowGeom)dispatch<void>(this,0x6C,originalGeom);
    if(originalLod!=shadowLod)dispatch<void>(this,0x78,originalGeom ? -1 : originalLod);
}

namespace {
#define BM0(R,name) R __fastcall bm_##name(BundledMesh* s,void*) { return s->name(); }
#define BM1(R,name,T) R __fastcall bm_##name(BundledMesh* s,void*,T v) { return s->name(v); }
BM0(int,addRef) BM0(int,getRef) BM0(int,release) BM1(void*,queryInterface,unsigned int)
BM1(void,setFov,float) BM0(float,getFov)
BM1(void*,setShadowProjection,void*) BM0(void*,getShadowProjection) BM0(int,getGeometryType)
BM0(BundledMeshTemplate*,getTemplate) BM1(const void*,getPartBounds,int)
BM0(const void*,getLocalBounds) BM0(float,getBoundingRadius) BM0(const Mat4*,getGeometryTransformation)
BM1(void,setGeometryTransformation,const Mat4*) BM0(unsigned int,getPartCount)
BM1(void,setGeomIndex,int) BM0(int,getGeomIndex) BM0(int,getGeomCount)
BM1(void,setLodIndex,int) BM0(int,getLodIndex) BM0(int,getLodCount)
BM1(int,setAnimationIndex,int) BM0(int,getAnimationIndex) BM0(int,getMaterialCount)
BM1(void*,setOwnerObject,void*) BM0(void*,refreshRootObject) BM0(bool,castsDynamicShadow)
BM1(void,setShadowCasterFrame,unsigned int) BM0(unsigned int,getShadowCasterFrame)
BM1(int,setDword29C,int) BM1(void*,setNextGeometry,void*) BM0(void*,getNextGeometry)
BM1(void,setTransparency,float)
#undef BM0
#undef BM1
void __fastcall bm_draw(BundledMesh* s,void*,void* a,void* b,void* c) { s->draw(a,b,c); }
void __fastcall bm_shadow(BundledMesh* s,void*,void* a,void* b,void* c,void* d,void* e,void* f) { s->drawShadowMap(a,b,c,d,e,f); }
void* __fastcall bm_destroy(void* s,void*,unsigned char flags) { return bundledMeshDestroyFlags(s,flags); }
void __fastcall bm_setPart(BundledMesh* s,void*,const Mat4* value,int part) { s->setPartTransformation(value,part); }
void __fastcall bm_setBones(BundledMesh* s,void*,const Mat4* value,int count) { s->setBoneTransforms(value,count); }
void __fastcall bm_setName(BundledMesh* s,void*,const vc71::string& value) { s->m_name=value; }
void __fastcall bm_uvTranslate(BundledMesh* s,void*,int i,float x,float y) { s->setUvTranslation(i,x,y); }
void __fastcall bm_uvRotate(BundledMesh* s,void*,int i,float angle) { s->setUvRotation(i,angle); }
void __fastcall bm_uvTransform(BundledMesh* s,void*,int i,const Mat4* value) { s->setUvTransformation(i,value); }
// Shared retail bodies: exact callee cleanup confirmed in the instruction census.
void __fastcall bm_empty1(void*,void*,unsigned int) {}
void __fastcall bm_empty2(void*,void*,unsigned int,unsigned int) {}
bool __fastcall bm_false1(void*,void*,unsigned int) { return false; }
bool __fastcall bm_false0(void*,void*) { return false; }
int __fastcall bm_zero0(void*,void*) { return 0; }
int __fastcall bm_zero1(void*,void*,unsigned int) { return 0; }
int __fastcall bm_seven(void*,void*) { return 7; }
void __fastcall bm_stamp(BundledMesh* s,void*) { s->m_visibleFrame=dx9rend::rendererFrameOrdinal(); }
}
#define BM_ENTRY(x) reinterpret_cast<void*>(&bm_##x)
void* const g_bundledMeshVtable[62]={
    BM_ENTRY(addRef),BM_ENTRY(getRef),BM_ENTRY(release),BM_ENTRY(queryInterface),
    BM_ENTRY(setFov),BM_ENTRY(getFov),BM_ENTRY(draw),BM_ENTRY(empty2),
    BM_ENTRY(shadow),BM_ENTRY(setShadowProjection),BM_ENTRY(getShadowProjection),BM_ENTRY(getGeometryType),
    BM_ENTRY(false1),BM_ENTRY(destroy),BM_ENTRY(getTemplate),BM_ENTRY(getPartBounds),
    BM_ENTRY(getLocalBounds),BM_ENTRY(getBoundingRadius),BM_ENTRY(getGeometryTransformation),BM_ENTRY(setPart),
    BM_ENTRY(setGeometryTransformation),BM_ENTRY(setBones),BM_ENTRY(empty1),BM_ENTRY(getPartCount),
    BM_ENTRY(empty2),BM_ENTRY(empty1),BM_ENTRY(setName),BM_ENTRY(setGeomIndex),
    BM_ENTRY(getGeomIndex),BM_ENTRY(getGeomCount),BM_ENTRY(setLodIndex),BM_ENTRY(getLodIndex),
    BM_ENTRY(getLodCount),BM_ENTRY(setAnimationIndex),BM_ENTRY(getAnimationIndex),BM_ENTRY(getMaterialCount),
    BM_ENTRY(uvTranslate),BM_ENTRY(uvRotate),BM_ENTRY(uvTransform),BM_ENTRY(setOwnerObject),
    BM_ENTRY(zero0),BM_ENTRY(refreshRootObject),BM_ENTRY(castsDynamicShadow),BM_ENTRY(setShadowCasterFrame),
    BM_ENTRY(getShadowCasterFrame),BM_ENTRY(setDword29C),BM_ENTRY(empty1),BM_ENTRY(false0),
    BM_ENTRY(empty2),BM_ENTRY(zero0),BM_ENTRY(setNextGeometry),BM_ENTRY(getNextGeometry),
    BM_ENTRY(false0),BM_ENTRY(empty1),BM_ENTRY(false0),BM_ENTRY(empty1),
    BM_ENTRY(empty1),BM_ENTRY(seven),BM_ENTRY(zero0),BM_ENTRY(zero1),BM_ENTRY(stamp),BM_ENTRY(setTransparency)
};
#undef BM_ENTRY
} } }
#endif
