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

#ifndef HG_ECS_HPP
#define HG_ECS_HPP

#include "hg_core.hpp"
#include "hg_memory.hpp"
#include "hg_containers.hpp"
#include "hg_math.hpp"
#include "hg_concurrency.hpp"
#include "hg_assets.hpp"

/**
 * An entity in the ecs
 */
struct HgEntity {
    /**
     * The entity handle
     */
    HgHandle handle;
};

/**
 * Compare entities
 */
constexpr bool operator==(HgEntity lhs, HgEntity rhs)
{
    return lhs.handle.id == rhs.handle.id;
}

/**
 * Compare entities
 */
constexpr bool operator!=(HgEntity lhs, HgEntity rhs)
{
    return lhs.handle.id != rhs.handle.id;
}

/**
 * Hashing for entities
 */
template<>
constexpr u64 hgHashImpl(HgEntity e)
{
    return hgHashImpl(e.handle.id);
}

/**
 * The unique component id for a type
 */
template<typename T>
inline u64 hgComponentId = (u64)-1;

/**
 * An entity Component System
 */
struct HgEcs;

/**
 * A system of components
 */
struct HgComponent {
    /**
     * The name of the component type
     */
    HgStringView name;
    /**
     * The component lookup from entity index
     */
    u32* indices;
    /**
     * The entity lookup from component index
     */
    HgEntity* entities;
    /**
     * The component data
     */
    void* components;
    /**
     * The width of each component in bytes
     */
    u32 width;
    /**
     * The number of components
     */
    u32 count;
    /**
     * The capacity of components
     */
    u32 capacity;
    /**
     * The function called on adding the component
     */
    void (*add)(void* component);
    /**
     * The function called on removing the component
     */
    void (*remove)(void* component);
    /**
     * The width of each component serialized in bytes
     */
    u32 serialWidth;
    /**
     * The function called on serializing the component
     *
     * Parameters
     * - stringArena The arena to allocate strings from, if not in the map
     * - entities The stored indices of entities
     * - strings The stored indices of strings
     * - srcComponent The component to serialize
     * - dstData Where to store the component data, may not be aligned
     */
    void (*serialize)(
        HgArena* stringArena,
        HgMap<HgEntity, u32>* entities,
        HgMap<HgStringView, u32>* strings,
        void* srcComponent,
        void* dstData);
    /**
     * The function called on deserializing the component
     *
     * Parameters
     * - entities The entities from stored indices
     * - strings The strings from stored indices
     * - srcData The component to deserialize, may not be aligned
     * - dstComponent Where to load the component
     */
    void (*deserialize)(
        HgEntity* entities,
        HgStringView* strings,
        void* srcData,
        void* dstComponent);
};

/**
 * An entity component system
 */
struct HgEcs {
    /**
     * The entity pool
     */
    HgPool entities;
    /**
     * The component systems
     */
    HgMap<u64, HgComponent> components;
};

/**
 * Create a new entity component system
 *
 * Parameters
 * - arena The arena to allocate from
 * - maxEntities The maximum number of entities which can be created
 * - maxComponentTypes The maximum number of types which can be created
 */
HgEcs hgEcsCreate(HgArena* arena, u32 maxEntities, u32 maxComponentTypes);

/**
 * Destroy all entities
 */
void hgEcsReset(HgEcs* ecs);

/**
 * The config to register a component
 */
struct HgEcsRegisterComponent {
    /**
     * The name of the copmonent to create, must be stable
     *
     * Note, the componentId is derived from this name
     */
    HgStringView name;
    /**
     * The alignment of the component data in bytes
     */
    u32 align;
    /**
     * The width of the component data in bytes
     */
    u32 width;
    /**
     * The max number of components of this type that will be added
     */
    u32 max;
    /**
     * The function called on adding the component
     */
    void (*add)(void* component);
    /**
     * The function called on removing the component
     */
    void (*remove)(void* component);
    /**
     * The width in bytes of the serialized component
     */
    u32 serialWidth;
    /**
     * The function called on serializing the component
     */
    void (*serialize)(
        HgArena* stringArena,
        HgMap<HgEntity, u32>* entities,
        HgMap<HgStringView, u32>* strings,
        void* srcComponent,
        void* dstData);
    /**
     * The function called on deserializing the component
     */
    void (*deserialize)(
        HgEntity* entities,
        HgStringView* strings,
        void* srcData,
        void* dstComponent);
};

