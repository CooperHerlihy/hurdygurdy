#pragma once
#undef HG_NO_LOGGING
#define HG_LOGGING 1
#include "hurdygurdy.hpp"

#define TEST(cond) do { \
    if (!(cond)) \
        HG_PANIC("Test failed in " __FILE__ ":%d %s() \"" #cond "\"\n", __LINE__, __func__); \
} while(0)

using namespace hg;

/**
 * Tracks lifetime events for testing RAII correctness
 *
 * Each instance gets a unique ID.  Stats tracks aggregate counts
 * (alive, default-constructed, copy-constructed, moved, destroyed).
 * Call stats.reset() before a test block, then verify at scope exits.
 *
 * The Tag parameter enables independent counters per subsystem:
 *   using MyTypeLifecycle = LifecycleT<struct MyTypeTag>;
 */
template<typename Tag = void>
struct LifecycleT {
    struct Stats {
        i64 alive = 0;
        i64 ctors = 0;
        i64 copies = 0;
        i64 moves = 0;
        i64 dtors = 0;

        void reset() { *this = Stats{}; }
    };

    static Stats stats;
    static u64 s_nextId;

    bool valid = false;
    u64 id;

    LifecycleT()
        : valid(true)
        , id(s_nextId++)
    {
        stats.alive++;
        stats.ctors++;
    }

    LifecycleT(const LifecycleT& o)
        : valid(o.valid)
        , id(s_nextId++)
    {
        if (valid)
        {
            stats.alive++;
            stats.copies++;
        }
    }

    LifecycleT(LifecycleT&& o)
        : valid(o.valid)
        , id(o.id)
    {
        o.valid = false;
        stats.moves++;
    }

    ~LifecycleT()
    {
        if (valid)
        {
            stats.alive--;
            stats.dtors++;
        }
    }

    LifecycleT& operator=(const LifecycleT& o)
    {
        if (this != &o)
        {
            if (valid)
            {
                stats.alive--;
                stats.dtors++;
            }
            valid = o.valid;
            id = o.id;
            if (valid)
            {
                stats.alive++;
                stats.copies++;
            }
        }
        return *this;
    }

    LifecycleT& operator=(LifecycleT&& o)
    {
        if (this != &o)
        {
            if (valid)
            {
                stats.alive--;
                stats.dtors++;
            }
            valid = o.valid;
            id = o.id;
            o.valid = false;
            stats.moves++;
        }
        return *this;
    }
};

template<typename Tag>
typename LifecycleT<Tag>::Stats LifecycleT<Tag>::stats{};

template<typename Tag>
u64 LifecycleT<Tag>::s_nextId = 0;

using Lifecycle = LifecycleT<>;

inline bool operator==(const Lifecycle& a, const Lifecycle& b)
{
    return a.id == b.id;
}

namespace hg {

template<>
inline u64 hash(Lifecycle val)
{
    return hash(val.id);
}

template<>
inline void serialize(Serializer* s, Lifecycle* val)
{
    serializeBegin(s);
    i64 id = static_cast<i64>(val->id);
    serialize(s, &id);
    serialize(s, &val->valid);
    serializeEnd(s);

    if (!s->writing)
        val->id = static_cast<u64>(id);
}

}

void testSpan();
void testProduct();
void testSum();
void testMaybe();
void testError();
void testUtils();
void testMemory();
void testConcurrency();
void testMath();
void testGeometry2D();
void testGeometry3D();
void testNoise();
void testStrings();
void testBinary();
void testSmartPtr();
void testArray();
void testQueue();
void testSet();
void testMap();
void testPool();
void testAssets();
void testSerialization();
void testGpu();
void testRender2D();
void testEcs();

