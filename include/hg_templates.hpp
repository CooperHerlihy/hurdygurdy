/*
 * =============================================================================
 *
 * Copyright (c) 2025-2026 Cooper Herlihy
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

#include "hurdygurdy.hpp"

namespace hg {

template<typename F>
void forPar(u64 begin, u64 end, F fn)
{
    static_assert(std::is_invocable_r_v<void, F, u64>);

    forPar(begin, end, &fn, [](void* pfn, u64 idx)
    {
        (*(F*)pfn)(idx);
    });
}

template<typename T>
void serialize(Serializer* s, T* val)
{
    serializeVoid(s, val, sizeof(*val));
}

template<typename... Ts>
void serializeObject(Serializer* s, Ts*... vals)
{
    serializeBegin(s);
    (serialize(s, vals), ...);
    serializeEnd(s);
}

template<typename T, u64 N>
void serialize(Serializer* s, T (*arr)[N])
{
    serializeBegin(s);
    for (u64 i = 0; i < N; ++i)
    {
        serialize(s, &(*arr)[i]);
    }
    serializeEnd(s);
}

template<typename T>
void serialize(Serializer* s, Array<T>* arr)
{
    serializeBegin(s);
    serialize(s, &arr->count);
    serialize(s, &arr->capacity);
    if (!s->writing)
        arr->vals = heapAlloc<T>(arr->capacity);
    for (u32 i = 0; i < arr->count; ++i)
    {
        serialize(s, &(*arr)[i]);
    }
    serializeEnd(s);
}

template<typename T>
Array<T> arrayCreate(u32 count, u32 capacity)
{
    if (capacity < count)
        capacity = count;

    Array<T> arr{};
    arr.vals = heapAlloc<T>(capacity);
    arr.count = count;
    arr.capacity = capacity;

    return arr;
}

template<typename T>
void arrayDestroy(Array<T>* arr)
{
    HG_ASSERT(arr != nullptr);
    heapFree(arr->vals, arr->capacity);
}

template<typename T>
Array<T> arrayTemp(Arena* arena, u32 count, u32 capacity)
{
    HG_ASSERT(arena != nullptr);
    HG_ASSERT(count <= capacity);

    Array<T> arr{};
    arr.vals = arenaAlloc<T>(arena, capacity);
    arr.count = count;
    arr.capacity = capacity;

    return arr;
}

template<typename T>
void arrayResize(Array<T>* arr, u32 newCount)
{
    if (newCount > arr->capacity)
    {
        arr->vals = heapRealloc(arr->vals, arr->capacity, newCount * 2);
        arr->capacity = newCount * 2;
    }
    arr->count = newCount;
}

template<typename T>
void arrayResizeTemp(Arena* arena, Array<T>* arr, u32 newCount)
{
    if (newCount > arr->capacity)
    {
        arr->vals = arenaRealloc(arena, arr->vals, arr->capacity, newCount * 2);
        arr->capacity = newCount * 2;
    }
    arr->count = newCount;
}

template<typename T>
T* arrayPush(Array<T>* arr)
{
    if (arr->count == arr->capacity)
    {
        u32 newCapacity = arr->capacity == 0 ? 16 : arr->capacity * 2;
        arr->vals = heapRealloc(arr->vals, arr->capacity, newCapacity);
        arr->capacity = newCapacity;
    }
    return &arr->vals[arr->count++];
}

template<typename T>
T* arrayPushTemp(Arena* arena, Array<T>* arr)
{
    if (arr->count == arr->capacity)
    {
        u32 newCapacity = arr->capacity == 0 ? 16 : arr->capacity * 2;
        arr->vals = arenaRealloc(arena, arr->vals, arr->capacity, newCapacity);
        arr->capacity = newCapacity;
    }
    return &arr->vals[arr->count++];
}

template<typename T>
T arrayRemove(Array<T>* arr, u32 idx)
{
    HG_ASSERT(idx < arr->count);

    T val = (*arr)[idx];
    if (idx + 1 < arr->count)
    {
        memCopy(
            &(*arr)[idx],
            &(*arr)[idx + 1],
            (arr->count - (idx + 1)) * sizeof(T));
    }
    --arr->count;
    return val;
}

template<typename T>
T arrayRemoveSwap(Array<T>* arr, u32 idx)
{
    T val = (*arr)[idx];
    if (idx + 1 < arr->count)
    {
        memCopy(
            &(*arr)[idx],
            &(*arr)[arr->count - 1],
            sizeof(T));
    }
    --arr->count;
    return val;
}

template<typename T>
T arrayPop(Array<T>* arr)
{
    HG_ASSERT(arr->count > 0);
    return arr->vals[--arr->count];
}

template<typename T>
Queue<T> queueCreate(u32 capacity)
{
    Queue<T> queue{};
    queue.vals = heapAlloc<T>(capacity);
    queue.front = 0;
    queue.back = 0;
    queue.count = 0;
    queue.capacity = capacity;
    return queue;
}

template<typename T>
void queueDestroy(Queue<T>* queue)
{
    if (queue != nullptr)
    {
        heapFree(queue->vals, queue->capacity);
    }
}

template<typename T, typename U>
void queuePushFront(Queue<T>* queue, U val)
{
    HG_ASSERT(queue != nullptr);
    static_assert(std::is_convertible_v<U, T>);

    ++queue->count;
    if (queue->count == queue->capacity)
    {
        u32 newCapacity = queue->capacity * 2;
        queue->vals = heapRealloc(queue->vals, queue->capacity, newCapacity);

        if (queue->back < queue->front)
        {
            memCopy(queue->vals + queue->capacity, queue->vals, queue->back * sizeof(T));
            queue->back += queue->capacity;
        }

        queue->capacity = newCapacity;
    }

    queue->front = (queue->front == 0 ? queue->capacity : queue->front) - 1;
    queue->vals[queue->front] = (T)val;
}

template<typename T, typename U>
void queuePushBack(Queue<T>* queue, U val)
{
    HG_ASSERT(queue != nullptr);
    static_assert(std::is_convertible_v<U, T>);

    ++queue->count;
    if (queue->count == queue->capacity)
    {
        u32 newCapacity = queue->capacity * 2;
        queue->vals = heapRealloc(queue->vals, queue->capacity, newCapacity);

        if (queue->back < queue->front)
        {
            memCopy(queue->vals + queue->capacity, queue->vals, queue->back * sizeof(T));
            queue->back += queue->capacity;
        }

        queue->capacity = newCapacity;
    }

    queue->vals[queue->back] = (T)val;
    queue->back = (queue->back + 1) % queue->capacity;
}

template<typename T>
T queuePopFront(Queue<T>* queue)
{
    HG_ASSERT(queue->count > 0);
    --queue->count;

    T ret = queue->vals[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    return ret;
}

template<typename T>
T queuePopBack(Queue<T>* queue)
{
    HG_ASSERT(queue->count > 0);
    --queue->count;

    queue->back = (queue->back == 0 ? queue->capacity : queue->back) - 1;
    return queue->vals[queue->back];
}

template<typename V>
void serialize(Serializer* s, Set<V>* set)
{
    serializeBegin(s);

    serialize(s, &set->count);
    serialize(s, &set->capacity);

    if (s->writing)
    {
        setForEach(set, [&](V* val)
        {
            serialize(s, val);
        });
    }
    else
    {
        u32 count = set->count;
        setDestroy(set);
        *set = setCreate(set->capacity);
        for (u32 i = 0; i < count; ++i)
        {
            V val;
            serialize(s, &val);
            setAdd(set, val);
        }
    }

    serializeEnd(s);
}

template<typename V>
Set<V> setCreate(u32 slotCount)
{
    HG_ASSERT(slotCount > 0);

    Set<V> set;
    set.hasVal = heapAlloc<bool>(slotCount);
    set.vals = heapAlloc<V>(slotCount);
    set.capacity = slotCount;
    setReset(&set);
    return set;
}

template<typename V>
void setDestroy(Set<V>* set)
{
    HG_ASSERT(set != nullptr);

    heapFree(set->hasVal);
    heapFree(set->vals);
}

template<typename V>
Set<V> setTemp(Arena* arena, u32 slotCount)
{
    HG_ASSERT(slotCount > 0);

    Set<V> set;
    set.hasVal = arenaAlloc<bool>(arena, slotCount);
    set.vals = arenaAlloc<V>(arena, slotCount);
    set.capacity = slotCount;
    setReset(&set);
    return set;
}

template<typename V>
void setResize(Set<V>* set, u32 newSize)
{
    HG_ASSERT(set != nullptr);

    if (newSize > set->capacity)
    {
        Set<V> newSet = setCreate<V>(newSize);

        for (u32 i = 0; i < set->capacity; ++i)
        {
            if (set->hasVal[i])
                setAdd(&newSet, set->vals[i]);
        }

        setDestroy(set);
        *set = newSet;
    }
}

template<typename V>
void setReset(Set<V>* set)
{
    HG_ASSERT(set != nullptr);

    for (u32 i = 0; i < set->capacity; ++i)
    {
        set->hasVal[i] = false;
    }
    set->count = 0;
}

template<typename V, typename T>
void setAdd(Set<V>* set, const T& val)
{
    HG_ASSERT(set != nullptr);
    HG_ASSERT(set->count < set->capacity - 1);

    static_assert(std::is_convertible_v<T, V>);
    V v = (V)val;

    u32 idx = (u32)(hash(v) % set->capacity);
    for (u32 dist = 0; set->hasVal[idx] && !(set->vals[idx] == v); ++dist)
    {
        u32 otherDist = (u32)(hash(set->vals[idx]) % set->capacity) - idx;
        if (otherDist > set->capacity)
            otherDist += set->capacity;

        if (otherDist < dist)
        {
            std::swap(v, set->vals[idx]);
            dist = otherDist;
        }

        idx = (idx + 1) % set->capacity;
    }

    set->hasVal[idx] = true;
    set->vals[idx] = v;
    ++set->count;
}

template<typename V, typename T>
void setRemove(Set<V>* set, const T& val)
{
    HG_ASSERT(set != nullptr);

    static_assert(std::is_convertible_v<T, V>);
    V v = (V)val;

    u32 idx = (u32)(hash(v) % set->capacity);
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
        if (hash(set->vals[next]) % set->capacity != next)
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
bool setHas(const Set<V>* set, const T& val)
{
    HG_ASSERT(set != nullptr);

    static_assert(std::is_convertible_v<T, V>);
    V v = (V)val;

    for (u32 idx = (u32)(hash(v) % set->capacity); set->hasVal[idx]; idx = (idx + 1) % set->capacity)
    {
        if (set->vals[idx] == v)
            return true;
    }
    return false;
}

template<typename V, typename F>
void setForEach(Set<V>* set, F fn)
{
    HG_ASSERT(set != nullptr);
    static_assert(std::is_invocable_r_v<void, F, V*>);

    for (u32 i = 0; i < set->capacity; ++i)
    {
        if (set->hasVal[i])
            fn(&set->vals[i]);
    }
}

template<typename K, typename V>
void serialize(Serializer* s, Map<K, V>* map)
{
    serializeBegin(s);

    serialize(s, &map->count);
    serialize(s, &map->capacity);

    if (s->writing)
    {
        mapForEach(map, [&](K* key, V* val)
        {
            serializeObject(s, key, val);
        });
    }
    else
    {
        u32 count = map->count;
        mapDestroy(map);
        *map = mapCreate(map->capacity);
        for (u32 i = 0; i < count; ++i)
        {
            K key;
            V val;
            serializeObject(s, &key, &val);
            mapAdd(map, key, val);
        }
    }

    serializeEnd(s);
}

template<typename K, typename V>
Map<K, V> mapCreate(u32 slotCount)
{
    HG_ASSERT(slotCount > 0);

    Map<K, V> map;
    map.hasVal = heapAlloc<bool>(slotCount);
    map.keys = heapAlloc<K>(slotCount);
    map.vals = heapAlloc<V>(slotCount);
    map.capacity = slotCount;
    mapReset(&map);

    return map;
}

template<typename K, typename V>
void mapDestroy(Map<K, V>* map)
{
    HG_ASSERT(map != nullptr);

    heapFree(map->hasVal, map->capacity);
    heapFree(map->keys, map->capacity);
    heapFree(map->vals, map->capacity);
}

template<typename K, typename V>
Map<K, V> mapTemp(Arena* arena, u32 slotCount)
{
    HG_ASSERT(arena != nullptr);
    HG_ASSERT(slotCount > 0);

    Map<K, V> map;
    map.hasVal = arenaAlloc<bool>(arena, slotCount);
    map.keys = arenaAlloc<K>(arena, slotCount);
    map.vals = arenaAlloc<V>(arena, slotCount);
    map.capacity = slotCount;
    mapReset(&map);

    return map;
}

template<typename K, typename V>
void mapResize(Map<K, V>* map, u32 newSize)
{
    HG_ASSERT(map != nullptr);

    if (newSize > map->capacity)
    {
        Map<K, V> newMap = mapCreate<K, V>(newSize);

        for (u32 i = 0; i < map->capacity; ++i)
        {
            if (map->hasVal[i])
                mapAdd(&newMap, map->keys[i], map->vals[i]);
        }

        mapDestroy(map);
        *map = newMap;
    }
}

template<typename K, typename V>
void mapReset(Map<K, V>* map)
{
    HG_ASSERT(map != nullptr);

    for (u32 i = 0; i < map->capacity; ++i)
    {
        map->hasVal[i] = false;
    }
    map->count = 0;
}

template<typename K, typename V, typename T, typename U>
V* mapAdd(Map<K, V>* map, const T& key, const U& val)
{
    HG_ASSERT(map != nullptr);
    HG_ASSERT(map->count < map->capacity - 1);

    static_assert(std::is_convertible_v<T, K> && std::is_convertible_v<U, V>);
    K k = (K)key;
    V v = (V)val;

    u32 idx = (u32)(hash(k) % map->capacity);
    for (u32 dist = 0; map->hasVal[idx] && !(map->keys[idx] == k); ++dist)
    {
        u32 otherDist = (u32)(hash(map->keys[idx]) % map->capacity) - idx;
        if (otherDist > map->capacity)
            otherDist += map->capacity;

        if (otherDist < dist)
        {
            std::swap(k, map->keys[idx]);
            std::swap(v, map->vals[idx]);
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
bool mapRemove(Map<K, V>* map, const T& key, V* val)
{
    HG_ASSERT(map != nullptr);

    static_assert(std::is_convertible_v<T, K>);
    K k = (K)key;

    u32 idx = (u32)(hash(k) % map->capacity);
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
        if (hash(map->keys[next]) % map->capacity != next)
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
V* mapGet(const Map<K, V>* map, const T& key)
{
    HG_ASSERT(map != nullptr);

    static_assert(std::is_convertible_v<T, K>);
    K k = (K)key;

    for (u32 idx = (u32)(hash(k) % map->capacity); map->hasVal[idx]; idx = (idx + 1) % map->capacity)
    {
        if (map->keys[idx] == k)
            return map->vals + idx;
    }
    return nullptr;
}

template<typename K, typename V, typename F>
void mapForEach(Map<K, V>* map, F fn)
{
    HG_ASSERT(map != nullptr);
    static_assert(std::is_invocable_r_v<void, F, K*, V*>);

    for (u32 i = 0; i < map->capacity; ++i)
    {
        if (map->hasVal[i])
            fn(&map->keys[i], &map->vals[i]);
    }
}

template<typename T>
void assetInit()
{
    assets<T>.map = mapCreate<String, Asset<T>*>();
    assets<T>.pool = poolCreate<Asset<T>>();
}

template<typename T>
void assetDeinit()
{
    poolDestroy(&assets<T>.pool);
    mapDestroy(&assets<T>.map);
}

template<typename T>
void assetLoadImpl(Asset<T>* data)
{
    (void)data;
    static_assert(false, "Asset type cannot be loaded without implementation");
}

template<typename T>
void assetUnloadImpl(Asset<T>* data)
{
    (void)data;
    static_assert(false, "Asset type cannot be unloaded without implementation");
}

template<typename T>
Asset<T>* assetCreate()
{
    Asset<T>* data = (Asset<T>*)poolAlloc(&assets<T>.pool);
    data->asset = {};
    data->refCount = 1;
    data->path = {};

    return data;
}

template<typename T>
Asset<T>* assetLoad(String path)
{
    if (Asset<T>** asset = mapGet(&assets<T>.map, path);
        asset != nullptr)
    {
        ++(*asset)->refCount;
        return *asset;
    }

    Asset<T>* data = (Asset<T>*)poolAlloc(&assets<T>.pool);
    data->asset = {};
    data->refCount = 1;
    data->path = stringCreate(path);

    if (assets<T>.map.count * 2 > assets<T>.map.capacity)
        mapResize(&assets<T>.map, assets<T>.map.capacity * 2);
    mapAdd(&assets<T>.map, data->path, data);

    assetLoadImpl(data);
    return data;
}

template<typename T>
void assetUnload(Asset<T>* asset)
{
    if (asset != nullptr && --asset->refCount == 0)
    {
        assetUnloadImpl(asset);

        if (asset->path != nullptr)
        {
            mapRemove(&assets<T>.map, asset->path);
            stringDestroy(&asset->path);
        }
        poolFree(&assets<T>.pool, asset);
    }
}

template<typename T>
void assetReload(Asset<T>* asset)
{
    HG_ASSERT(asset != nullptr);

    assetUnloadImpl(asset);
    assetLoadImpl(asset);
}

template<typename T>
Asset<T>* assetCopy(Asset<T>* asset)
{
    HG_ASSERT(asset != nullptr);
    ++asset->refCount;
    return asset;
}

template<typename T>
void serialize(Serializer* s, Asset<T>** asset)
{
    if (s->writing)
    {
        String path = (*asset)->path;
        serialize(s, &path);
    }
    else
    {
        HG_ARENA_SCOPE(s->arena);
        StringBuilder path;
        serialize(s, &path);
        if (path != "")
            *asset = assetLoad<T>(path);
        else
            *asset = nullptr;
    }
}

template<typename T, typename Fn>
void ecsForEachSingle(Ecs* ecs, Fn& fn)
{
    static_assert(std::is_invocable_r_v<void, Fn, Entity, T*>);

    Entity* e = ecsEntities<T>(ecs);
    Entity* end = e + ecsCount<T>(ecs);
    T* c = ecsComponents<T>(ecs);
    for (; e != end; ++e, ++c)
    {
        fn(*e, c);
    }
}

template<typename... Ts, typename Fn>
void ecsForEachMulti(Ecs* ecs, Fn& fn)
{
    static_assert(std::is_invocable_r_v<void, Fn, Entity, Ts*...>);

    u64 id = ecsFindSmallest<Ts...>(ecs);
    Component* system = mapGet(&ecs->components, id);
    HG_ASSERT(system != nullptr);

    Entity* e = system->entities.vals + 1;
    Entity* end = e + system->entities.count - 1;
    for (; e != end; ++e)
    {
        if (ecsHasAll<Ts...>(ecs, *e))
            fn(*e, ecsGet<Ts>(ecs, *e)...);
    }
}

template<typename... Ts, typename Fn>
void ecsForEach(Ecs* ecs, Fn fn)
{
    static_assert(sizeof...(Ts) != 0);

    if constexpr (sizeof...(Ts) == 1)
    {
        ecsForEachSingle<Ts...>(ecs, fn);
    } else {
        ecsForEachMulti<Ts...>(ecs, fn);
    }
}

template<typename T, typename Fn>
void ecsForParSingle(Ecs* ecs, Fn& fn)
{
    static_assert(std::is_invocable_r_v<void, Fn, Entity, T*>);

    struct Capture {
        Ecs* ecs;
        Fn* fn;
    };
    Capture capture{ecs, &fn};

    forPar(0, ecsCount<T>(ecs), &capture, [](void* pcapture, u64 idx)
    {
        Capture* capture = (Capture*)pcapture;
        (*capture->fn)(
            ecsEntities<T>(capture->ecs)[idx],
            &ecsComponents<T>(capture->ecs)[idx]);
    });
}

template<typename... Ts, typename Fn>
void ecsForParMulti(Ecs* ecs, Fn& fn)
{
    static_assert(std::is_invocable_r_v<void, Fn, Entity, Ts*...>);

    Component* system = mapGet(&ecs->components, ecsFindSmallest<Ts...>(ecs));
    HG_ASSERT(system != nullptr);

    struct Capture {
        Ecs* ecs;
        Component* system;
        Fn* fn;
    };
    Capture capture{ecs, system, &fn};

    forPar(1, system->entities.count, &capture, [](void* pcapture, u64 idx)
    {
        Capture* capture = (Capture*)pcapture;
        Entity e = capture->system->entities[idx];
        if (ecsHasAll<Ts...>(capture->ecs, e))
            (*capture->fn)(e, ecsGet<Ts>(capture->ecs, e)...);
    });
}

template<typename... Ts, typename Fn>
void ecsForPar(Ecs* ecs, Fn fn)
{
    static_assert(sizeof...(Ts) != 0);

    if constexpr (sizeof...(Ts) == 1)
    {
        ecsForParSingle<Ts...>(ecs, fn);
    } else {
        ecsForParMulti<Ts...>(ecs, fn);
    }
}

} // namespace hg

#endif // TEMPLATES_HPP