/**
 * Create a new component type in the ECS, with componentId hgHashImpl(name)
 *
 * Parameters
 * - ecs The entity component system
 * - arena The arena to allocate from
 * - config The component config
 */
void hgEcsRegisterComponent(HgEcs* ecs, HgArena* arena, HgEcsRegisterComponent* config);

/**
 * The function called on adding the component
 */
template<typename T>
void hgEcsAddImpl(T* component)
{
    *component = {};
}

/**
 * The function called on removing the component
 */
template<typename T>
void hgEcsRemoveImpl(T* component)
{
    (void)component;
}

/**
 * The width in bytes of the serialized component
 */
template<typename T>
inline constexpr u32 hgEcsSerialWidthImpl = sizeof(T);

/**
 * The function called on serializing the component
 */
template<typename T>
void hgEcsSerializeImpl(
    HgArena* stringArena,
    HgMap<HgEntity, u32>* entities,
    HgMap<HgStringView, u32>* strings,
    T* srcComponent,
    void* dstData)
{
    (void)stringArena;
    (void)entities;
    (void)strings;
    memcpy(dstData, srcComponent, sizeof(T));
}

/**
 * The function called on deserializing the component
 */
template<typename T>
void hgEcsDeserializeImpl(
    HgEntity* entities,
    HgStringView* strings,
    void* srcData,
    T* dstComponent)
{
    (void)entities;
    (void)strings;
    memcpy(dstComponent, srcData, sizeof(T));
}

/**
 * Create a new component type in the Ecs from a type and its name
 */
