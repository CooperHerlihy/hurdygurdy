/*
 * =============================================================================
 *
 * Copyright (c) 2025 Cooper Herlihy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * =============================================================================
 */

#ifndef HG_TEMPLATES_HPP
#define HG_TEMPLATES_HPP

#include "hg_assets.hpp"
#include "hg_concurrency.hpp"
#include "hg_containers.hpp"
#include "hg_core.hpp"
#include "hg_ecs.hpp"

template<typename F>
void hgThreadsFor(u64 begin, u64 end, F fn)
{
    static_assert(std::is_invocable_r_v<void, F, u64>);

    hgThreadsFor(begin, end, &fn, [](void* pfn, u64 idx)
    {
        (*(F*)pfn)(idx);
    });
}

template<typename T>
void hgSerialize(HgSerializer* s, T* val)
{
    HgStringView data = {(char*)val, sizeof(T)};
    if (s->writing)
    {
        hgSerialize(s, &data);
    }
    else
    {
        hgArenaScope(s->arena);
        hgSerialize(s, &data);
        hgMemCopy(val, data.chars, sizeof(T));
    }
}

template<typename T>
void hgAssetInit()
{
    hgAssets<T>.map = hgMapCreate<HgStringView, HgAsset<T>*>();
    hgAssets<T>.pool = hgPoolCreate<HgAsset<T>>();
}

template<typename T>
void hgAssetDeinit()
{
    hgPoolDestroy(&hgAssets<T>.pool);
    hgMapDestroy(&hgAssets<T>.map);
}

template<typename T>
void hgAssetLoadImpl(HgAsset<T>* data)
{
    (void)data;
    static_assert(false, "Asset type cannot be loaded without implementation");
}

template<typename T>
void hgAssetUnloadImpl(HgAsset<T>* data)
{
    (void)data;
    static_assert(false, "Asset type cannot be unloaded without implementation");
}

template<typename T>
HgAsset<T>* hgAssetCreate()
{
    HgAsset<T>* asset = hgPoolAlloc(&hgAssets<T>.pool);
    asset->data = {};
    asset->refCount = 1;
    asset->path = {};

    return asset;
}

template<typename T>
HgAsset<T>* hgAssetLoad(HgStringView path)
{
    if (HgAsset<T>** asset = hgMapGet(&hgAssets<T>.map, path);
        asset != nullptr)
    {
        ++(*asset)->refCount;
        return *asset;
    }

    HgAsset<T>* asset = hgPoolAlloc(&hgAssets<T>.pool);
    asset->data = {};
    asset->refCount = 1;
    asset->path = hgStringCreate(path);

    if (hgAssets<T>.map.count * 2 > hgAssets<T>.map.capacity)
        hgMapResize(&hgAssets<T>.map, hgAssets<T>.map.capacity * 2);
    hgMapAdd(&hgAssets<T>.map, asset->path, asset);

    hgAssetLoadImpl(asset);
    return asset;
}

template<typename T>
void hgAssetUnload(HgAsset<T>* asset)
{
    if (asset != nullptr && --asset->refCount == 0)
    {
        hgAssetUnloadImpl(asset);

        if (asset->path != nullptr)
        {
            hgMapRemove(&hgAssets<T>.map, asset->path);
            hgStringDestroy(&asset->path);
        }
        hgPoolFree(&hgAssets<T>.pool, asset);
    }
}

template<typename T>
void hgAssetReload(HgAsset<T>* asset)
{
    hgAssert(asset != nullptr);

    hgAssetUnloadImpl(asset);
    hgAssetLoadImpl(asset);
}

template<typename T>
HgAsset<T>* hgAssetCopy(HgAsset<T>* asset)
{
    hgAssert(asset != nullptr);
    ++asset->refCount;
    return asset;
}

template<typename T>
void hgSerialize(HgSerializer* s, HgAsset<T>** asset)
{
    if (s->writing)
    {
        HgStringView path = (*asset)->path;
        hgSerialize(s, &path);
    }
    else
    {
        hgArenaScope(s->arena);
        HgStringView path;
        hgSerialize(s, &path);
        if (path != "")
            *asset = hgAssetLoad<T>(path);
        else
            *asset = nullptr;
    }
}

template<typename T>
HgArray<T> hgArrayCreate(u32 count, u32 capacity)
{
    if (capacity < count)
        capacity = count;

    HgArray<T> arr{};
    arr.vals = hgGpaAlloc<T>(capacity);
    arr.count = count;
    arr.capacity = capacity;

    return arr;
}

template<typename T>
void hgArrayDestroy(HgArray<T>* arr)
{
    hgAssert(arr != nullptr);
    hgGpaFree(arr->vals, arr->capacity);
}

