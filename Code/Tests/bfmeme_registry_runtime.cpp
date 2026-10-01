// [TEMP] Native x86 fixture. Test subclasses model only observable hook outcomes.
#include "../BF2/External/BFMeme/Object.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#if defined(TEST_REGISTRY)
#include "../BF2/External/BFMeme/NamedObjectRegistry.h"
#endif
#include <cstdio>
#include <cstring>
using namespace dice::meme;
static int checks = 0, failures = 0;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)
class FixtureObject : public Object
{
public:
    FixtureObject() : name("fixture"), permitCreate(true), permitDestroy(true) {}
    ~FixtureObject() override { ++destroyed; }
    const char* getName() override { return name; }
    bool setName(const char* value) override { name = value; return false; }
    bool createRuntime() override { ++created; return permitCreate; }
    bool destroyRuntime() override { ++destroyHooks; return permitDestroy; }
    TypeDescriptor* getType() override { return getClassType(); }
    static TypeDescriptor* getClassType()
    { static ClassTypeDescriptor<FixtureObject, Object> type("fixture::FixtureObject"); return &type; }
    const char* name;
    bool permitCreate, permitDestroy;
    static int created, destroyed, destroyHooks;
};
int FixtureObject::created = 0;
int FixtureObject::destroyed = 0;
int FixtureObject::destroyHooks = 0;
int main()
{
    CHECK(sizeof(RuntimeObject) == 4);
    CHECK(sizeof(Object) == 4);
    CHECK(sizeof(TypeDescriptor) == 16);
    Object object;
    CHECK(!object.getName());
    CHECK(!object.setName("ignored"));
    CHECK(!object.slot_0x20() && !object.slot_0x24());
    CHECK(object.onCreate(nullptr));
    CHECK(object.visit0x2c(nullptr, nullptr));
    CHECK(object.visit0x34(nullptr, nullptr));
    CHECK(object.visit0x38(nullptr, nullptr));
    CHECK(object.visit0x3c(nullptr, nullptr));
    CHECK(object.getType() == Object::getClassType());
    CHECK(object.getType()->getBaseType() == RuntimeObject::getClassType());
    CHECK(object.getType()->isDerivedFrom(RuntimeObject::getClassType()));
    CHECK(object.getType()->isDerivedFrom(nullptr));
    CHECK(!RuntimeObject::getClassType()->isDerivedFrom(object.getType()));
    static TypeDescriptor qualified("fixture::Duplicate");
    static TypeDescriptor full("Duplicate");
    CHECK(!std::strcmp(qualified.getShortName(), "Duplicate"));
    CHECK(TypeDescriptor::find("Duplicate") == &full); // full names precede short names
    CHECK(TypeDescriptor::find("fixture::Duplicate") == &qualified);
    CHECK(!TypeDescriptor::find("not registered"));
    CHECK(!qualified.getDescription() && !qualified.create());
    CHECK(qualified.getInstanceCount() == 0 && qualified.getInstanceSize() == 0);
    RuntimeObject* runtime = nullptr;
    CHECK(destroyRuntimeObject(runtime));
    CHECK(!createRuntimeObject(runtime, nullptr));
    TypeDescriptor* type = FixtureObject::getClassType();
    CHECK(createRuntimeObject(runtime, type));
    CHECK(runtime && type->getInstanceCount() == 1 && FixtureObject::created == 1);
    CHECK(createRuntimeObject(runtime, type));
    CHECK(type->getInstanceCount() == 1 && FixtureObject::created == 2 && FixtureObject::destroyed == 1);
    CHECK(destroyRuntimeObject(runtime));
    CHECK(!runtime && type->getInstanceCount() == 0 && FixtureObject::destroyed == 2);
    CHECK(createRuntimeObject(runtime, type));
    FixtureObject* rejected = static_cast<FixtureObject*>(runtime);
    rejected->permitDestroy = false;
    CHECK(!destroyRuntimeObject(runtime));
    CHECK(!runtime && type->getInstanceCount() == 1 && FixtureObject::destroyed == 2);
    // Retail rejection clears the reference without decrementing or deleting.
    type->removeInstance(); delete rejected;
#if defined(TEST_REGISTRY)
    CHECK(sizeof(NameTreeNode) == 8 && sizeof(NameTreeRoot) == 16);
    NameTreeNode base;
    char path[256] = "unchanged";
    CHECK(base.objectPath(nullptr, path, 0) && !std::strcmp(path, "unchanged"));
    CHECK(!base.insert(nullptr, &object, nullptr));
    FixtureObject z, a, duplicate;
    z.name = "Zulu"; a.name = "Alpha"; duplicate.name = "Alpha";
    {
        NameTreeRoot root;
        CHECK(!root.insert(nullptr, nullptr, nullptr));
        CHECK(!root.insert(nullptr, &object, nullptr));
        CHECK(root.insert(nullptr, &z, "MiddleLeft/Sub"));
        CHECK(root.insert(nullptr, &a, "MiddleLeft\\Sub"));
        CHECK(root.insert(nullptr, &duplicate, "MiddleLeft/Sub"));
        CHECK(root.findObject(nullptr, "MiddleLeft/Sub/Alpha") == &a);
        CHECK(root.findObject(nullptr, "MiddleLeft\\Sub\\Zulu") == &z);
        CHECK(!root.findObject(nullptr, "MiddleLeft/Sub/Missing"));
        NameTreeNode* parent = root.findParent(&a);
        CHECK(parent && !std::strcmp(parent->getName(), "Sub"));
        CHECK(parent->getChildren()->getObject() == &a);
        CHECK(parent->getChildren()->next->getObject() == &duplicate);
        CHECK(parent->getChildren()->next->next->getObject() == &z);
        CHECK(root.contains(&z) && !root.contains(&object));
        CHECK(root.objectPath(&z, path, sizeof(path)) && !std::strcmp(path, "MiddleLeft/Sub/Zulu"));
        CHECK(root.nodePath(parent, path, sizeof(path)) && !std::strcmp(path, "MiddleLeft/Sub"));
        std::strcpy(path, "sentinel");
        CHECK(!root.objectPath(&z, path, 0) && !std::strcmp(path, "sentinel"));
        CHECK(!root.nodePath(parent, path, 0) && path[0] == 0);
        CHECK(!parent->createContainer(nullptr, "Alpha")); // name collision with entry
        CHECK(!root.createContainer(nullptr, "Trailing/")); // leaves intermediate container
        CHECK(root.findChild("Trailing") != nullptr);
        CHECK(root.eraseObject(nullptr, &a));
        CHECK(root.findObject(nullptr, "MiddleLeft/Sub/Alpha") == &duplicate);
        CHECK(!root.contains(&a));
        CHECK(root.eraseNode(nullptr, root.findEntry(nullptr, &duplicate)));
        CHECK(!root.findEntry(nullptr, &duplicate));
        CHECK(!root.eraseObject(nullptr, &object));
        CHECK(!root.objectPath(&object, path, sizeof(path)) && !path[0]);
    } // entries are deleted; the stack Objects are not
    CHECK(FixtureObject::destroyed == 3);
    NameTreeObjectEntry entry(&a);
    CHECK(entry.setName("renamed") && !std::strcmp(a.name, "renamed")); // ignores false hook
    CHECK(!entry.setName(nullptr));
    runtime = nullptr;
    CHECK(createRuntimeObject(runtime, type));
    NameTreeObjectEntry owned(static_cast<Object*>(runtime));
    CHECK(owned.close(nullptr));
    CHECK(!owned.getObject() && type->getInstanceCount() == 0);
    CHECK(owned.contains(nullptr));
#endif
    std::printf("BFMeme %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
