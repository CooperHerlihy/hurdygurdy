#include "tests.hpp"
#include "hg/smart_ptr.hpp"

using namespace hg;

TEST(testUniquePtrDefault)
{
    UniquePtr<i32> ptr;
    ASSERT(ptr.ptr == nullptr);
    ASSERT((i32*)ptr == nullptr);
}

TEST(testUniquePtrNullptr)
{
    UniquePtr<i32> ptr{nullptr};
    ASSERT(ptr.ptr == nullptr);
}

TEST(testUniquePtrMakeUnique)
{
    Lifecycle::stats.reset();
    {
        UniquePtr<Lifecycle> ptr = makeUnique<Lifecycle>();
        ASSERT(ptr.ptr != nullptr);
        ASSERT(ptr->valid);
        ASSERT((*ptr).valid);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 1);
}

TEST(testUniquePtrMoveConstruct)
{
    Lifecycle::stats.reset();
    {
        UniquePtr<Lifecycle> a = makeUnique<Lifecycle>();
        Lifecycle* addr = a.ptr;
        UniquePtr<Lifecycle> b = std::move(a);
        ASSERT(a.ptr == nullptr);
        ASSERT(b.ptr == addr);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 1);
}

TEST(testUniquePtrMoveAssign)
{
    Lifecycle::stats.reset();
    {
        UniquePtr<Lifecycle> a = makeUnique<Lifecycle>();
        UniquePtr<Lifecycle> b = makeUnique<Lifecycle>();
        Lifecycle* addr = a.ptr;
        b = std::move(a);
        ASSERT(a.ptr == nullptr);
        ASSERT(b.ptr == addr);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 2);
}

TEST(testSharedPtrDefault)
{
    SharedPtr<i32> ptr;
    ASSERT(ptr.ptr == nullptr);
    ASSERT((i32*)ptr == nullptr);
}

TEST(testSharedPtrNullptr)
{
    SharedPtr<i32> ptr{nullptr};
    ASSERT(ptr.ptr == nullptr);
}

TEST(testSharedPtrMakeShared)
{
    Lifecycle::stats.reset();
    {
        SharedPtr<Lifecycle> ptr = makeShared<Lifecycle>();
        ASSERT(ptr.ptr != nullptr);
        ASSERT(ptr->valid);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 1);
}

TEST(testSharedPtrClone)
{
    Lifecycle::stats.reset();
    {
        SharedPtr<Lifecycle> a = makeShared<Lifecycle>();
        ASSERT(a.ptr != nullptr);
        {
            SharedPtr<Lifecycle> b = a.clone();
            ASSERT(b.ptr == a.ptr);
            ASSERT(b->valid);
        }
        ASSERT(a->valid);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 1);
}

TEST(testSharedPtrMoveConstruct)
{
    Lifecycle::stats.reset();
    {
        SharedPtr<Lifecycle> a = makeShared<Lifecycle>();
        SharedPtr<Lifecycle> b = std::move(a);
        ASSERT(a.ptr == nullptr);
        ASSERT(b.ptr != nullptr);
        ASSERT(b->valid);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 1);
}

TEST(testSharedPtrMoveAssign)
{
    Lifecycle::stats.reset();
    {
        SharedPtr<Lifecycle> a = makeShared<Lifecycle>();
        SharedPtr<Lifecycle> b = makeShared<Lifecycle>();
        b = std::move(a);
        ASSERT(a.ptr == nullptr);
        ASSERT(b->valid);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 2);
}

TEST(testUniquePtrComparison)
{
    UniquePtr<i32> a{nullptr};
    UniquePtr<i32> b{nullptr};
    ASSERT((i32*)a == (i32*)b);
}

TEST(testUniquePtrComparisonDifferent)
{
    UniquePtr<i32> a = makeUnique<i32>(1);
    UniquePtr<i32> b = makeUnique<i32>(2);
    ASSERT((i32*)a != (i32*)b);
}

TEST(testSharedPtrCloneMultiple)
{
    Lifecycle::stats.reset();
    {
        SharedPtr<Lifecycle> a = makeShared<Lifecycle>();
        ASSERT(a.ptr->refCount == 1);
        {
            SharedPtr<Lifecycle> b = a.clone();
            ASSERT(a.ptr->refCount == 2);
            {
                SharedPtr<Lifecycle> c = a.clone();
                ASSERT(a.ptr->refCount == 3);
            }
            ASSERT(a.ptr->refCount == 2);
        }
        ASSERT(a.ptr->refCount == 1);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 1);
}

TEST(testSharedPtrCloneLifecycle)
{
    Lifecycle::stats.reset();
    {
        SharedPtr<Lifecycle> a = makeShared<Lifecycle>();
        SharedPtr<Lifecycle> b = a.clone();
        SharedPtr<Lifecycle> c = a.clone();
        SharedPtr<Lifecycle> d = a.clone();
        ASSERT(b->valid);
        ASSERT(c->valid);
        ASSERT(d->valid);
        ASSERT(a.ptr->refCount == 4);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 1);
}

TEST(testSharedPtrMoveFromClone)
{
    Lifecycle::stats.reset();
    {
        SharedPtr<Lifecycle> a = makeShared<Lifecycle>();
        SharedPtr<Lifecycle> b = a.clone();
        SharedPtr<Lifecycle> c = std::move(b);
        ASSERT(b.ptr == nullptr);
        ASSERT(a->valid);
        ASSERT(c->valid);
        ASSERT(a.ptr->refCount == 2);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 1);
}

TEST(testSharedPtrNullptrDerefSafety)
{
    SharedPtr<i32> ptr{nullptr};
    ASSERT((i32*)ptr == nullptr);
}

TEST(testUniquePtrMoveSelf)
{
    Lifecycle::stats.reset();
    {
        UniquePtr<Lifecycle> a = makeUnique<Lifecycle>();
        Lifecycle* addr = a.ptr;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
        a = std::move(a);
#pragma clang diagnostic pop
        ASSERT(a.ptr == addr);
        ASSERT(a->valid);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 1);
}

TEST(testSharedPtrMoveSelf)
{
    Lifecycle::stats.reset();
    {
        SharedPtr<Lifecycle> a = makeShared<Lifecycle>();
        Lifecycle* addr = &a.ptr->val;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
        a = std::move(a);
#pragma clang diagnostic pop
        ASSERT(&a.ptr->val == addr);
        ASSERT(a->valid);
        ASSERT(a.ptr->refCount == 1);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 1);
}