template<typename T>
HgArray<T> hgArrayTemp(HgArena* arena, u32 count, u32 capacity)
{
    hgAssert(arena != nullptr);
    hgAssert(count <= capacity);

    HgArray<T> arr{};
    arr.vals = hgArenaAlloc<T>(arena, capacity);
    arr.count = count;
    arr.capacity = capacity;

    return arr;
}

template<typename T>
void hgArrayResize(HgArray<T>* arr, u32 newCount)
{
    if (newCount > arr->capacity)
    {
        arr->vals = hgGpaRealloc(arr->vals, arr->capacity, newCount * 2);
        arr->capacity = newCount * 2;
    }
    arr->count = newCount;
}

template<typename T>
T* hgArrayPush(HgArray<T>* arr)
{
    if (arr->count == arr->capacity)
    {
        u32 newCapacity = arr->capacity == 0 ? 16 : arr->capacity * 2;
        hgGpaRealloc(arr->vals, arr->capacity, newCapacity);
        arr->capacity = newCapacity;
    }
    return &arr->vals[arr->count++];
}

template<typename T>
T* hgArrayPushTemp(HgArena* arena, HgArray<T>* arr)
{
    if (arr->count == arr->capacity)
        hgArenaRealloc(arena, arr->vals, arr->capacity, arr->capacity == 0 ? 16 : arr->capacity * 2);
    return &arr->vals[arr->count++];
}

template<typename T>
T hgArrayPop(HgArray<T>* arr)
{
    hgAssert(arr->count > 0);
    return arr->vals[--arr->count];
}

template<typename T>
HgQueue<T> hgQueueCreate(u32 capacity)
{
    HgQueue<T> queue{};
    queue.vals = hgGpaAlloc<T>(capacity);
    queue.front = 0;
    queue.back = 0;
    queue.count = 0;
    queue.capacity = capacity;
    return queue;
}

template<typename T>
void hgQueueDestroy(HgQueue<T>* queue)
{
    if (queue != nullptr)
    {
        hgGpaFree(queue->vals, queue->capacity);
    }
}

template<typename T, typename U>
void hgQueuePushFront(HgQueue<T>* queue, U val)
{
    hgAssert(queue != nullptr);
    static_assert(std::is_convertible_v<U, T>);

    ++queue->count;
    if (queue->count == queue->capacity)
    {
        u32 newCapacity = queue->capacity * 2;
        queue->vals = hgGpaRealloc(queue->vals, queue->capacity, newCapacity);

        if (queue->back < queue->front)
        {
            hgMemCopy(queue->vals + queue->capacity, queue->vals, queue->back * sizeof(T));
            queue->back += queue->capacity;
        }

        queue->capacity = newCapacity;
    }

    queue->front = (queue->front == 0 ? queue->capacity : queue->front) - 1;
    queue->vals[queue->front] = (T)val;
}

template<typename T, typename U>
void hgQueuePushBack(HgQueue<T>* queue, U val)
{
    hgAssert(queue != nullptr);
    static_assert(std::is_convertible_v<U, T>);

    ++queue->count;
    if (queue->count == queue->capacity)
    {
        u32 newCapacity = queue->capacity * 2;
        queue->vals = hgGpaRealloc(queue->vals, queue->capacity, newCapacity);

        if (queue->back < queue->front)
        {
            hgMemCopy(queue->vals + queue->capacity, queue->vals, queue->back * sizeof(T));
            queue->back += queue->capacity;
        }

        queue->capacity = newCapacity;
    }

    queue->vals[queue->back] = (T)val;
    queue->back = (queue->back + 1) % queue->capacity;
}