#define hgEcsRegisterType(ecs, arena, type, maxCount) \
    do { \
        hgComponentId<type> = hgHashImpl(#type); \
        HgEcsRegisterComponent registerComponent{}; \
        registerComponent.name = #type; \
        registerComponent.align = alignof(type); \
        registerComponent.width = sizeof(type); \
        registerComponent.max = maxCount; \
        registerComponent.add = [](void* component) \
        { \
            hgEcsAddImpl<type>((type*)component); \
        }; \
        registerComponent.remove = [](void* component) \
        { \
            hgEcsRemoveImpl<type>((type*)component); \
        }; \
        registerComponent.serialWidth = hgEcsSerialWidthImpl<type>; \
        registerComponent.serialize = []( \
            HgArena* stringArena, \
            HgMap<HgEntity, u32>* entities, \
            HgMap<HgStringView, u32>* strings, \
            void* srcComponent, \
            void* dstData) \
        { \
            hgEcsSerializeImpl<type>( \
                stringArena, \
                entities, \
                strings, \
                (type*)srcComponent, \
                dstData); \
        }; \
        registerComponent.deserialize = []( \
            HgEntity* entities, \
            HgStringView* strings, \
            void* srcData, \
            void* dstComponent) \
        { \
            hgEcsDeserializeImpl<type>( \
                entities, \
                strings, \
                srcData, \
                (type*)dstComponent); \
        }; \
        hgEcsRegisterComponent(ecs, arena, &registerComponent); \
    } while (0)

/**
 * Returns the name of the component type
 */
HgStringView hgEcsComponentName(HgEcs* ecs, u64 componentId);

/**
 * Return a new entity
 */
HgEntity hgEcsSpawn(HgEcs* ecs);

/**
 * Destroy an entity
 *
 * Note, this function will invalidate iterators
 */
void hgEcsDespawn(HgEcs* ecs, HgEntity e);

/**
 * Return whether an entity is alive
 */
bool hgEcsAlive(HgEcs* ecs, HgEntity e);

/**
 * Add a component to an entity
 *
 * Note, the entity must not have a component of this type already
 *
 * Returns
 * - A pointer to the created component
 */
void* hgEcsAdd(HgEcs* ecs, HgEntity e, u64 componentId);

/**
 * Add a component to an entity
 *
 * Note, the entity must not have a component of this type already
 *
 * Returns
 * - A pointer to the created component
 */
template<typename T>
T* hgEcsAdd(HgEcs* ecs, HgEntity e)
{
    return (T*)hgEcsAdd(ecs, e, hgComponentId<T>);
}

/**
 * Remove a component from an entity
 *
 * Note, this function will invalidate iterators
 */
void hgEcsRemove(HgEcs* ecs, HgEntity e, u64 componentId);

/**
 * Remove a component from an entity
 *
 * Note, this function will invalidate iterators
 */
template<typename T>
void hgEcsRemove(HgEcs* ecs, HgEntity e)
{
    hgEcsRemove(ecs, e, hgComponentId<T>);
}

/**
 * Check whether an entity has a component or not
 */
bool hgEcsHas(HgEcs* ecs, HgEntity e, u64 componentId);

/**
 * Check whether an entity has a component or not
 */
template<typename T>
bool hgEcsHas(HgEcs* ecs, HgEntity e)
{
    return hgEcsHas(ecs, e, hgComponentId<T>);
}

/**
 * Return whether the entity has all given components
 */
template<typename... Ts>
bool hgEcsHasAll(HgEcs* ecs, HgEntity e)
{
    return (hgEcsHas<Ts>(ecs, e) && ...);
}

/**
 * Return whether the entity has any of the given components
 */
template<typename... Ts>
bool hgEcsHasAny(HgEcs* ecs, HgEntity e)
{
    return (hgEcsHas<Ts>(ecs, e) || ...);
}

/**
 * Get a pointer to the entity's component
 *
 * Note, the entity must have the component
 *
 * Returns
 * - A pointer to the entity's component, will never be nullptr
 */
void* hgEcsGet(HgEcs* ecs, HgEntity e, u64 componentId);

/**
 * Get a reference to the entity's component
 *
 * Note, the entity must have the component
 *
 * Returns
 * - A reference to the entity's component
 */
template<typename T>
T* hgEcsGet(HgEcs* ecs, HgEntity e)
{
    return (T*)hgEcsGet(ecs, e, hgComponentId<T>);
}

/**
 * Get the entity from it's component
 *
 * Parameters
 * - c The component to lookup, must be a valid component
 * - componentId The id of the component
 */
HgEntity hgEcsGetEntity(HgEcs* ecs, const void* c, u64 componentId);

/**
 * Get the entity from it's component
 *
 * Parameters
 * - c The component to lookup, must be a valid component
 */
template<typename T>
HgEntity hgEcsGetEntity(HgEcs* ecs, const T* c)
{
    return hgEcsGetEntity(ecs, (void*)c, hgComponentId<T>);
}

/**
 * Return a pointer to all entities of type
 */
HgEntity* hgEcsEntities(HgEcs* ecs, u64 componentId);

/**
 * Return a pointer to all entities of type
 */
template<typename T>
HgEntity* hgEcsEntities(HgEcs* ecs)
{
    return hgEcsEntities(ecs, hgComponentId<T>);
}

/**
 * Return a pointer to all components of type
 */
void* hgEcsComponents(HgEcs* ecs, u64 componentId);

/**
 * Return a pointer to all components of type
 */
template<typename T>
T* hgEcsComponents(HgEcs* ecs)
{
    return (T*)hgEcsComponents(ecs, hgComponentId<T>);
}

/**
 * Return the number of active components of a type
 */
u32 hgEcsCount(HgEcs* ecs, u64 componentId);

/**
 * Return the number of active components of a type
 */
template<typename T>
u32 hgEcsCount(HgEcs* ecs)
{
    return hgEcsCount(ecs, hgComponentId<T>);
}

/**
 * Find the id of the system with the fewest elements
 */
u64 hgEcsFindSmallest(HgEcs* ecs, u64* ids, u32 idCount);

/**
 * Find the id of the system with the fewest elements
 */
template<typename... Ts>
u64 hgEcsFindSmallest(HgEcs* ecs)
{
    u32 index = 0;
    u64 ids[sizeof...(Ts)];
    ((ids[index++] = hgComponentId<Ts>), ...);
    return hgEcsFindSmallest(ecs, ids, sizeof...(Ts));
}

/**
 * Iterate over all entities with the single given component
 *
 * Note, specifying only one component allows deterministic ordering (such
 * as in the case of sorting), as well as extra optimization
 *
 * The function receives as parameters:
 * - The entity id
 * - A reference to the component
 *
 * Parameters
 * - function The function to call
 */
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

/**
 * Iterate over all entities with the given components
 *
 * The function receives as parameters:
 * - The entity id
 * - A reference to each component...
 *
 * Parameters
 * - function The function to call
 */
template<typename... Ts, typename Fn>
void hgEcsForEachMulti(HgEcs* ecs, Fn& fn)
{
    static_assert(std::is_invocable_r_v<void, Fn, HgEntity, Ts*...>);

    u64 id = hgEcsFindSmallest<Ts...>(ecs);
    HgComponent* system = hgMapGet(&ecs->components, id);
    hgAssert(system != nullptr);

    HgEntity* e = system->entities;
    HgEntity* end = e + system->count;
    for (; e != end; ++e)
    {
        if (hgEcsHasAll<Ts...>(ecs, *e))
            fn(*e, hgEcsGet<Ts>(ecs, *e)...);
    }
}

/**
 * Iterate over all entities with the given components
 *
 * Note, calls the single or multi version from the number of components
 *
 * Parameters
 * - function The function to call
 */
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

/**
 * Iterate over all entities with the single given component
 *
 * Note, specifying only one component allows deterministic ordering (such
 * as in the case of sorting), as well as extra optimization
 *
 * The function receives as parameters:
 * - The entity id
 * - A reference to the component
 *
 * Parameters
 * - fn The function to call
 */
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

/**
 * Iterate over all entities with the given components
 *
 * The function receives as parameters:
 * - The entity id
 * - A reference to each component...
 *
 * Parameters
 * - fn The function to call
 */
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

    hgThreadsFor(0, system->count, &capture, [](void* pcapture, u64 idx)
    {
        Capture* capture = (Capture*)pcapture;
        HgEntity e = capture->system->entities[idx];
        if (hgEcsHasAll<Ts...>(capture->ecs, e))
            (*capture->fn)(e, hgEcsGet<Ts>(capture->ecs, e)...);
    });
}

