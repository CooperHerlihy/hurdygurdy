#include "tests.hpp"
#include "hg/memory.hpp"
#include "hg/error.hpp"

#include <cstring>

using namespace hg;

// Arena

TEST(testArenaAlloc)
{
    Arena a{256};
    u32* p = static_cast<u32*>(a.alloc(4, 4));
    ASSERT(p != nullptr);
    *p = 42;
    ASSERT(*p == 42);
}

TEST(testArenaAlignment)
{
    Arena a{256};
    a.alloc(1, 1);
    void* p = a.alloc(4, 4);
    ASSERT(p != nullptr);
    ASSERT((reinterpret_cast<uptr>(p) & 3) == 0);
}

TEST(testArenaLargeAlignment)
{
    Arena a{256};
    void* p = a.alloc(1, 16);
    ASSERT(p != nullptr);
    ASSERT((reinterpret_cast<uptr>(p) & 15) == 0);
}

TEST(testArenaExactCapacity)
{
    Arena a{16};
    void* p = a.alloc(16, 1);
    ASSERT(p != nullptr);
}

TEST(testArenaOutOfMemory)
{
    setError("");
    Arena a{16};
    a.alloc(16, 1);
    void* p = a.alloc(1, 1);
    ASSERT(p == nullptr);
    ASSERT(getError().length() > 0);
    setError("");
}

TEST(testArenaAllocAfterOutOfMemory)
{
    setError("");
    Arena a{16};
    a.alloc(16, 1);
    ASSERT(a.alloc(1, 1) == nullptr);
    setError("");
    void* p = a.alloc(1, 1);
    ASSERT(p == nullptr);
    setError("");
}

TEST(testArenaAllocOversized)
{
    setError("");
    Arena a{16};
    void* p = a.alloc(32, 1);
    ASSERT(p == nullptr);
    ASSERT(getError().length() > 0);
    setError("");
}

TEST(testArenaAllocArray)
{
    Arena a{256};
    u32* p = static_cast<u32*>(a.alloc(3 * sizeof(u32), alignof(u32)));
    ASSERT(p != nullptr);
    p[0] = 10;
    p[1] = 20;
    p[2] = 30;
    ASSERT(p[0] == 10);
    ASSERT(p[2] == 30);
}

TEST(testArenaDistinctAllocations)
{
    Arena a{256};
    void* p1 = a.alloc(16, 1);
    void* p2 = a.alloc(16, 1);
    ASSERT(p1 != nullptr);
    ASSERT(p2 != nullptr);
    ASSERT(p1 != p2);
    std::memset(p1, 0xAB, 16);
    std::memset(p2, 0xCD, 16);
    ASSERT(static_cast<u8*>(p1)[15] == 0xAB);
    ASSERT(static_cast<u8*>(p2)[15] == 0xCD);
}

TEST(testArenaEmptyArena)
{
    Arena a{};
    void* p = a.alloc(4, 4);
    ASSERT(p == nullptr);
}

TEST(testArenaMoveConstruct)
{
    Arena a{256};
    Arena b{std::move(a)};
    void* pa = a.alloc(4, 4);
    ASSERT(pa == nullptr);
    void* pb = b.alloc(4, 4);
    ASSERT(pb != nullptr);
}

TEST(testArenaMoveConstructCapacityFollows)
{
    Arena a{128};
    void* p1 = a.alloc(64, 1);
    ASSERT(p1 != nullptr);
    Arena b{std::move(a)};
    void* p2 = b.alloc(64, 1);
    ASSERT(p2 != nullptr);
    ASSERT(b.alloc(64, 1) == nullptr);
}

TEST(testArenaMoveAssign)
{
    Arena old{64};
    Arena a{256};
    old = std::move(a);
    void* pa = a.alloc(4, 4);
    ASSERT(pa == nullptr);
    void* pold = old.alloc(4, 4);
    ASSERT(pold != nullptr);
}

TEST(testArenaMoveAssignSelfSafe)
{
    Arena a{64};
    void* p = a.alloc(16, 1);
    ASSERT(p != nullptr);
    Arena& ref = a;
    a = std::move(ref);
    ASSERT(a.alloc(16, 1) != nullptr);
}

// ArenaScope

TEST(testArenaScopeUnused)
{
    Arena a{256};
    {
        ArenaScope scope{&a};
    }
    void* p = a.alloc(256, 1);
    ASSERT(p != nullptr);
    void* n = a.alloc(1, 1);
    ASSERT(n == nullptr);
}

TEST(testArenaScopeRAII)
{
    Arena a{256};
    void* p1 = a.alloc(128, 1);
    void* p2 = nullptr;
    {
        ArenaScope scope{&a};
        p2 = a.alloc(128, 1);
        ASSERT(p2 != nullptr);
        void* p3 = a.alloc(1, 1);
        ASSERT(p3 == nullptr);
    }
    void* p3 = a.alloc(1, 1);
    ASSERT(p3 != nullptr);
    ASSERT(p3 != p1);
    ASSERT(p3 == p2);
}

TEST(testArenaScopeDefaultEmpty)
{
    ArenaScope scope{};
    ASSERT(static_cast<Arena*>(scope) == nullptr);
}

