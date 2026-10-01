// Per-type replication policy and its owning registry. [INFERRED] Header placement.
// Preserve virtual order; native STL layouts and destructor slots differ by platform.
// [UNRESOLVED] PredictionMode enumerator names.

#ifndef IO_NETWORK_NETWORKABLEINFO_H     // [INFERRED] guard style
#define IO_NETWORK_NETWORKABLEINFO_H

#include <string>
#include <list>
#include <map>
#include <vector>
#include <iosfwd>       // the PredictionMode stream operators

#include "../../Core/IBase.h"
#include "../../Core/Hash.h"

namespace dice {
namespace hfe {
namespace io {

// Replication prediction modes; unrecognized stream values use PMNone.
enum PredictionMode
{
    PMNone       = 0,
    PMLinear     = 1,
    PMCubic      = 2,
    PMUsePhysics = 3
};
std::istream& operator>>(std::istream& in, PredictionMode& mode);
std::ostream& operator<<(std::ostream& out, PredictionMode mode);

// Replication-policy interface. Keep the orientation and controller accessors after the destructor.
class INetworkableInfo : public IBase
{
public:
    virtual void           setName(const std::string& name)   = 0;   // vptr +0x10
    virtual const std::string& getName() const                = 0;   // vptr +0x14
    virtual int            getID() const                      = 0;   // vptr +0x18
    virtual void           setID(int id)                      = 0;   // vptr +0x1C
    virtual void           setPredictionMode(PredictionMode)  = 0;   // vptr +0x20
    virtual PredictionMode getPredictionMode() const          = 0;   // vptr +0x24
    virtual void           setBasePriority(float priority)    = 0;   // vptr +0x28
    virtual float          getBasePriority() const            = 0;   // vptr +0x2C
    virtual void           setIsUnique(bool unique)           = 0;   // vptr +0x30
    virtual bool           getIsUnique() const                = 0;   // vptr +0x34
    virtual void           setForceNetworkableId(bool force)  = 0;   // vptr +0x38
    virtual bool           getForceNetworkableId() const      = 0;   // vptr +0x3C

    virtual ~INetworkableInfo() {}                                   // vptr +0x40 / +0x44

    virtual int  getIsControlledBy() const                    = 0;   // vptr +0x48
    virtual void setIsControlledBy(int who)                   = 0;   // vptr +0x4C
    virtual bool getHasOrientation() const                    = 0;   // vptr +0x50
    virtual void setHasOrientation(bool has)                  = 0;   // vptr +0x54
    virtual bool getHasYaw() const                            = 0;   // vptr +0x58
    virtual bool getHasPitch() const                          = 0;   // vptr +0x5C
    virtual bool getHasRoll() const                           = 0;   // vptr +0x60
    virtual void setHasYaw(bool has)                          = 0;   // vptr +0x64
    virtual void setHasPitch(bool has)                        = 0;   // vptr +0x68
    virtual void setHasRoll(bool has)                         = 0;   // vptr +0x6C

    // [CHOSEN, not recovered] Name for the usage flag accessed through this interface.
    // The manager initializes it after construction.
    bool m_isUsed;
};

// NetworkableInfo - the PER-type replication policy. Shared between every networkable of a type,
// refcounted, owned by NetworkableInfoManager, reached from io::INetworkable's +0x08.
class NetworkableInfo : public INetworkableInfo
{
public:
    NetworkableInfo();

    virtual void  addRef();                                   // +0x00
    virtual int   getRef();                                   // +0x04
    virtual int   release();                                  // +0x08
    virtual void* queryInterface(unsigned int iid) const;     // +0x0C

    virtual void  setName(const std::string& name);           // +0x10
    virtual const std::string& getName() const;               // +0x14
    virtual int   getID() const;                              // +0x18
    virtual void  setID(int id);                              // +0x1C
    virtual void  setPredictionMode(PredictionMode mode);     // +0x20
    virtual PredictionMode getPredictionMode() const;         // +0x24
    virtual void  setBasePriority(float priority);            // +0x28
    virtual float getBasePriority() const;                    // +0x2C
    virtual void  setIsUnique(bool unique);                   // +0x30
    virtual bool  getIsUnique() const;                        // +0x34
    virtual void  setForceNetworkableId(bool force);          // +0x38
    virtual bool  getForceNetworkableId() const;              // +0x3C

    virtual ~NetworkableInfo();                               // +0x40
                                                              // +0x44

