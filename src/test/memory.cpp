#include "tests.hpp"
#include "hg/memory.hpp"
#include "hg/error.hpp"

using namespace hg;

// heapAlloc

TEST(testHeapAllocReturnsNonNull)
{
    void* p = heapAlloc(64, 1);
    ASSERT(p != nullptr);
    heapFree(p, 64);
}

TEST(testHeapAllocZeroSize)
{
    void* p = heapAlloc(0, 1);
    ASSERT(p != nullptr);
    heapFree(p, 0);
}

TEST(testHeapAllocTyped)
{
    u32* arr = heapAlloc<u32>(4);
    ASSERT(arr != nullptr);
    arr[0] = 10;
    arr[3] = 40;
    ASSERT(arr[0] == 10);
    ASSERT(arr[3] == 40);
    heapFree<u32>(arr, 4);
}

// Arena

TEST(testArenaDefault)
{
    Arena a{};
    ASSERT(a.memory == nullptr);
    ASSERT(a.capacity == 0);
    ASSERT(a.head == 0);
}

TEST(testArenaCapacity)
{
    Arena a{256};
    ASSERT(a.memory != nullptr);
    ASSERT(a.capacity == 256);
    ASSERT(a.head == 0);
}

TEST(testArenaAlloc)
{
    Arena a{256};
    u32* p = static_cast<u32*>(a.alloc(4, 4));
    ASSERT(p != nullptr);
    ASSERT(a.head == 4);
    *p = 42;
    ASSERT(*p == 42);
}

TEST(testArenaAlignment)
{
    Arena a{256};
    a.alloc(1, 1);
    void* p = a.alloc(4, 4);
    ASSERT(p != nullptr);
    ASSERT(a.head == 8);
    ASSERT((reinterpret_cast<uptr>(p) & 3) == 0);
}

TEST(testArenaExactCapacity)
{
    Arena a{16};
    void* p = a.alloc(16, 1);
    ASSERT(p != nullptr);
    ASSERT(a.head == 16);
}

TEST(testArenaOutOfMemory)
{
    setError("");
    Arena a{16};
    a.alloc(16, 1);
    void* p = a.alloc(1, 1);
    ASSERT(p == nullptr);
    ASSERT(getError().length > 0);
    setError("");
}

TEST(testArenaTypedAlloc)
{
    Arena a{256};
    u32* p = a.alloc<u32>(3);
    ASSERT(p != nullptr);
    p[0] = 10;
    p[1] = 20;
    p[2] = 30;
    ASSERT(p[0] == 10);
    ASSERT(p[2] == 30);
}

TEST(testArenaExtendSuccess)
{
    Arena a{256};
    u32* p = a.alloc<u32>(2);
    ASSERT(p != nullptr);
    ASSERT(a.head == 8);
    bool ok = a.extend(p, 2, 4);
    ASSERT(ok);
    ASSERT(a.head == 16);
}

TEST(testArenaExtendNonLast)
{
    Arena a{256};
    u32* first = a.alloc<u32>(2);
    a.alloc<u32>(2);
    bool ok = a.extend(first, 2, 4);
    ASSERT(!ok);
    ASSERT(a.head == 16);
}

TEST(testArenaExtendBeyondCapacity)
{
    Arena a{16};
    u8* p = static_cast<u8*>(a.alloc(4, 1));
    bool ok = a.extend(p, 4, 20);
    ASSERT(!ok);
}

TEST(testArenaMoveConstruct)
{
    Arena a{256};
    ASSERT(a.memory != nullptr);
    Arena b{std::move(a)};
    ASSERT(a.memory == nullptr);
    ASSERT(a.capacity == 0);
    ASSERT(a.head == 0);
    ASSERT(b.memory != nullptr);
    ASSERT(b.capacity == 256);
}

TEST(testArenaMoveAssign)
{
    Arena old{64};
    Arena a{128};
    old = std::move(a);
    ASSERT(a.memory == nullptr);
    ASSERT(old.memory != nullptr);
    ASSERT(old.capacity == 128);
}

// ArenaScope

TEST(testArenaScopeDefault)
{
    ArenaScope scope{};
    ASSERT(scope.arena == nullptr);
    ASSERT(scope.head == 0);
}

TEST(testArenaScopeRAII)
{
    Arena a{256};
    a.alloc(16, 1);
    {
        ArenaScope scope{&a};
        ASSERT(scope.arena == &a);
        a.alloc(32, 1);
        ASSERT(a.head == 48);
    }
    ASSERT(a.head == 16);
}

TEST(testArenaScopeNested)
{
    Arena a{256};
    a.alloc(8, 1);
    {
        ArenaScope outer{&a};
        a.alloc(8, 1);
        {
            ArenaScope inner{&a};
            a.alloc(8, 1);
            ASSERT(a.head == 24);
        }
        ASSERT(a.head == 16);
    }
    ASSERT(a.head == 8);
}

TEST(testArenaScopeNull)
{
    ArenaScope scope{};
}

TEST(testArenaScopeConversion)
{
    ArenaScope scope;
    Arena* ap = scope;
    ASSERT(ap == scope.arena);
}

TEST(testArenaScopeAlloc)
{
    Arena a{256};
    ArenaScope scope{&a};
    u32* p = static_cast<u32*>(scope.alloc(4, 4));
    ASSERT(p != nullptr);
    ASSERT(a.head == 4);
}

TEST(testArenaScopeTypedAlloc)
{
    Arena a{256};
    ArenaScope scope{&a};
    u32* p = scope.alloc<u32>(3);
    ASSERT(p != nullptr);
    p[0] = 10;
    ASSERT(p[0] == 10);
}

TEST(testArenaScopeExtend)
{
    Arena a{256};
    ArenaScope scope{&a};
    u32* p = scope.alloc<u32>(2);
    ASSERT(p != nullptr);
    bool ok = scope.extend(p, 2, 4);
    ASSERT(ok);
    ASSERT(a.head == 16);
}

TEST(testArenaScopeMoveConstruct)
{
    Arena a{256};
    ArenaScope scope{&a};
    ArenaScope other{std::move(scope)};
    ASSERT(scope.arena == nullptr);
    ASSERT(other.arena == &a);
}

TEST(testArenaScopeMoveAssign)
{
    Arena a{256};
    Arena b{256};
    ArenaScope x{&a};
    ArenaScope y{&b};
    x = std::move(y);
    ASSERT(y.arena == nullptr);
    ASSERT(x.arena == &b);
}

// getScratch

TEST(testGetScratchReturnsValidArena)
{
    ArenaScope scope = getScratch();
    Arena* ap = scope;
    ASSERT(ap != nullptr);
    ASSERT(ap->memory != nullptr);
    ASSERT(ap->capacity > 0);
    void* p = ap->alloc(64, 4);
    ASSERT(p != nullptr);
}

TEST(testGetScratchSameArena)
{
    ArenaScope a = getScratch();
    ArenaScope b = getScratch();
    ASSERT(a.arena == b.arena);
}

TEST(testGetScratchConflict)
{
    ArenaScope a = getScratch();
    u32* p = a.alloc<u32>(1);
    ASSERT(p != nullptr);
    Arena const* conflicts[] = {a.arena};
    ArenaScope b = getScratch(conflicts, 1);
    ASSERT(b.arena != a.arena);
    u32* q = b.alloc<u32>(1);
    ASSERT(q != nullptr);
    *q = 42;
    ASSERT(*q == 42);
}