TEST(testArenaScopeNested)
{
    Arena a{256};
    ArenaScope outer{&a};
    void* po = a.alloc(8, 1);
    ASSERT(po != nullptr);
    void* pi = nullptr;
    {
        ArenaScope inner{&a};
        pi = a.alloc(8, 1);
        ASSERT(pi != nullptr);
        ASSERT(pi != po);
    }
    void* pAfterInner = a.alloc(8, 1);
    ASSERT(pAfterInner == pi);
    void* withinOuter = a.alloc(8, 1);
    ASSERT(withinOuter != nullptr);
    ASSERT(withinOuter != po);
}

TEST(testArenaScopeAlloc)
{
    Arena a{256};
    ArenaScope scope{&a};
    u32* p = static_cast<u32*>(scope.alloc(4, 4));
    ASSERT(p != nullptr);
    *p = 7;
    ASSERT(*p == 7);
    ASSERT(scope.alloc(4, 4) != a.alloc(4, 4));
}

TEST(testArenaScopeImplicitConvert)
{
    Arena a{256};
    ArenaScope scope{&a};
    Arena* ap = scope;
    ASSERT(ap == &a);
    void* p = ap->alloc(16, 1);
    ASSERT(p != nullptr);
}

TEST(testArenaScopeTypedAlloc)
{
    Arena a{256};
    ArenaScope scope{&a};
    u32* p = static_cast<u32*>(scope.alloc(3 * sizeof(u32), alignof(u32)));
    ASSERT(p != nullptr);
    p[0] = 10;
    p[1] = 20;
    p[2] = 30;
    ASSERT(p[0] == 10);
    ASSERT(p[1] == 20);
    ASSERT(p[2] == 30);
}

// getScratch

TEST(testGetScratchReturnsValidArena)
{
    {
        ArenaScope scope = getScratch();
        Arena* ap = scope;
        ASSERT(ap != nullptr);
        void* p = ap->alloc(64, 4);
        ASSERT(p != nullptr);
        std::memset(p, 0, 64);
    }
    {
        ArenaScope scope = getScratch();
        Arena* ap = scope;
        ASSERT(ap != nullptr);
        void* p = ap->alloc(64, 4);
        ASSERT(p != nullptr);
    }
}

TEST(testGetScratchConflict)
{
    ArenaScope a = getScratch();
    u32* p = static_cast<u32*>(a.alloc(sizeof(u32), alignof(u32)));
    ASSERT(p != nullptr);
    *p = 42;
    const Arena* conflicts[] = {a};
    ArenaScope b = getScratch(conflicts);
    ASSERT(static_cast<Arena*>(b) != static_cast<Arena*>(a));
    u32* q = static_cast<u32*>(b.alloc(sizeof(u32), alignof(u32)));
    ASSERT(q != nullptr);
    ASSERT(q != p);
    *q = 99;
    ASSERT(*q == 99);
    ASSERT(*p == 42);
}

TEST(testGetScratchConflictSingleArg)
{
    ArenaScope a = getScratch();
    const Arena* arena = a;
    ArenaScope b = getScratch(arena);
    ASSERT(static_cast<Arena*>(b) != arena);
    ASSERT(b.alloc(16, 1) != nullptr);
}

TEST(testGetScratchSpanConflicts)
{
    ArenaScope a = getScratch();
    ArenaScope b = getScratch({a});
    const Arena* conflicts[] = {a, b};
    ArenaScope c = getScratch(std::span<const Arena*>{conflicts, 2});
    ASSERT(static_cast<Arena*>(c) != static_cast<Arena*>(a));
    ASSERT(static_cast<Arena*>(c) != static_cast<Arena*>(b));
    ASSERT(c.alloc(16, 1) != nullptr);
}

// ArenaAllocator

TEST(testArenaAllocatorString)
{
    Arena a{256};
    ArenaScope scope{&a};
    string_temp s{ArenaAllocator<char>{scope}};
    s = "hello";
    s += " world";
    ASSERT(s == "hello world");
}

TEST(testArenaAllocatorLongString)
{
    Arena a{256};
    ArenaScope scope{&a};
    string_temp s{ArenaAllocator<char>{scope}};
    s.assign(64, 'x');
    ASSERT(s.size() == 64);
    ASSERT(s[0] == 'x');
    ASSERT(s[63] == 'x');
}

TEST(testArenaAllocatorVector)
{
    Arena a{256};
    ArenaScope scope{&a};
    vector_temp<u32> v{ArenaAllocator<u32>{&a}};
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    ASSERT(v.size() == 3);
    ASSERT(v[0] == 1);
    ASSERT(v[2] == 3);
}

TEST(testArenaAllocatorVectorScopeRestore)
{
    Arena a{256};
    void* reserved = a.alloc(128, 1);
    ASSERT(reserved != nullptr);
    {
        ArenaScope scope{&a};
        vector_temp<u32> v{ArenaAllocator<u32>{scope}};
        v.resize(16);
        ASSERT(v.size() == 16);
    }
    void* p = a.alloc(128, 1);
    ASSERT(p != nullptr);
}

TEST(testArenaAllocatorCompare)
{
    Arena a{256};
    ArenaScope scope{&a};
    ArenaAllocator<u32> x{&a};
    ArenaAllocator<u64> y{scope};
    ASSERT(x == y);
    Arena b{256};
    ArenaAllocator<u32> z{&b};
    ASSERT(!(x == z));
}