/**
 * Iterate over all entities with the given components
 *
 * Note, calls the single of multi version from the number of components
 *
 * The function receives as parameters:
 * - The entity id
 * - A reference to each component...
 *
 * Parameters
 * - chunkSize The number of executions per group
 * - fn The function to call
 */
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

/**
 * Sort components
 *
 * Parameters
 * - componentId The component system to sort
 * - data The data passed to compare
 * - compare The comparison function
 */
void hgEcsSort(HgEcs* ecs, u64 componentId, void* data, bool (*compare)(void*, HgEcs* ecs, HgEntity lhs, HgEntity rhs));

/**
 * Sort components
 *
 * Parameters
 * - data The data passed to compare
 * - compare The comparison function
 */
template<typename T>
void hgEcsSort(HgEcs* ecs, void* data, bool (*compare)(void*, HgEcs* ecs, HgEntity lhs, HgEntity rhs))
{
    hgEcsSort(ecs, hgComponentId<T>, data, compare);
}

/**
 * Serialize a scene from a root entity
 */
HgBinary hgEcsSerialize(HgArena* arena, HgEcs* ecs, HgEntity root);

/**
 * Deserialize a scene and return the root entity
 */
HgEntity hgEcsDeserialize(HgEcs* ecs, HgBinary scene);