template<typename T>
T hgQueuePopFront(HgQueue<T>* queue)
{
    hgAssert(queue->count > 0);
    --queue->count;

    T ret = queue->vals[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    return ret;
}

template<typename T>
T hgQueuePopBack(HgQueue<T>* queue)
{
    hgAssert(queue->count > 0);
    --queue->count;

    queue->back = (queue->back == 0 ? queue->capacity : queue->back) - 1;
    return queue->vals[queue->back];
}

template<typename V>
HgSet<V> hgSetCreate(u32 slotCount)
{
    hgAssert(slotCount > 0);

    HgSet<V> set;
    set.hasVal = hgGpaAlloc<bool>(slotCount);
    set.vals = hgGpaAlloc<V>(slotCount);
    set.capacity = slotCount;
    hgSetReset(&set);
    return set;
}

template<typename V>
void hgSetDestroy(HgSet<V>* set)
{
    hgAssert(set != nullptr);

    hgGpaFree(set->hasVal);
    hgGpaFree(set->vals);
}

template<typename V>
HgSet<V> hgSetTemp(HgArena* arena, u32 slotCount)
{
    hgAssert(slotCount > 0);

    HgSet<V> set;
    set.hasVal = hgArenaAlloc<bool>(arena, slotCount);
    set.vals = hgArenaAlloc<V>(arena, slotCount);
    set.capacity = slotCount;
    hgSetReset(&set);
    return set;
}

template<typename V>
void hgSetResize(HgSet<V>* set, u32 newSize)
{
    hgAssert(set != nullptr);

    if (newSize > set->capacity)
    {
        HgSet<V> newSet = hgSetCreate<V>(newSize);

        for (u32 i = 0; i < set->capacity; ++i)
        {
            if (set->hasVal[i])
                hgSetAdd(&newSet, set->vals[i]);
        }

        hgSetDestroy(set);
        *set = newSet;
    }
}

template<typename V>
void hgSetReset(HgSet<V>* set)
{
    hgAssert(set != nullptr);

    for (u32 i = 0; i < set->capacity; ++i)
    {
        set->hasVal[i] = false;
    }
    set->count = 0;
}

template<typename V, typename T>
void hgSetAdd(HgSet<V>* set, const T& val)
{
    hgAssert(set != nullptr);
    hgAssert(set->count < set->capacity - 1);

    static_assert(std::is_convertible_v<T, V>);
    V v = (V)val;

    u32 idx = (u32)(hgHash(v) % set->capacity);
    for (u32 dist = 0; set->hasVal[idx] && !(set->vals[idx] == v); ++dist)
    {
        u32 otherDist = (u32)(hgHash(set->vals[idx]) % set->capacity) - idx;
        if (otherDist > set->capacity)
            otherDist += set->capacity;

        if (otherDist < dist)
        {
            hgSwap(&v, &set->vals[idx]);
            dist = otherDist;
        }

        idx = (idx + 1) % set->capacity;
    }

    set->hasVal[idx] = true;
    set->vals[idx] = v;
    ++set->count;
}

template<typename V, typename T>
void hgSetRemove(HgSet<V>* set, const T& val)
{
    hgAssert(set != nullptr);

    static_assert(std::is_convertible_v<T, V>);
    V v = (V)val;

    u32 idx = (u32)(hgHash(v) % set->capacity);
    while (set->hasVal[idx])
    {
        if (set->vals[idx] == v)
            break;
        idx = (idx + 1) % set->capacity;
    }
    if (!set->hasVal[idx])
        return;

    u32 next = (idx + 1) % set->capacity;
    while (set->hasVal[next])
    {
        if (hgHash(set->vals[next]) % set->capacity != next)
        {
            set->vals[idx] = set->vals[next];
            idx = next;
        }
        next = (next + 1) % set->capacity;
    }
    set->hasVal[idx] = false;
    --set->count;
}

template<typename V, typename T>
bool hgSetHas(const HgSet<V>* set, const T& val)
{
    hgAssert(set != nullptr);

    static_assert(std::is_convertible_v<T, V>);
    V v = (V)val;

    for (u32 idx = (u32)(hgHash(v) % set->capacity); set->hasVal[idx]; idx = (idx + 1) % set->capacity)
    {
        if (set->vals[idx] == v)
            return true;
    }
    return false;
}

template<typename V, typename F>
void hgSetForEach(HgSet<V>* set, F fn)
{
    hgAssert(set != nullptr);
    static_assert(std::is_invocable_r_v<void, F, V*>);

    for (u32 i = 0; i < set->capacity; ++i)
    {
        if (set->hasVal[i])
            fn(&set->vals[i]);
    }
}

template<typename K, typename V>
HgMap<K, V> hgMapCreate(u32 slotCount)
{
    hgAssert(slotCount > 0);

    HgMap<K, V> map;
    map.hasVal = hgGpaAlloc<bool>(slotCount);
    map.keys = hgGpaAlloc<K>(slotCount);
    map.vals = hgGpaAlloc<V>(slotCount);
    map.capacity = slotCount;
    hgMapReset(&map);

    return map;
}

template<typename K, typename V>
void hgMapDestroy(HgMap<K, V>* map)
{
    hgAssert(map != nullptr);

    hgGpaFree(map->hasVal, map->capacity);
    hgGpaFree(map->keys, map->capacity);
    hgGpaFree(map->vals, map->capacity);
}

template<typename K, typename V>
HgMap<K, V> hgMapTemp(HgArena* arena, u32 slotCount)
{
    hgAssert(arena != nullptr);
    hgAssert(slotCount > 0);

    HgMap<K, V> map;
    map.hasVal = hgArenaAlloc<bool>(arena, slotCount);
    map.keys = hgArenaAlloc<K>(arena, slotCount);
    map.vals = hgArenaAlloc<V>(arena, slotCount);
    map.capacity = slotCount;
    hgMapReset(&map);

    return map;
}

template<typename K, typename V>
void hgMapResize(HgMap<K, V>* map, u32 newSize)
{
    hgAssert(map != nullptr);

    if (newSize > map->capacity)
    {
        HgMap<K, V> newMap = hgMapCreate<K, V>(newSize);

        for (u32 i = 0; i < map->capacity; ++i)
        {
            if (map->hasVal[i])
                hgMapAdd(&newMap, map->keys[i], map->vals[i]);
        }

        hgMapDestroy(map);
        *map = newMap;
    }
}

template<typename K, typename V>
void hgMapReset(HgMap<K, V>* map)
{
    hgAssert(map != nullptr);

    for (u32 i = 0; i < map->capacity; ++i)
    {
        map->hasVal[i] = false;
    }
    map->count = 0;
}

template<typename K, typename V, typename T, typename U>
V* hgMapAdd(HgMap<K, V>* map, const T& key, const U& val)
{
    hgAssert(map != nullptr);
    hgAssert(map->count < map->capacity - 1);

    static_assert(std::is_convertible_v<T, K> && std::is_convertible_v<U, V>);
    K k = (K)key;
    V v = (V)val;

    u32 idx = (u32)(hgHash(k) % map->capacity);
    for (u32 dist = 0; map->hasVal[idx] && !(map->keys[idx] == k); ++dist)
    {
        u32 otherDist = (u32)(hgHash(map->keys[idx]) % map->capacity) - idx;
        if (otherDist > map->capacity)
            otherDist += map->capacity;

        if (otherDist < dist)
        {
            hgSwap(&k, &map->keys[idx]);
            hgSwap(&v, &map->vals[idx]);
            dist = otherDist;
        }

        idx = (idx + 1) % map->capacity;
    }

    map->hasVal[idx] = true;
    map->keys[idx] = k;
    map->vals[idx] = v;
    ++map->count;

    return map->vals + idx;
}

template<typename K, typename V, typename T>
bool hgMapRemove(HgMap<K, V>* map, const T& key, V* val)
{
    hgAssert(map != nullptr);

    static_assert(std::is_convertible_v<T, K>);
    K k = (K)key;

    u32 idx = (u32)(hgHash(k) % map->capacity);
    while (map->hasVal[idx])
    {
        if (map->keys[idx] == k)
            break;
        idx = (idx + 1) % map->capacity;
    }
    if (!map->hasVal[idx])
        return false;

    if (val != nullptr)
        *val = map->vals[idx];

    u32 next = (idx + 1) % map->capacity;
    while (map->hasVal[next])
    {
        if (hgHash(map->keys[next]) % map->capacity != next)
        {
            map->keys[idx] = map->keys[next];
            map->vals[idx] = map->vals[next];
            idx = next;
        }
        next = (next + 1)  % map->capacity;
    }
    map->hasVal[idx] = false;
    --map->count;

    return true;
}

template<typename K, typename V, typename T>
V* hgMapGet(const HgMap<K, V>* map, const T& key)
{
    hgAssert(map != nullptr);

    static_assert(std::is_convertible_v<T, K>);
    K k = (K)key;

    for (u32 idx = (u32)(hgHash(key) % map->capacity); map->hasVal[idx]; idx = (idx + 1) % map->capacity)
    {
        if (map->keys[idx] == k)
            return map->vals + idx;
    }
    return nullptr;
}

template<typename K, typename V, typename F>
void hgMapForEach(HgMap<K, V>* map, F fn)
{
    hgAssert(map != nullptr);
    static_assert(std::is_invocable_r_v<void, F, K*, V*>);

    for (u32 i = 0; i < map->capacity; ++i)
    {
        if (map->hasVal[i])
            fn(&map->keys[i], &map->vals[i]);
    }
}

static constexpr u32 hgPoolStockSize = 1024;

template<typename T>
void hgPoolRestock(HgPool<T>* pool)
{
    hgAssert(pool != nullptr);

    T* store = hgGpaAlloc<T>(hgPoolStockSize);
    for (u32 i = 0; i < hgPoolStockSize; ++i)
    {
        hgQueuePushBack(&pool->freeList, store + i);
    }
    *hgArrayPush(&pool->itemStores) = store;
}

template<typename T>
HgPool<T> hgPoolCreate()
{
    HgPool<T> pool{};
    pool.freeList = hgQueueCreate<T*>();
    pool.itemStores = hgArrayCreate<T*>(0, 4);
    hgPoolRestock(&pool);
    return pool;
}

template<typename T>
void hgPoolDestroy(HgPool<T>* pool)
{
    hgAssert(pool != nullptr);

    for (u32 i = 0; i < pool->itemStores.count; ++i)
    {
        hgGpaFree(pool->itemStores[i], hgPoolStockSize);
    }
    hgArrayDestroy(&pool->itemStores);
    hgQueueDestroy(&pool->freeList);
}

template<typename T>
T* hgPoolAlloc(HgPool<T>* pool)
{
    hgAssert(pool != nullptr);

    if (pool->freeList.count == 0)
    {
        hgPoolRestock(pool);
    }

    return hgQueuePopFront(&pool->freeList);
}

template<typename T>
void hgPoolFree(HgPool<T>* pool, T* item)
{
    hgAssert(pool != nullptr);
    hgAssert(item != nullptr);
    hgQueuePushFront(&pool->freeList, item);
}

template<typename T, typename Fn>
void hgEcsForEachSingle(HgEcs* ecs, Fn& fn)
{
    static_assert(std::is_invocable_r_v<void, Fn, HgEntity, T*>);

    HgEntity* e = hgEcsEntities<T>(ecs);
    HgEntity* end = e + hgEcsCount<T>(ecs);
    T* c = hgEcsComponents<T>(ecs);
    for (; e != end; ++e, ++c)
    {
        fn(*e, c);
    }
}

template<typename... Ts, typename Fn>
void hgEcsForEachMulti(HgEcs* ecs, Fn& fn)
{
    static_assert(std::is_invocable_r_v<void, Fn, HgEntity, Ts*...>);

    u64 id = hgEcsFindSmallest<Ts...>(ecs);
    HgComponent* system = hgMapGet(&ecs->components, id);
    hgAssert(system != nullptr);

    HgEntity* e = system->entities.vals + 1;
    HgEntity* end = e + system->entities.count - 1;
    for (; e != end; ++e)
    {
        if (hgEcsHasAll<Ts...>(ecs, *e))
            fn(*e, hgEcsGet<Ts>(ecs, *e)...);
    }
}

template<typename... Ts, typename Fn>
void hgEcsForEach(HgEcs* ecs, Fn fn)
{
    static_assert(sizeof...(Ts) != 0);

    if constexpr (sizeof...(Ts) == 1)
    {
        hgEcsForEachSingle<Ts...>(ecs, fn);
    } else {
        hgEcsForEachMulti<Ts...>(ecs, fn);
    }
}

template<typename T, typename Fn>
void hgEcsForParSingle(HgEcs* ecs, Fn& fn)
{
    static_assert(std::is_invocable_r_v<void, Fn, HgEntity, T*>);

    struct Capture {
        HgEcs* ecs;
        Fn* fn;
    };
    Capture capture{ecs, &fn};

    hgThreadsFor(0, hgEcsCount<T>(ecs), &capture, [](void* pcapture, u64 idx)
    {
        Capture* capture = (Capture*)pcapture;
        (*capture->fn)(
            hgEcsEntities<T>(capture->ecs)[idx],
            &hgEcsComponents<T>(capture->ecs)[idx]);
    });
}

template<typename... Ts, typename Fn>
void hgEcsForParMulti(HgEcs* ecs, Fn& fn)
{
    static_assert(std::is_invocable_r_v<void, Fn, HgEntity, Ts*...>);

    HgComponent* system = hgMapGet(&ecs->components, hgEcsFindSmallest<Ts...>(ecs));
    hgAssert(system != nullptr);

    struct Capture {
        HgEcs* ecs;
        HgComponent* system;
        Fn* fn;
    };
    Capture capture{ecs, system, &fn};

    hgThreadsFor(1, system->entities.count, &capture, [](void* pcapture, u64 idx)
    {
        Capture* capture = (Capture*)pcapture;
        HgEntity e = capture->system->entities[idx];
        if (hgEcsHasAll<Ts...>(capture->ecs, e))
            (*capture->fn)(e, hgEcsGet<Ts>(capture->ecs, e)...);
    });
}

template<typename... Ts, typename Fn>
void hgEcsForPar(HgEcs* ecs, Fn fn)
{
    static_assert(sizeof...(Ts) != 0);

    if constexpr (sizeof...(Ts) == 1)
    {
        hgEcsForParSingle<Ts...>(ecs, fn);
    } else {
        hgEcsForParMulti<Ts...>(ecs, fn);
    }
}

#endif // HG_TEMPLATES_HPP
