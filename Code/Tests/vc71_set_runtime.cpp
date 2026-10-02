// [TEMP] [CHOSEN, not recovered] Focused x86 runtime fixture for the VC7.1 set-layout shim.
// Exercises only the evidence-scoped 4-byte-key tree topology and its Memory.dll-style allocation
// boundary. No renderer, LightManager, QueryManager, or shipped STL implementation is linked.

#include "../RendDX9Abi/Vc71Map.h"

#include <cstdio>
#include <cstdlib>

namespace {

int checks;
int failures;

#define CHECK(expression)                                                                  \
    do                                                                                     \
    {                                                                                      \
        ++checks;                                                                          \
        if (!(expression))                                                                 \
        {                                                                                  \
            ++failures;                                                                    \
            std::printf("FAIL line %d: %s\n", __LINE__, #expression);                    \
        }                                                                                  \
    } while (0)

void* livePointers[256];
unsigned livePointerCount;
unsigned allocationCalls;
unsigned freeCalls;
unsigned invalidFreeCalls;
bool failNextAllocation;

void rememberPointer(void* pointer)
{
    CHECK(pointer != 0);
    CHECK(livePointerCount < sizeof(livePointers) / sizeof(livePointers[0]));
    if (pointer && livePointerCount < sizeof(livePointers) / sizeof(livePointers[0]))
        livePointers[livePointerCount++] = pointer;
}

bool forgetPointer(void* pointer)
{
    for (unsigned i = 0; i < livePointerCount; ++i)
    {
        if (livePointers[i] == pointer)
        {
            livePointers[i] = livePointers[--livePointerCount];
            return true;
        }
    }
    return false;
}

void validate(const vc71::set_layout& set, const unsigned int* expected, unsigned count)
{
    CHECK(set.head != 0);
    if (!set.head)
        return;

    CHECK(set.head->isNil == 1);
    CHECK(set.head->color == 1);
    CHECK(set.size == count);
    CHECK(vc71::set_layout::next(set.head) == set.head);

    if (count == 0)
    {
        CHECK(set.empty());
        CHECK(set.begin() == set.end());
        CHECK(set.head->parent == set.head);
        CHECK(set.head->left == set.head);
        CHECK(set.head->right == set.head);
        CHECK(set.find(0xDEADBEEFu) == set.end());
        return;
    }

    CHECK(!set.empty());
    CHECK(set.head->parent != set.head);
    CHECK(set.head->parent->parent == set.head);
    CHECK(set.head->left != set.head && set.head->left->key == expected[0]);
    CHECK(set.head->right != set.head && set.head->right->key == expected[count - 1]);

    vc71::set_node* node = set.begin();
    unsigned walked = 0;
    while (node != set.end() && walked < count)
    {
        CHECK(node != 0);
        if (!node)
            break;
        CHECK(node->isNil == 0);
        CHECK(node->key == expected[walked]);
        CHECK(set.find(node->key) == node);
        CHECK(node->left != 0 && node->right != 0 && node->parent != 0);
        if (node->left != set.head)
            CHECK(node->left->parent == node);
        if (node->right != set.head)
            CHECK(node->right->parent == node);
        node = vc71::set_layout::next(node);
        ++walked;
    }
    CHECK(walked == count);
    CHECK(node == set.end());
    CHECK(set.find(0xDEADBEEFu) == set.end());
}

void insertKeys(vc71::set_layout& set, const unsigned int* keys, unsigned count)
{
    for (unsigned i = 0; i < count; ++i)
        CHECK(set.insert(keys[i]));
}

struct SavedNode
{
    unsigned int key;
    vc71::set_node* node;
};

void saveNodes(const vc71::set_layout& set, const unsigned int* keys, unsigned count,
               SavedNode* saved)
{
    for (unsigned i = 0; i < count; ++i)
    {
        saved[i].key = keys[i];
        saved[i].node = set.find(keys[i]);
        CHECK(saved[i].node != set.end());
    }
}

void checkSurvivingNodes(const vc71::set_layout& set, const SavedNode* saved, unsigned savedCount,
                         const unsigned int* survivingKeys, unsigned survivingCount)
{
    for (unsigned i = 0; i < survivingCount; ++i)
    {
        unsigned savedIndex = 0;
        while (savedIndex < savedCount && saved[savedIndex].key != survivingKeys[i])
            ++savedIndex;
        CHECK(savedIndex != savedCount);
        if (savedIndex != savedCount)
            CHECK(set.find(survivingKeys[i]) == saved[savedIndex].node);
    }
}

void testConstructionFailure()
{
    const unsigned liveBefore = livePointerCount;
    vc71::set_layout set = {};
    failNextAllocation = true;
    set.construct();
    CHECK(set.comp == 0);
    CHECK(set.head == 0);
    CHECK(set.size == 0);
    CHECK(set.empty());
    CHECK(!set.insert(1));
    set.clear();
    set.destroy();
    CHECK(livePointerCount == liveBefore);
}

void testSingleDuplicateAndDestroy()
{
    const unsigned liveBefore = livePointerCount;
    vc71::set_layout set = {};
    set.construct();
    vc71::set_node* const sentinel = set.head;
    const unsigned int none[] = { 0 };
    validate(set, none, 0);
    CHECK(livePointerCount == liveBefore + 1);

    CHECK(set.insert(10));
    vc71::set_node* const ten = set.find(10);
    const unsigned allocationsBeforeDuplicate = allocationCalls;
    CHECK(!set.insert(10));
    CHECK(allocationCalls == allocationsBeforeDuplicate);
    CHECK(set.find(10) == ten);
    const unsigned int one[] = { 10 };
    validate(set, one, 1);

    const unsigned freesBeforeIgnoredErase = freeCalls;
    set.erase(0);
    set.erase(set.end());
    CHECK(freeCalls == freesBeforeIgnoredErase);
    validate(set, one, 1);

    set.erase(ten);
    CHECK(set.head == sentinel);
    validate(set, none, 0);
    CHECK(livePointerCount == liveBefore + 1);

    set.destroy();
    CHECK(set.comp == 0 && set.head == 0 && set.size == 0);
    CHECK(livePointerCount == liveBefore);
    const unsigned freesAfterDestroy = freeCalls;
    set.destroy();
    CHECK(freeCalls == freesAfterDestroy);
}

void testEraseTopologiesAndClear()
{
    const unsigned liveBefore = livePointerCount;
    vc71::set_layout set = {};
    set.construct();
    vc71::set_node* const sentinel = set.head;

    const unsigned int insertionOrder[] = {
        8, 4, 12, 2, 6, 10, 14, 5, 7, 9, 11, 13, 15
    };
    insertKeys(set, insertionOrder, sizeof(insertionOrder) / sizeof(insertionOrder[0]));
    const unsigned int all[] = { 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    validate(set, all, sizeof(all) / sizeof(all[0]));

    SavedNode saved[sizeof(all) / sizeof(all[0])];
    saveNodes(set, all, sizeof(all) / sizeof(all[0]), saved);

    // Leaf.
    set.erase(set.find(5));
    const unsigned int withoutFive[] = { 2, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    validate(set, withoutFive, sizeof(withoutFive) / sizeof(withoutFive[0]));
    checkSurvivingNodes(set, saved, sizeof(saved) / sizeof(saved[0]), withoutFive,
                        sizeof(withoutFive) / sizeof(withoutFive[0]));

    // One child: 6 now has only its right child 7.
    set.erase(set.find(6));
    const unsigned int withoutSix[] = { 2, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    validate(set, withoutSix, sizeof(withoutSix) / sizeof(withoutSix[0]));
    checkSurvivingNodes(set, saved, sizeof(saved) / sizeof(saved[0]), withoutSix,
                        sizeof(withoutSix) / sizeof(withoutSix[0]));

    // Two children, non-root, with a successor below the direct right child (13 under 14).
    vc71::set_node* const thirteen = set.find(13);
    set.erase(set.find(12));
    const unsigned int withoutTwelve[] = { 2, 4, 7, 8, 9, 10, 11, 13, 14, 15 };
    validate(set, withoutTwelve, sizeof(withoutTwelve) / sizeof(withoutTwelve[0]));
    checkSurvivingNodes(set, saved, sizeof(saved) / sizeof(saved[0]), withoutTwelve,
                        sizeof(withoutTwelve) / sizeof(withoutTwelve[0]));
    CHECK(set.find(13) == thirteen);
    CHECK(thirteen->parent == set.find(8));

    // Root with two children. Its deeper successor node (9) must become root without moving.
    vc71::set_node* const nine = set.find(9);
    set.erase(set.find(8));
    const unsigned int withoutRoot[] = { 2, 4, 7, 9, 10, 11, 13, 14, 15 };
    validate(set, withoutRoot, sizeof(withoutRoot) / sizeof(withoutRoot[0]));
    checkSurvivingNodes(set, saved, sizeof(saved) / sizeof(saved[0]), withoutRoot,
                        sizeof(withoutRoot) / sizeof(withoutRoot[0]));
    CHECK(set.head->parent == nine);
    CHECK(set.find(9) == nine);

    const unsigned liveBeforeClear = livePointerCount;
    set.clear();
    CHECK(set.head == sentinel);
    validate(set, all, 0);
    CHECK(livePointerCount + (sizeof(withoutRoot) / sizeof(withoutRoot[0])) == liveBeforeClear);

    // clear() retains a valid sentinel and the set remains reusable.
    const unsigned int reusedInsertion[] = { 20, 10 };
    insertKeys(set, reusedInsertion, sizeof(reusedInsertion) / sizeof(reusedInsertion[0]));
    const unsigned int reused[] = { 10, 20 };
    validate(set, reused, sizeof(reused) / sizeof(reused[0]));

    // Root with one child, followed by the last/root node.
    vc71::set_node* const ten = set.find(10);
    set.erase(set.find(20));
    const unsigned int justTen[] = { 10 };
    validate(set, justTen, 1);
    CHECK(set.head->parent == ten && ten->parent == set.head);
    set.erase(ten);
    validate(set, all, 0);

    // Direct-right-child successor case.
    const unsigned int directInsertion[] = { 4, 2, 6 };
    insertKeys(set, directInsertion, sizeof(directInsertion) / sizeof(directInsertion[0]));
    vc71::set_node* const six = set.find(6);
    set.erase(set.find(4));
    const unsigned int directRemaining[] = { 2, 6 };
    validate(set, directRemaining, sizeof(directRemaining) / sizeof(directRemaining[0]));
    CHECK(set.head->parent == six && set.find(6) == six);

    set.destroy();
    CHECK(livePointerCount == liveBefore);
}

} // anonymous namespace

namespace renddx9abi {

void* engineAlloc(unsigned int size)
{
    if (failNextAllocation)
    {
        failNextAllocation = false;
        return 0;
    }
    void* const pointer = std::malloc(size);
    if (pointer)
    {
        ++allocationCalls;
        rememberPointer(pointer);
    }
    return pointer;
}

void engineFree(void* pointer)
{
    if (!pointer || !forgetPointer(pointer))
    {
        ++invalidFreeCalls;
        return;
    }
    ++freeCalls;
    std::free(pointer);
}

} // namespace renddx9abi

int main()
{
    static_assert(sizeof(void*) == 4, "VC7.1 set-layout fixture is x86-only");
    static_assert(sizeof(vc71::set_node) == 0x14, "the set node is 20 bytes");
    static_assert(sizeof(vc71::set_layout) == 12, "the set header is 12 bytes");

    testConstructionFailure();
    testSingleDuplicateAndDestroy();
    testEraseTopologiesAndClear();

    CHECK(invalidFreeCalls == 0);
    CHECK(livePointerCount == 0);
    CHECK(allocationCalls == freeCalls);
    std::printf("VC7.1 set runtime: %d/%d checks passed; %u allocations balanced (%s)\n",
                checks - failures, checks, allocationCalls,
#if BF2_PLUGIN_ABI_NATIVE
                "RendDX9 native"
#else
                "RendDX9 compatibility"
#endif
    );
    return failures ? 1 : 0;
}