/**
 * A node component for entities in a hierarchy
 */
struct HgNode {
    /**
     * The entity's parent, if any
     */
    HgEntity parent{};
    /**
     * The next child of this entity's parent
     */
    HgEntity nextSibling{};
    /**
     * The previous child of this entity's parent
     */
    HgEntity prevSibling{};
    /**
     * The first of this entity's children, forming a linked list
     */
    HgEntity firstChild{};
};

/**
 * The serialized form for HgNode
 */
struct HgNodeSerial {
    u32 parent;
    u32 nextSibling;
    u32 prevSibling;
    u32 firstChild;
};

/**
 * HgNode serialize width
 */
template<>
inline constexpr u32 hgEcsSerialWidthImpl<HgNode> = sizeof(HgNodeSerial);

/**
 * HgNode serialize implementation
 */
template<>
void hgEcsSerializeImpl(
    HgArena* stringArena,
    HgMap<HgEntity, u32>* entities,
    HgMap<HgStringView, u32>* strings,
    HgNode* srcComponent,
    void* dstData);

/**
 * HgNode deserialize implementation
 */
template<>
void hgEcsDeserializeImpl(
    HgEntity* entities,
    HgStringView* strings,
    void* srcData,
    HgNode* dstComponent);

/**
 * Add a new child to an entity in a hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - parent The parent to add to, must be alive
 * - child The child to add, must be alive
 */
void hgNodeAddChild(HgEcs* ecs, HgEntity parent, HgEntity child);

/**
 * Remove the entity from its hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to detach, must be alive
 */
void hgNodeDetach(HgEcs* ecs, HgEntity e);

/**
 * Destroy the entity and all its children in a hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to destroy to, must be alive
 */
void hgNodeDestroy(HgEcs* ecs, HgEntity e);

/**
 * The transform component for entities in absolute space
 */
struct HgTransform {
    /**
     * The entity's transform model matrix in world space
     */
    HgMat4 mat{1.0f};
    /**
     * The entity's position relative to its parent
     * - x: -left, +right
     * - y: -up, +down
     * - z: -backward, +forward
     */
    HgVec3 position{0.0f, 0.0f, 0.0f};
    /**
     * The entity's scaling relative to its parent
     * - x: horizonatal
     * - y: vertical
     * - z: depth
     */
    HgVec3 scale{1.0f, 1.0f, 1.0f};
    /**
     * The entity's rotation relative to its parent
     */
    HgQuat rotation{1.0f, 0.0f, 0.0f, 0.0f};
};

/**
 * The serialized form for HgTransform
 */
struct HgTransformSerial {
    HgVec3 position;
    HgVec3 scale;
    HgQuat rotation;
};

/**
 * HgTransform serialize width
 */
template<>
inline constexpr u32 hgEcsSerialWidthImpl<HgTransform> = sizeof(HgTransformSerial);

/**
 * HgTransform serialize implementation
 */
template<>
void hgEcsSerializeImpl(
    HgArena* stringArena,
    HgMap<HgEntity, u32>* entities,
    HgMap<HgStringView, u32>* strings,
    HgTransform* srcComponent,
    void* dstData);

/**
 * HgTransform deserialize implementation
 */
template<>
void hgEcsDeserializeImpl(
    HgEntity* entities,
    HgStringView* strings,
    void* srcData,
    HgTransform* dstComponent);

/**
 * Get the position from HgTransform mat
 */
constexpr HgVec3 hgTransformWorldPos(HgTransform& tf)
{
    return HgVec3{tf.mat.w};
}

/**
 * Update HgTransform for the entity and its children to the HgTransformLocal
 */
void hgTransformUpdate(HgEcs* ecs, HgEntity e);

#endif // HG_ECS_HPP
