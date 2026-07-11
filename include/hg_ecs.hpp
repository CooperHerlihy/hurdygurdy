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

#ifndef HG_ECS_HPP
#define HG_ECS_HPP

#include "hg_containers.hpp"
#include "hg_core.hpp"
#include "hg_math.hpp"
#include "hg_serialization.hpp"

namespace hg {

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
 */
static constexpr HgEntity hgEntityNull = HgEntity{};

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
constexpr u64 hgHash(HgEntity e)
{
    return hgHash(e.handle.id);
}

/**
 * The unique component id for a type
 */
template<typename T>
inline u64 hgComponentId = (u64)-1;

/**
 * The function called on removing the component, may be overridden
 */
template<typename T>
void hgEcsDtor(T* component)
{
    (void)component;
}

/**
 * The serializer for an ecs
 */
union HgEntitySerializer {
    /**
     * The indices of entities
     */
    u32* entityToIdx;
    /**
     * The entities by index
     */
    HgEntity* idxToEntity;
};

/**
 * The default serialization for a component, should be overridden
 */
template<typename T>
void hgEcsSerialize(HgSerializer* s, T* val, HgEntitySerializer* entities)
{
    hgSerialize(s, val);
    (void)entities;
}

/**
 * HgEntity ecs serialization
 */
void hgEntitySerialize(HgSerializer* s, HgEntity* val, HgEntitySerializer* ecs);

/**
 * A system of components
 */
struct HgComponent {
    /**
     * The name of the component type
     */
    HgString name;
    /**
     * The component lookup from entity index
     */
    HgArray<u32> indices;
    /**
     * The entity lookup from component index
     */
    HgArray<HgEntity> entities;
    /**
     * The component data
     */
    HgArrayAny components;
    /**
     * The function called on removing the component
     */
    void (*dtor)(void* component);
    /**
     * The function called on serializing the component
     *
     * Parameters
     * - s The serializer
     * - The value to serialize
     * - ecs The ecs serializer data, if needed
     */
    void (*serialize)(HgSerializer* s, void* val, HgEntitySerializer* ecs);
};

/**
 * An entity component system
 */
struct HgEcs {
    /**
     * The entity pool
     */
    HgHandlePool entities;
    /**
     * The component systems
     */
    HgMap<u64, HgComponent> components;
};

/**
 * Create a new entity component system
 */
HgEcs hgEcsCreate();

/**
 * Destroy an entity component system
 */
void hgEcsDestroy(HgEcs* ecs);

/**
 * Destroy all entities, leave components registered
 */
void hgEcsReset(HgEcs* ecs);

/**
 * HgEcs serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgEcs* ecs);

/**
 * The config to register a component
 */
struct HgEcsRegisterComponent {
    /**
     * The name of the component to create
     *
     * Note, the componentId is derived from this name
     */
    HgString name;
    /**
     * The width of the component data in bytes
     */
    u32 width;
    /**
     * The alignment of the component data in bytes
     */
    u32 align;
    /**
     * The function called on removing the component
     */
    void (*dtor)(void* component);
    /**
     * The function called on serializing the component
     */
    void (*serialize)(HgSerializer* s, void* val, HgEntitySerializer* ecs);
};

/**
 * Register a new component type
 */
void hgEcsRegisterComponent(HgEcs* ecs, HgEcsRegisterComponent* config);

/**
 * Register a new component type, creating the name from the type
 */
#define hgEcsRegisterType(ecs, T) \
    do { \
        hgComponentId<T> = hgHash(#T); \
        HgEcsRegisterComponent registerComponent_##T{}; \
        registerComponent_##T.name = #T; \
        registerComponent_##T.width = sizeof(T); \
        registerComponent_##T.align = alignof(T); \
        registerComponent_##T.dtor = [](void* component) \
        { \
            hgEcsDtor<T>((T*)component); \
        }; \
        registerComponent_##T.serialize = []( \
            HgSerializer* s, \
            void* val, \
            HgEntitySerializer* entities) \
        { \
            hgEcsSerialize<T>(s, (T*)val, entities); \
        }; \
        hgEcsRegisterComponent(ecs, &registerComponent_##T); \
    } while (0)

/**
 * Unregister a component
 */
void hgEcsUnregisterComponent(HgEcs* ecs, u64 componentId);

/**
 * Unregister a component
 */
template<typename T>
void hgEcsUnregisterComponent(HgEcs* ecs)
{
    hgEcsUnregisterComponent(ecs, hgComponentId<T>);
}

/**
 * Returns the name of the component type
 */
HgString hgEcsComponentName(HgEcs* ecs, u64 componentId);

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
 * Iterate over all entities with the given components
 *
 * Note, calls the single or multi version from the number of components
 *
 * Parameters
 * - function The function to call
 */
template<typename... Ts, typename Fn>
void hgEcsForEach(HgEcs* ecs, Fn fn);

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
void hgEcsForPar(HgEcs* ecs, Fn fn);

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
 * HgNode serialization implementation
 */
template<>
void hgEcsSerialize(HgSerializer* s, HgNode* node, HgEntitySerializer* ecs);

/**
 * Add an empty node to an entity
 */
HgNode* hgNodeAdd(HgEcs* ecs, HgEntity e);

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
 * Add a new child to an entity in a hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - parent The parent to add to, must be alive
 * - child The child to add, must be alive
 */
void hgNodeAddChild(HgEcs* ecs, HgEntity parent, HgEntity child);

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
 * HgTransform serialization impl
 */
template<>
void hgSerialize(HgSerializer* s, HgTransform* node);

/**
 * Add an identity transform to an entity
 */
HgTransform* hgTransformAdd(
    HgEcs* ecs,
    HgEntity e,
    HgVec3 position = HgVec3{0.0f},
    HgVec3 scale = HgVec3{1.0f},
    HgQuat rotation = HgQuat{1.0f});

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
} // namespace hg

#endif // HG_ECS_HPP
