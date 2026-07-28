#include "tests.hpp"

void testSmartPtr()
{
    // ============================================================================
    // UniquePtr
    // ============================================================================
    //
    // UniquePtr is a move-only heap-allocated smart pointer with unique ownership.

    // Default construction yields null pointer
    {
        UniquePtr<i32> ptr;
        TEST(ptr.ptr == nullptr);
        TEST((i32*)ptr == nullptr);
    }

    // nullptr construction yields null pointer
    {
        UniquePtr<i32> ptr{nullptr};
        TEST(ptr.ptr == nullptr);
    }

    // makeUnique creates a new object on the heap
    {
        Lifecycle::stats.reset();
        {
            UniquePtr<Lifecycle> ptr = makeUnique<Lifecycle>();
            TEST(ptr.ptr != nullptr);
            TEST(ptr->valid);
            TEST((*ptr).valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.dtors == 1);
    }

    // Move construct transfers ownership
    {
        Lifecycle::stats.reset();
        UniquePtr<Lifecycle> a = makeUnique<Lifecycle>();
        Lifecycle* addr = a.ptr;
        UniquePtr<Lifecycle> b = std::move(a);
        TEST(a.ptr == nullptr);
        TEST(b.ptr == addr);
    }
    TEST(Lifecycle::stats.alive == 0);
    TEST(Lifecycle::stats.dtors == 1);

    // Move assign transfers ownership and frees old
    {
        Lifecycle::stats.reset();
        UniquePtr<Lifecycle> a = makeUnique<Lifecycle>();
        UniquePtr<Lifecycle> b = makeUnique<Lifecycle>();
        Lifecycle* addr = a.ptr;
        b = std::move(a);
        TEST(a.ptr == nullptr);
        TEST(b.ptr == addr);
    }
    TEST(Lifecycle::stats.alive == 0);
    TEST(Lifecycle::stats.dtors == 2);

    // ============================================================================
    // SharedPtr
    // ============================================================================
    //
    // SharedPtr is a reference-counted, move-only heap-allocated smart pointer
    // with shared ownership. Clone via clone() to increment ref count.

    // Default construction yields null pointer
    {
        SharedPtr<i32> ptr;
        TEST(ptr.ptr == nullptr);
        TEST((i32*)ptr == nullptr);
    }

    // nullptr construction yields null pointer
    {
        SharedPtr<i32> ptr{nullptr};
        TEST(ptr.ptr == nullptr);
    }

    // makeShared creates a new object that is alive
    {
        Lifecycle::stats.reset();
        {
            SharedPtr<Lifecycle> ptr = makeShared<Lifecycle>();
            TEST(ptr.ptr != nullptr);
            TEST(ptr->valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.dtors == 1);
    }

    // Cloning keeps object alive until last clone is destroyed
    {
        Lifecycle::stats.reset();
        {
            SharedPtr<Lifecycle> a = makeShared<Lifecycle>();
            TEST(a.ptr != nullptr);
            {
                SharedPtr<Lifecycle> b = a.clone();
                TEST(b.ptr == a.ptr);
                TEST(b->valid);
            }
            // a still valid after b destroyed
            TEST(a->valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.dtors == 1);
    }

    // Move construct transfers ownership without destroying
    {
        Lifecycle::stats.reset();
        {
            SharedPtr<Lifecycle> a = makeShared<Lifecycle>();
            SharedPtr<Lifecycle> b = std::move(a);
            TEST(a.ptr == nullptr);
            TEST(b.ptr != nullptr);
            TEST(b->valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.dtors == 1);
    }

    // Move assign transfers and destroys old target
    {
        Lifecycle::stats.reset();
        {
            SharedPtr<Lifecycle> a = makeShared<Lifecycle>();
            SharedPtr<Lifecycle> b = makeShared<Lifecycle>();
            b = std::move(a);
            TEST(a.ptr == nullptr);
            TEST(b->valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.dtors == 2);
    }
}