    virtual int   getIsControlledBy() const;                  // +0x48
    virtual void  setIsControlledBy(int who);                 // +0x4C
    virtual bool  getHasOrientation() const;                  // +0x50
    virtual void  setHasOrientation(bool has);                // +0x54
    virtual bool  getHasYaw() const;                          // +0x58
    virtual bool  getHasPitch() const;                        // +0x5C
    virtual bool  getHasRoll() const;                         // +0x60
    virtual void  setHasYaw(bool has);                        // +0x64
    virtual void  setHasPitch(bool has);                      // +0x68
    virtual void  setHasRoll(bool has);                       // +0x6C

private:
    /* +0x08 */ int         m_refCount;             // starts at 1
    /* +0x0C */ int         m_isControlledBy;       // starts at -1
    /* +0x10 */ bool        m_hasOrientation;
    /* +0x11 */ bool        m_hasYaw;
    /* +0x12 */ bool        m_hasPitch;
    /* +0x13 */ bool        m_hasRoll;
    /* +0x14 */ bool        m_isUnique;
    /* +0x18 */ float       m_basePriority;         // 1.0f, clamped to <= 255.0 on set
    /* +0x1C */ bool        m_forceNetworkableId;
    /* +0x20 */ int         m_id;                   // not initialised by the constructor
    /* +0x24 */ int         m_predictionMode;
    /* +0x28 */ std::string m_name;
};

// Interface for registering and looking up networkable descriptors. Preserve virtual declaration order.
class INetworkableInfoManager : public IBase
{
public:
    virtual INetworkableInfo* getInfo(const std::string& name) const = 0;          // vptr +0x10
    virtual INetworkableInfo* getInfo(int id) const                  = 0;          // vptr +0x14
    virtual void deleteInfo(const std::string& name)                 = 0;          // vptr +0x18
    virtual void deleteInfo(int id)                                  = 0;          // vptr +0x1C
    virtual void deleteInfo(const INetworkableInfo* info)            = 0;          // vptr +0x20
    virtual void deleteAllInfos()                                    = 0;          // vptr +0x24
    virtual const INetworkableInfo* getActiveNetworkableInfo() const = 0;          // vptr +0x28
    virtual void setActiveNetworkableInfo(const INetworkableInfo* i) = 0;          // vptr +0x2C
    virtual INetworkableInfo* createNewInfo(const std::string& name) = 0;          // vptr +0x30
    virtual std::list<std::string>& getNetworkableInfoList()         = 0;          // vptr +0x34
    virtual const std::string& getWorkingFolderAndFile(const std::string& n) const = 0;  // +0x38
    virtual std::vector<INetworkableInfo*>& getAllUsedTemplates() const = 0;       // vptr +0x3C

    virtual ~INetworkableInfoManager() {}                                          // +0x40 / +0x44
};

// Owns replication policies and exposes the NetworkableInfoManager singleton.
class NetworkableInfoManager : public INetworkableInfoManager
{
public:
    NetworkableInfoManager();

    virtual void  addRef();                                   // +0x00
    virtual int   getRef();                                   // +0x04
    virtual int   release();                                  // +0x08
    virtual void* queryInterface(unsigned int iid) const;     // +0x0C

    virtual INetworkableInfo* getInfo(const std::string& name) const;   // +0x10
    virtual INetworkableInfo* getInfo(int id) const;                    // +0x14
    virtual void deleteInfo(const std::string& name);                   // +0x18
    virtual void deleteInfo(int id);                                    // +0x1C
    virtual void deleteInfo(const INetworkableInfo* info);              // +0x20
    virtual void deleteAllInfos();                                      // +0x24
    virtual const INetworkableInfo* getActiveNetworkableInfo() const;   // +0x28
    virtual void setActiveNetworkableInfo(const INetworkableInfo* i);   // +0x2C
    virtual INetworkableInfo* createNewInfo(const std::string& name);   // +0x30
    virtual std::list<std::string>& getNetworkableInfoList();           // +0x34
    virtual const std::string& getWorkingFolderAndFile(const std::string& name) const;
                                                                        // +0x38
    virtual std::vector<INetworkableInfo*>& getAllUsedTemplates() const; // +0x3C

    virtual ~NetworkableInfoManager();                        // +0x40
                                                              // Creates the requested manager interface with one reference.
    static IBase* create(unsigned int iid, IBase* outer);

private:
    /* +0x04 */ int                                   m_refCount;              // starts at 1
    /* +0x08 */ std::list<std::string>                m_networkableInfoList;
    /* +0x0C */ std::map<int, INetworkableInfo*>      m_infoById;
    /* +0x18 */ int                                   m_networkInfoIdCounter;  // starts at 0
    /* +0x1C */ Hash<std::string,
                     const INetworkableInfo*,
                     StringHashFunc>                  m_infoByName;            // 3011 buckets
    /* +0x28 */ const INetworkableInfo*               m_activeNetworkableInfo; // starts at 0
    /* +0x2C */ std::map<int, std::string>            m_workingFoldersAndFiles;
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_NETWORK_NETWORKABLEINFO_H
