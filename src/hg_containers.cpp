#include "hg_core.hpp"
#include "hg_containers.hpp"
#include "hg_templates.hpp"

namespace hg {

template<>
void serialize(Serializer* s, ArrayAny* arr)
{
    serializeBegin(s);
    serialize(s, &arr->width);
    serialize(s, &arr->align);
    serialize(s, &arr->count);
    serialize(s, &arr->capacity);
    if (!s->writing)
        arr->vals = gpaAlloc(arr->capacity * arr->width, arr->align);
    serializeVoid(s, arr->vals, arr->count * arr->width);
    serializeEnd(s);
}

ArrayAny arrayAnyCreate(u32 width, u32 align, u32 count, u32 capacity)
{
    if (capacity < count)
        capacity = count;

    ArrayAny arr{};
    arr.vals = gpaAlloc(capacity * width, align);
    arr.count = count;
    arr.capacity = capacity;
    arr.width = width;
    arr.align = align;

    return arr;
}

void arrayAnyDestroy(ArrayAny* arr)
{
    HG_ASSERT(arr != nullptr);

    gpaFree(arr->vals, arr->capacity * arr->width);
}

ArrayAny arrayAnyTemp(Arena* arena, u32 width, u32 align, u32 count, u32 capacity)
{
    HG_ASSERT(arena != nullptr);
    HG_ASSERT(count <= capacity);

    ArrayAny arr{};
    arr.vals = arenaAlloc(arena, capacity * width, align);
    arr.count = count;
    arr.capacity = capacity;
    arr.width = width;
    arr.align = align;

    return arr;
}

void arrayAnyResize(ArrayAny* arr, u32 newCount)
{
    if (newCount > arr->capacity)
    {
        arr->vals = gpaRealloc(
            arr->vals,
            arr->capacity * arr->width,
            newCount * 2 * arr->width,
            arr->align);
        arr->capacity = newCount * 2;
    }
    arr->count = newCount;
}

void arrayAnyResizeTemp(Arena* arena, ArrayAny* arr, u32 newCount)
{
    if (newCount > arr->capacity)
    {
        arr->vals = arenaRealloc(
            arena,
            arr->vals,
            arr->capacity * arr->width,
            newCount * 2 * arr->width,
            arr->align);
        arr->capacity = newCount * 2;
    }
    arr->count = newCount;
}

void* arrayAnyPush(ArrayAny* arr)
{
    if (arr->count == arr->capacity)
    {
        u32 newCapacity = arr->capacity == 0 ? 16 : arr->capacity * 2;
        arr->vals = gpaRealloc(
            arr->vals,
            arr->capacity * arr->width,
            newCapacity * arr->width,
            arr->align);
        arr->capacity = newCapacity;
    }
    return (u8*)arr->vals + arr->count++ * arr->width;
}

void* arrayAnyPushTemp(Arena* arena, ArrayAny* arr)
{
    if (arr->count == arr->capacity)
    {
        u32 newCapacity = arr->capacity == 0 ? 16 : arr->capacity * 2;
        arr->vals = arenaRealloc(
            arena,
            arr->vals,
            arr->capacity * arr->width,
            newCapacity * arr->width,
            arr->align);
        arr->capacity = newCapacity;
    }
    return (u8*)arr->vals + arr->count++ * arr->width;
}

void arrayAnyRemove(ArrayAny* arr, u32 idx, void* dst)
{
    HG_ASSERT(idx < arr->count);

    memCopy(dst, (*arr)[idx], arr->width);
    if (idx + 1 < arr->count)
    {
        memCopy(
            (*arr)[idx],
            (*arr)[idx + 1],
            (arr->count - (idx + 1)) * arr->width);
    }
    --arr->count;
}

void arrayAnyRemoveSwap(ArrayAny* arr, u32 idx, void* dst)
{
    HG_ASSERT(idx < arr->count);

    memCopy(dst, (*arr)[idx], arr->width);
    if (idx + 1 < arr->count)
    {
        memCopy(
            (*arr)[idx],
            (*arr)[arr->count - 1],
            arr->width);
    }
    --arr->count;
}

void arrayAnyPop(ArrayAny* arr, void* dst)
{
    HG_ASSERT(arr->count > 0);
    --arr->count;
    if (dst != nullptr)
        memCopy(dst, (u8*)arr->vals + arr->count * arr->width, arr->width);
}

static constexpr u32 poolStockSize = 1024;

static void poolRestock(Pool* pool)
{
    HG_ASSERT(pool != nullptr);

    void* store = gpaAlloc(poolStockSize * pool->width, pool->align);
    for (u32 i = 0; i < poolStockSize; ++i)
    {
        queuePushBack(&pool->freeList, (u8*)store + i * pool->width);
    }
    *arrayPush(&pool->itemStores) = store;
}

Pool poolCreate(u32 width, u32 align)
{
    Pool pool{};
    pool.freeList = queueCreate<void*>();
    pool.itemStores = arrayCreate<void*>(0, 4);
    pool.width = width;
    pool.align = align;
    poolRestock(&pool);
    return pool;
}

void poolDestroy(Pool* pool)
{
    HG_ASSERT(pool != nullptr);

    for (u32 i = 0; i < pool->itemStores.count; ++i)
    {
        gpaFree(pool->itemStores[i], poolStockSize);
    }
    arrayDestroy(&pool->itemStores);
    queueDestroy(&pool->freeList);
}

void* poolAlloc(Pool* pool)
{
    HG_ASSERT(pool != nullptr);

    if (pool->freeList.count == 0)
    {
        poolRestock(pool);
    }

    return queuePopFront(&pool->freeList);
}

void poolFree(Pool* pool, void* item)
{
    HG_ASSERT(pool != nullptr);
    HG_ASSERT(item != nullptr);
    queuePushFront(&pool->freeList, item);
}

HandlePool handlePoolCreate()
{
    HandlePool handles{};
    handles.handles = arrayCreate<Handle>();
    handles.freed = arrayCreate<Handle>();

    handlePoolAlloc(&handles);

    return handles;
}

void handlePoolDestroy(HandlePool* pool)
{
    HG_ASSERT(pool != nullptr);

    arrayDestroy(&pool->handles);
    arrayDestroy(&pool->freed);
}

void handlePoolReset(HandlePool* pool)
{
    HG_ASSERT(pool != nullptr);

    pool->handles.count = 0;
    pool->freed.count = 0;

    handlePoolAlloc(pool);
}

Handle handlePoolAlloc(HandlePool* pool)
{
    HG_ASSERT(pool != nullptr);

    if (pool->freed.count > 0)
    {
        Handle handle = arrayPop(&pool->freed);
        pool->handles[handleIdx(handle)] = handle;
        return handle;
    }
    else
    {
        Handle handle = {pool->handles.count};
        *arrayPush(&pool->handles) = handle;
        return handle;
    }
}

bool handlePoolAlive(HandlePool* pool, Handle handle)
{
    HG_ASSERT(pool != nullptr);

    u32 idx = handleIdx(handle);
    return handle != handleNull && idx < pool->handles.count && pool->handles[idx] == handle;
}

void handlePoolFree(HandlePool* pool, Handle handle)
{
    HG_ASSERT(pool != nullptr);
    HG_ASSERT(handlePoolAlive(pool, handle));
    pool->handles[handleIdx(handle)] = handleNull;
    *arrayPush(&pool->freed) = handleNextGeneration(handle);
}

} // namespace hg
