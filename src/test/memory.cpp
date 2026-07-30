#include "tests.hpp"
#include "hg/memory.hpp"
#include "hg/error.hpp"

void testMemory()
{
    // ------------------------------------------------------------------
    // heapAlloc / heapFree
    // ------------------------------------------------------------------

    // heapAlloc returns non-null pointer
    {
        void* p = heapAlloc(64, 1);
        TEST(p != nullptr);
        heapFree(p, 64);
    }

    // heapAlloc zero size returns valid pointer (malloc(0) is non-null)
    {
        void* p = heapAlloc(0, 1);
        TEST(p != nullptr);
        heapFree(p, 0);
    }

    // heapAlloc<T> template — allocates typed array
    {
        u32* arr = heapAlloc<u32>(4);
        TEST(arr != nullptr);
        arr[0] = 10;
        arr[3] = 40;
        TEST(arr[0] == 10);
        TEST(arr[3] == 40);
        heapFree<u32>(arr, 4);
    }

    // ------------------------------------------------------------------
    // Arena — default constructor
    // ------------------------------------------------------------------

    // Default-constructed Arena has null memory, zero capacity
    {
        Arena a{};
        TEST(a.memory == nullptr);
        TEST(a.capacity == 0);
        TEST(a.head == 0);
        // Double-destroy is safe (nullptr check in ~Arena)
    }

    // ------------------------------------------------------------------
    // Arena — capacity constructor
    // ------------------------------------------------------------------

    // Capacity constructor allocates memory
    {
        Arena a{256};
        TEST(a.memory != nullptr);
        TEST(a.capacity == 256);
        TEST(a.head == 0);
    }

    // ------------------------------------------------------------------
    // Arena — alloc
    // ------------------------------------------------------------------

    // Allocate from arena
    {
        Arena a{256};
        u32* p = static_cast<u32*>(a.alloc(4, 4));
        TEST(p != nullptr);
        TEST(a.head == 4);
        *p = 42;
        TEST(*p == 42);
    }

    // Allocate respects alignment (bumps to next aligned address)
    {
        Arena a{256};
        // Allocate 1 byte at alignment 1 — head becomes 1
        a.alloc(1, 1);
        // Allocate 4 bytes at alignment 4 — head aligns from 1 to 4, then +4 = 8
        void* p = a.alloc(4, 4);
        TEST(p != nullptr);
        TEST(a.head == 8);
        // The pointer should be 4-byte aligned
        TEST((reinterpret_cast<uptr>(p) & 3) == 0);
    }

    // Allocate exact capacity succeeds
    {
        Arena a{16};
        void* p = a.alloc(16, 1);
        TEST(p != nullptr);
        TEST(a.head == 16);
    }

    // Allocate out of memory returns nullptr and sets error
    {
        setError("");
        Arena a{16};
        a.alloc(16, 1); // fills arena
        void* p = a.alloc(1, 1); // overflows
        TEST(p == nullptr);
        TEST(getError().length > 0);
        setError(""); // clear for subsequent tests
    }

    // alloc<T> template — typed convenience
    {
        Arena a{256};
        u32* p = a.alloc<u32>(3);
        TEST(p != nullptr);
        p[0] = 10;
        p[1] = 20;
        p[2] = 30;
        TEST(p[0] == 10);
        TEST(p[2] == 30);
    }

    // ------------------------------------------------------------------
    // Arena — extend
    // ------------------------------------------------------------------

    // Extend the last allocation — succeeds
    {
        Arena a{256};
        u32* p = a.alloc<u32>(2);
        TEST(p != nullptr);
        TEST(a.head == 8);
        bool ok = a.extend(p, 2, 4);
        TEST(ok);
        TEST(a.head == 16);
    }

    // Extend non-last allocation — fails
    {
        Arena a{256};
        u32* first = a.alloc<u32>(2);
        a.alloc<u32>(2);
        bool ok = a.extend(first, 2, 4);
        TEST(!ok);
        // head unchanged
        TEST(a.head == 16);
    }

    // Extend beyond capacity — fails
    {
        Arena a{16};
        u8* p = static_cast<u8*>(a.alloc(4, 1));
        bool ok = a.extend(p, 4, 20);
        TEST(!ok);
    }

    // ------------------------------------------------------------------
    // Arena — move semantics
    // ------------------------------------------------------------------

    // Move construct — transfers ownership
    {
        Arena a{256};
        TEST(a.memory != nullptr);
        Arena b{std::move(a)};
        TEST(a.memory == nullptr); // moved-from is empty
        TEST(a.capacity == 0);
        TEST(a.head == 0);
        TEST(b.memory != nullptr);
        TEST(b.capacity == 256);
    }

    // Move assign — transfers ownership and frees old
    {
        Arena old{64};
        Arena a{128};
        old = std::move(a);
        TEST(a.memory == nullptr);
        TEST(old.memory != nullptr);
        TEST(old.capacity == 128);
    }

    // ------------------------------------------------------------------
    // ArenaScope — default constructor
    // ------------------------------------------------------------------

    // Default-constructed ArenaScope has null arena
    {
        ArenaScope scope{};
        TEST(scope.arena == nullptr);
        TEST(scope.head == 0);
    }

    // ------------------------------------------------------------------
    // ArenaScope — RAII head restoration
    // ------------------------------------------------------------------

    // Scope restores head on destruction
    {
        Arena a{256};
        a.alloc(16, 1); // head = 16
        {
            ArenaScope scope{&a};
            TEST(scope.arena == &a);
            a.alloc(32, 1); // head = 48
            TEST(a.head == 48);
        }
        TEST(a.head == 16); // restored
    }

    // Nested scopes restore correctly
    {
        Arena a{256};
        a.alloc(8, 1); // head = 8
        {
            ArenaScope outer{&a};
            a.alloc(8, 1); // head = 16
            {
                ArenaScope inner{&a};
                a.alloc(8, 1); // head = 24
                TEST(a.head == 24);
            }
            TEST(a.head == 16); // restored by inner
        }
        TEST(a.head == 8); // restored by outer
    }

    // Scope with null arena is safe
    {
        ArenaScope scope{};
        // No crash on destruction
    }

    // ------------------------------------------------------------------
    // ArenaScope — operator Arena*
    // ------------------------------------------------------------------

    // Implicit conversion to Arena* works
    {
        ArenaScope scope;
        Arena* ap = scope; // implicit conversion
        TEST(ap == scope.arena);
    }

    // ------------------------------------------------------------------
    // ArenaScope — forward alloc/extend
    // ------------------------------------------------------------------

    // ArenaScope::alloc forwards to arena
    {
        Arena a{256};
        ArenaScope scope{&a};
        u32* p = static_cast<u32*>(scope.alloc(4, 4));
        TEST(p != nullptr);
        TEST(a.head == 4);
    }

    // ArenaScope::alloc<T> forwards to arena
    {
        Arena a{256};
        ArenaScope scope{&a};
        u32* p = scope.alloc<u32>(3);
        TEST(p != nullptr);
        p[0] = 10;
        TEST(p[0] == 10);
    }

    // ArenaScope::extend forwards to arena
    {
        Arena a{256};
        ArenaScope scope{&a};
        u32* p = scope.alloc<u32>(2);
        TEST(p != nullptr);
        bool ok = scope.extend(p, 2, 4);
        TEST(ok);
        TEST(a.head == 16);
    }

    // ------------------------------------------------------------------
    // ArenaScope — move semantics
    // ------------------------------------------------------------------

    // Move construct
    {
        Arena a{256};
        ArenaScope scope{&a};
        ArenaScope other{std::move(scope)};
        TEST(scope.arena == nullptr); // moved-from
        TEST(other.arena == &a);
    }

    // Move assign
    {
        Arena a{256};
        Arena b{256};
        ArenaScope x{&a};
        ArenaScope y{&b};
        x = std::move(y);
        TEST(y.arena == nullptr); // moved-from
        TEST(x.arena == &b);
    }

    // ------------------------------------------------------------------
    // getScratch
    // ------------------------------------------------------------------

    // getScratch returns a valid arena with allocatable memory
    {
        ArenaScope scope = getScratch();
        Arena* ap = scope;
        TEST(ap != nullptr);
        TEST(ap->memory != nullptr);
        TEST(ap->capacity > 0);
        void* p = ap->alloc(64, 4);
        TEST(p != nullptr);
    }

    // Multiple getScratch calls without conflicts return the same arena
    {
        ArenaScope a = getScratch();
        ArenaScope b = getScratch();
        TEST(a.arena == b.arena);
    }

    // getScratch with conflict returns a different arena
    {
        ArenaScope a = getScratch();
        u32* p = a.alloc<u32>(1);
        TEST(p != nullptr);
        // Passing a's arena as conflict forces getScratch to find another
        Arena const* conflicts[] = {a.arena};
        ArenaScope b = getScratch(conflicts, 1);
        TEST(b.arena != a.arena);
        // The second arena is also usable
        u32* q = b.alloc<u32>(1);
        TEST(q != nullptr);
        *q = 42;
        TEST(*q == 42);
    }
}

