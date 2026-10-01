// Per-type replication policies and their owning registry. [INFERRED] Source placement.

#include "NetworkableInfo.h"

#include <iostream>

#include "../Console/Console.h"     // io::mainConsole / OldConsole::getWorkingFile - createNewInfo
#include "../../Core/Ids.h"         // IID_IBase, IID_INetworkableInfo, IID_INetworkableInfoManager

namespace dice {
namespace hfe {
namespace io {

// NetworkableInfo

// Initializes replication settings. The manager assigns the id and usage flag after construction.
NetworkableInfo::NetworkableInfo()
{
    m_basePriority   = 1.0f;    // +0x18, stored as 0x3F800000
    m_refCount       = 1;       // +0x08 - starts at one, not zero
    m_isControlledBy = -1;      // +0x0C, `mov dword ptr [eax+0Ch], 0FFFFFFFFh` - the "nobody" sentinel

    // Initialize independent replication-feature flags.
    m_hasOrientation     = false;   // +0x10
    m_hasYaw             = false;   // +0x11
    m_hasPitch           = false;   // +0x12
    m_hasRoll            = false;   // +0x13
    m_isUnique           = false;   // +0x14
    m_forceNetworkableId = false;   // +0x1C

    m_predictionMode = 0;       // +0x24
    // +0x28 m_name is default-constructed - in the disassembly this is the `lock add` on the shared
    // empty-string refcount followed by the store of the empty-rep data pointer.
}

// Releases owned policy-name storage.
NetworkableInfo::~NetworkableInfo()
{
}

// Non-atomic reference counting. release deletes at zero and returns the remaining count.
void NetworkableInfo::addRef()
{
    ++m_refCount;               // +0x08
}

int NetworkableInfo::getRef()
{
    return m_refCount;
}

int NetworkableInfo::release()
{
    // ELF 8466780 preserves EBX; PE 6896E0 preserves ESI across deletion.
    // Return the saved count, never a member read after the object is freed.
    const int remaining = --m_refCount;
    if (remaining == 0 && this != 0)   // the `this != 0` test is dice's, and it is in the binary
        delete this;                    // 0x84667A0, `call dword ptr [eax+44h]`
    return remaining;
}

// Answers exactly two IIDs, IBase first.
void* NetworkableInfo::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<NetworkableInfo*>(this);
    if (iid == IID_INetworkableInfo)
        return const_cast<NetworkableInfo*>(this);
    return 0;
}

// `add eax,28h` then a tail jump into std::string::assign.
void NetworkableInfo::setName(const std::string& name)
{
    m_name = name;              // +0x28
}

// `mov eax,[ebp+this] / add eax, 28h / retn` - it returns the address of the
// member, i.e. a reference to it, without touching the string itself.
const std::string& NetworkableInfo::getName() const
{
    return m_name;
}

// 0x8466810 / 0x8466820, 11 and 14 bytes.
int NetworkableInfo::getID() const
{
    return m_id;                // +0x20
}

void NetworkableInfo::setID(int id)
{
    m_id = id;
}

// 0x8466850 / 0x8466860, 14 and 11 bytes.
void NetworkableInfo::setPredictionMode(PredictionMode mode)
{
    m_predictionMode = static_cast<int>(mode);      // +0x24
}

PredictionMode NetworkableInfo::getPredictionMode() const
{
    return static_cast<PredictionMode>(m_predictionMode);
}

// Sets base priority with an upper limit of 255. Negative values are retained.
void NetworkableInfo::setBasePriority(float priority)
{
    m_basePriority = (priority > 255.0f) ? 255.0f : priority;
}

// `fld dword ptr [eax+18h] / retn`.
float NetworkableInfo::getBasePriority() const
{
    return m_basePriority;
}

// 0x8466920 / 0x8466930, 14 and 12 bytes - byte store, movzx load.
void NetworkableInfo::setIsUnique(bool unique)
{
    m_isUnique = unique;        // +0x14
}

bool NetworkableInfo::getIsUnique() const
{
    return m_isUnique;
}

// 0x8466940 / 0x8466950, 14 and 12 bytes. Resolves what the earlier pass recorded as the
// "[UNRESOLVED] bool at +0x1C".
void NetworkableInfo::setForceNetworkableId(bool force)
{
    m_forceNetworkableId = force;   // +0x1C
}

bool NetworkableInfo::getForceNetworkableId() const
{
    return m_forceNetworkableId;
}

// 0x8466830 / 0x8466840, 11 and 14 bytes. dword accesses, not byte - so +0x0C is an int, which is
// consistent with the constructor seeding it to -1 rather than to 0.
int NetworkableInfo::getIsControlledBy() const
{
    return m_isControlledBy;    // +0x0C
}

void NetworkableInfo::setIsControlledBy(int who)
{
    m_isControlledBy = who;
}

// 0x84668A0 / 0x84668B0, 12 and 14 bytes.
bool NetworkableInfo::getHasOrientation() const
{
    return m_hasOrientation;    // +0x10
}

void NetworkableInfo::setHasOrientation(bool has)
{
    m_hasOrientation = has;
}

// the three getters, 12 bytes each, reading +0x11, +0x12, +0x13.
bool NetworkableInfo::getHasYaw() const
{
    return m_hasYaw;            // +0x11
}

bool NetworkableInfo::getHasPitch() const
{
    return m_hasPitch;          // +0x12
}

bool NetworkableInfo::getHasRoll() const
{
    return m_hasRoll;           // +0x13
}

// Preserve virtual order: all three orientation getters precede their setters.
void NetworkableInfo::setHasYaw(bool has)
{
    m_hasYaw = has;
}

void NetworkableInfo::setHasPitch(bool has)
{
    m_hasPitch = has;
}

void NetworkableInfo::setHasRoll(bool has)
{
    m_hasRoll = has;
}

// NetworkableInfoManager

// Constructs empty network-info indices with one reference, ID counter zero and no active entry.
// After member construction, explicitly clears only the ID and name indices; preserve that sequence.
NetworkableInfoManager::NetworkableInfoManager()
{
    m_refCount             = 1;     // +0x04 - born with one reference, so create() must not addRef
    m_networkInfoIdCounter = 0;     // +0x18
    m_activeNetworkableInfo = 0;    // +0x28

    // The two no-op clears described above. Present in the ELF; kept.
    m_infoById.clear();
    m_infoByName.clear();
}

// Destroys registry containers without releasing the NetworkableInfo objects they reference.
NetworkableInfoManager::~NetworkableInfoManager()
{
}

// Reference-counted manager lifetime.
void NetworkableInfoManager::addRef()
{
    ++m_refCount;
}

int NetworkableInfoManager::getRef()
{
    return m_refCount;
}

int NetworkableInfoManager::release()
{
    // ELF 8468120 / PE 683290 likewise return a register saved before deletion.
    const int remaining = --m_refCount;
    if (remaining == 0 && this != 0)   // again the redundant `this` test, again in the binary
        delete this;                    // 0x8468140, `call dword ptr [eax+44h]`
    return remaining;
}

// IBase first, then the manager's own IID - the same two-test shape as every
// other singleton's.
void* NetworkableInfoManager::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<NetworkableInfoManager*>(this);
    if (iid == IID_INetworkableInfoManager)
        return const_cast<NetworkableInfoManager*>(this);
    return 0;
}

// Returns the named info and marks it used. Names are case-sensitive; empty or unknown names return null.
INetworkableInfo* NetworkableInfoManager::getInfo(const std::string& name) const
{
    if (name.length() == 0)
        return 0;

    const INetworkableInfo* info = 0;
    m_infoByName.get(name, info);

    INetworkableInfo* result = const_cast<INetworkableInfo*>(info);
    if (result != 0)
        result->m_isUsed = true;

    return result;
}

// Returns the policy with id and marks it used, or null when absent.
INetworkableInfo* NetworkableInfoManager::getInfo(int id) const
{
    std::map<int, INetworkableInfo*>::const_iterator i = m_infoById.find(id);
    if (i == m_infoById.end())
        return 0;

    INetworkableInfo* info = i->second;
    if (info != 0)
        info->m_isUsed = true;

    return info;
}

// Looks up and unregisters an info by name or id. Missing entries are ignored.
void NetworkableInfoManager::deleteInfo(const std::string& name)
{
    deleteInfo(getInfo(name));
}

void NetworkableInfoManager::deleteInfo(int id)
{
    deleteInfo(getInfo(id));
}

// Unregisters info from all lookup containers without deleting or releasing it. Null is ignored.
void NetworkableInfoManager::deleteInfo(const INetworkableInfo* info)
{
    if (info == 0)
        return;

    const std::string& name = info->getName();
    m_infoByName.remove(name);

    m_infoById.erase(info->getID());
    m_workingFoldersAndFiles.erase(info->getID());
}

// Frees all registered infos and clears lookup containers, retaining the cached name list.
// [CHOSEN] Direct storage deletion preserves the original missing-destructor behavior and its string leak.
void NetworkableInfoManager::deleteAllInfos()
{
    for (std::map<int, INetworkableInfo*>::iterator i = m_infoById.begin(); i != m_infoById.end(); ++i)
    {
        // [CHOSEN, not recovered] spelling. What is recovered is the emitted call: a bare
        // operator delete with no destructor - see the block above.
        ::operator delete(i->second);
    }

    m_infoById.clear();
    m_infoByName.clear();
    m_workingFoldersAndFiles.clear();
}

// Active info selected by creation and used by console commands.
const INetworkableInfo* NetworkableInfoManager::getActiveNetworkableInfo() const
{
    return m_activeNetworkableInfo;
}

void NetworkableInfoManager::setActiveNetworkableInfo(const INetworkableInfo* info)
{
    m_activeNetworkableInfo = info;
}

// Creates, registers and activates a named info, recording its defining script when available.
// A duplicate name returns null and clears the active info.
INetworkableInfo* NetworkableInfoManager::createNewInfo(const std::string& name)
{
    // `calcStringHashValueNoCase(name) % m_maxSize`, computed once at the call site and reused for
    // both probes and the insert - the two-step form Core/Hash.h documents. Spelled through
    // bucketOf() rather than with a literal 3011 so the table size stays in one place.
    const unsigned int bucket = m_infoByName.bucketOf(name);

    if (m_infoByName.exists(name, bucket))              // step 1, inlined chain walk
    {
        setActiveNetworkableInfo(0);
        return 0;
    }

    NetworkableInfo* info = new NetworkableInfo();      // step 2, operator new(0x2Cu) at 0x84674EA
    if (info == 0)                                      // dice's null test; kept
    {
        setActiveNetworkableInfo(0);
        return 0;
    }

    info->setName(name);                                // step 3

    if (!m_infoByName.exists(name, bucket))             // step 4 - the second, redundant probe
        m_infoByName.insert(name, info, bucket);

    info->setID(m_networkInfoIdCounter++);              // step 5
    info->m_isUsed = false;                             // step 6, 0x846758F
    m_infoById.insert(std::make_pair(info->getID(), static_cast<INetworkableInfo*>(info)));  // step 7
    setActiveNetworkableInfo(info);                     // step 8

    // Step 9. mainConsole is the .con executor; getWorkingFile is its vptr +0x90.
    const std::string& workingFile = mainConsole->getWorkingFile();
    if (workingFile.length() != 0)
        m_workingFoldersAndFiles.insert(std::make_pair(info->getID(), workingFile));

    return info;
}

// Rebuilds and returns the manager-owned name list, beginning with an empty entry.
// A subsequent call replaces its contents.
std::list<std::string>& NetworkableInfoManager::getNetworkableInfoList()
{
    m_networkableInfoList.clear();
    m_networkableInfoList.push_back("");

    for (std::map<int, INetworkableInfo*>::iterator i = m_infoById.begin(); i != m_infoById.end(); ++i)
        m_networkableInfoList.push_back(i->second->getName());

    return m_networkableInfoList;
}

// Returns the defining script path for name, or a shared empty string when unavailable.
const std::string& NetworkableInfoManager::getWorkingFolderAndFile(const std::string& name) const
{
    static const std::string emptyString;

    const INetworkableInfo* info = getInfo(name);
    if (info == 0)
        return emptyString;

    std::map<int, std::string>::const_iterator i = m_workingFoldersAndFiles.find(info->getID());
    if (i == m_workingFoldersAndFiles.end())
        return emptyString;

    return i->second;
}

// Returns used infos in shared static storage rebuilt on each call. Callers share and overwrite that result buffer.
std::vector<INetworkableInfo*>& NetworkableInfoManager::getAllUsedTemplates() const
{
    static std::vector<INetworkableInfo*> mUsedTemplates;   // attested name; see above

    mUsedTemplates.clear();

    for (std::map<int, INetworkableInfo*>::const_iterator i = m_infoById.begin();
         i != m_infoById.end(); ++i)
    {
        INetworkableInfo* info = i->second;
        if (info->m_isUsed)
            mUsedTemplates.push_back(info);
    }

    return mUsedTemplates;
}

// Creates the requested manager interface with one owned reference; rejects aggregation.
IBase* NetworkableInfoManager::create(unsigned int iid, IBase* outer)
{
    if (outer != 0)
        return 0;

    NetworkableInfoManager* self = new NetworkableInfoManager();    // 0x84FB5B0, operator new(0x38u)
    void* itf = self->queryInterface(iid);              // 0x84FB5D2, virtual
    if (itf == 0)
        self->release();                                // 0x84FB5E4, virtual
    return static_cast<IBase*>(itf);
}

} // namespace io
} // namespace hfe
} // namespace dice
