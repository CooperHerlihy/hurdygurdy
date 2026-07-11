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
struct Entity {
    /**
     * The entity handle
     */
    Handle handle;
};

/**
 */
static constexpr Entity entityNull = Entity{};

/**
 * Compare entities
 */
constexpr bool operator==(Entity lhs, Entity rhs)
{
    return lhs.handle.id == rhs.handle.id;
}

/**
 * Compare entities
 */
constexpr bool operator!=(Entity lhs, Entity rhs)
{
    return lhs.handle.id != rhs.handle.id;
}

/**
 * Hashing for entities
 */
template<>
constexpr u64 hash(Entity e)
{
    return hash(e.handle.id);
}

/**
 * The unique component id for a type
 */
template<typename T>
inline u64 componentId = (u64)-1;

/**
 * The function called on removing the component, may be overridden
 */
template<typename T>
void ecsDtor(T* component)
{
    (void)component;
}

/**
 * The serializer for an ecs
 */
union EntitySerializer {
    /**
     * The indices of entities
     */
    u32* entityToIdx;
    /**
     * The entities by index
     */
    Entity* idxToEntity;
};

/**
 * The default serialization for a component, should be overridden
 */
template<typename T>
void ecsSerialize(Serializer* s, T* val, EntitySerializer* entities)
{
    serialize(s, val);
    (void)entities;
}

/**
 * Entity ecs serialization
 */
void entitySerialize(Serializer* s, Entity* val, EntitySerializer* ecs);

/**
 * A system of components
 */
struct Component {
    /**
     * The name of the component type
     */
    String name;
    /**
     * The component lookup from entity index
     */
    Array<u32> indices;
    /**
     * The entity lookup from component index
     */
    Array<Entity> entities;
    /**
     * The component data
     */
    ArrayAny components;
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
    void (*serialize)(Serializer* s, void* val, EntitySerializer* ecs);
};

/**
 * An entity component system
 */
struct Ecs {
    /**
     * The entity pool
     */
    HandlePool entities;
    /**
     * The component systems
     */
    Map<u64, Component> components;
};

/**
 * Create a new entity component system
 */
Ecs ecsCreate();

/**
 * Destroy an entity component system
 */
void ecsDestroy(Ecs* ecs);

/**
 * Destroy all entities, leave components registered
 */
void ecsReset(Ecs* ecs);

/**
 * Ecs serialization
 */
template<>
void serialize(Serializer* s, Ecs* ecs);

/**
 * The config to register a component
 */
struct EcsRegisterComponent {
    /**
     * The name of the component to create
     *
     * Note, the componentId is derived from this name
     */
    String name;
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
    void (*serialize)(Serializer* s, void* val, EntitySerializer* ecs);
};

/**
 * Register a new component type
 */
void ecsRegisterComponent(Ecs* ecs, EcsRegisterComponent* config);

/**
 * Register a new component type, creating the name from the type
 */
#define HG_ECS_REGISTER_TYPE(ecs, T) \
    do { \
        componentId<T> = hash(#T); \
        EcsRegisterComponent registerComponent_##T{}; \
        registerComponent_##T.name = #T; \
        registerComponent_##T.width = sizeof(T); \
        registerComponent_##T.align = alignof(T); \
        registerComponent_##T.dtor = [](void* component) \
        { \
            ecsDtor<T>((T*)component); \
        }; \
        registerComponent_##T.serialize = []( \
            Serializer* s, \
            void* val, \
            EntitySerializer* entities) \
        { \
            ecsSerialize<T>(s, (T*)val, entities); \
        }; \
        ecsRegisterComponent(ecs, &registerComponent_##T); \
    } while (0)

/**
 * Unregister a component
 */
void ecsUnregisterComponent(Ecs* ecs, u64 componentId);

/**
 * Unregister a component
 */
template<typename T>
void ecsUnregisterComponent(Ecs* ecs)
{
    ecsUnregisterComponent(ecs, componentId<T>);
}

/**
 * Returns the name of the component type
 */
String ecsComponentName(Ecs* ecs, u64 componentId);

/**
 * Return a new entity
 */
Entity ecsSpawn(Ecs* ecs);

/**
 * Destroy an entity
 *
 * Note, this function will invalidate iterators
 */
void ecsDespawn(Ecs* ecs, Entity e);

/**
 * Return whether an entity is alive
 */
bool ecsAlive(Ecs* ecs, Entity e);

/**
 * Add a component to an entity
 *
 * Note, the entity must not have a component of this type already
 *
 * Returns
 * - A pointer to the created component
 */
void* ecsAdd(Ecs* ecs, Entity e, u64 componentId);

/**
 * Add a component to an entity
 *
 * Note, the entity must not have a component of this type already
 *
 * Returns
 * - A pointer to the created component
 */
template<typename T>
T* ecsAdd(Ecs* ecs, Entity e)
{
    return (T*)ecsAdd(ecs, e, componentId<T>);
}

/**
 * Remove a component from an entity
 *
 * Note, this function will invalidate iterators
 */
void ecsRemove(Ecs* ecs, Entity e, u64 componentId);

/**
 * Remove a component from an entity
 *
 * Note, this function will invalidate iterators
 */
template<typename T>
void ecsRemove(Ecs* ecs, Entity e)
{
    ecsRemove(ecs, e, componentId<T>);
}

/**
 * Check whether an entity has a component or not
 */
bool ecsHas(Ecs* ecs, Entity e, u64 componentId);

/**
 * Check whether an entity has a component or not
 */
template<typename T>
bool ecsHas(Ecs* ecs, Entity e)
{
    return ecsHas(ecs, e, componentId<T>);
}

/**
 * Return whether the entity has all given components
 */
template<typename... Ts>
bool ecsHasAll(Ecs* ecs, Entity e)
{
    return (ecsHas<Ts>(ecs, e) && ...);
}

/**
 * Return whether the entity has any of the given components
 */
template<typename... Ts>
bool ecsHasAny(Ecs* ecs, Entity e)
{
    return (ecsHas<Ts>(ecs, e) || ...);
}

/**
 * Get a pointer to the entity's component
 *
 * Note, the entity must have the component
 *
 * Returns
 * - A pointer to the entity's component, will never be nullptr
 */
void* ecsGet(Ecs* ecs, Entity e, u64 componentId);

/**
 * Get a reference to the entity's component
 *
 * Note, the entity must have the component
 *
 * Returns
 * - A reference to the entity's component
 */
template<typename T>
T* ecsGet(Ecs* ecs, Entity e)
{
    return (T*)ecsGet(ecs, e, componentId<T>);
}

/**
 * Get the entity from it's component
 *
 * Parameters
 * - c The component to lookup, must be a valid component
 * - componentId The id of the component
 */
Entity ecsGetEntity(Ecs* ecs, const void* c, u64 componentId);

/**
 * Get the entity from it's component
 *
 * Parameters
 * - c The component to lookup, must be a valid component
 */
template<typename T>
Entity ecsGetEntity(Ecs* ecs, const T* c)
{
    return ecsGetEntity(ecs, (void*)c, componentId<T>);
}

/**
 * Return a pointer to all entities of type
 */
Entity* ecsEntities(Ecs* ecs, u64 componentId);

/**
 * Return a pointer to all entities of type
 */
template<typename T>
Entity* ecsEntities(Ecs* ecs)
{
    return ecsEntities(ecs, componentId<T>);
}

/**
 * Return a pointer to all components of type
 */
void* ecsComponents(Ecs* ecs, u64 componentId);

/**
 * Return a pointer to all components of type
 */
template<typename T>
T* ecsComponents(Ecs* ecs)
{
    return (T*)ecsComponents(ecs, componentId<T>);
}

/**
 * Return the number of active components of a type
 */
u32 ecsCount(Ecs* ecs, u64 componentId);

/**
 * Return the number of active components of a type
 */
template<typename T>
u32 ecsCount(Ecs* ecs)
{
    return ecsCount(ecs, componentId<T>);
}

/**
 * Find the id of the system with the fewest elements
 */
u64 ecsFindSmallest(Ecs* ecs, u64* ids, u32 idCount);

/**
 * Find the id of the system with the fewest elements
 */
template<typename... Ts>
u64 ecsFindSmallest(Ecs* ecs)
{
    u32 index = 0;
    u64 ids[sizeof...(Ts)];
    ((ids[index++] = componentId<Ts>), ...);
    return ecsFindSmallest(ecs, ids, sizeof...(Ts));
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
void ecsForEach(Ecs* ecs, Fn fn);

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
void ecsForPar(Ecs* ecs, Fn fn);

/**
 * Sort components
 *
 * Parameters
 * - componentId The component system to sort
 * - data The data passed to compare
 * - compare The comparison function
 */
void ecsSort(Ecs* ecs, u64 componentId, void* data, bool (*compare)(void*, Ecs* ecs, Entity lhs, Entity rhs));

/**
 * Sort components
 *
 * Parameters
 * - data The data passed to compare
 * - compare The comparison function
 */
template<typename T>
void ecsSort(Ecs* ecs, void* data, bool (*compare)(void*, Ecs* ecs, Entity lhs, Entity rhs))
{
    ecsSort(ecs, componentId<T>, data, compare);
}

/**
 * A node component for entities in a hierarchy
 */
struct Node {
    /**
     * The entity's parent, if any
     */
    Entity parent{};
    /**
     * The next child of this entity's parent
     */
    Entity nextSibling{};
    /**
     * The previous child of this entity's parent
     */
    Entity prevSibling{};
    /**
     * The first of this entity's children, forming a linked list
     */
    Entity firstChild{};
};

/**
 * Node serialization implementation
 */
template<>
void ecsSerialize(Serializer* s, Node* node, EntitySerializer* ecs);

/**
 * Add an empty node to an entity
 */
Node* nodeAdd(Ecs* ecs, Entity e);

/**
 * Remove the entity from its hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to detach, must be alive
 */
void nodeDetach(Ecs* ecs, Entity e);

/**
 * Destroy the entity and all its children in a hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to destroy to, must be alive
 */
void nodeDestroy(Ecs* ecs, Entity e);

/**
 * Add a new child to an entity in a hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - parent The parent to add to, must be alive
 * - child The child to add, must be alive
 */
void nodeAddChild(Ecs* ecs, Entity parent, Entity child);

/**
 * The transform component for entities in absolute space
 */
struct Transform {
    /**
     * The entity's transform model matrix in world space
     */
    Mat4 mat{1.0f};
    /**
     * The entity's position relative to its parent
     * - x: -left, +right
     * - y: -up, +down
     * - z: -backward, +forward
     */
    Vec3 position{0.0f, 0.0f, 0.0f};
    /**
     * The entity's scaling relative to its parent
     * - x: horizonatal
     * - y: vertical
     * - z: depth
     */
    Vec3 scale{1.0f, 1.0f, 1.0f};
    /**
     * The entity's rotation relative to its parent
     */
    Quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
};

/**
 * Transform serialization impl
 */
template<>
void serialize(Serializer* s, Transform* node);

/**
 * Add an identity transform to an entity
 */
Transform* transformAdd(
    Ecs* ecs,
    Entity e,
    Vec3 position = Vec3{0.0f},
    Vec3 scale = Vec3{1.0f},
    Quat rotation = Quat{1.0f});

/**
 * Get the position from Transform mat
 */
constexpr Vec3 transformWorldPos(Transform& tf)
{
    return Vec3{tf.mat.w};
}

/**
 * Update Transform for the entity and its children to the TransformLocal
 */
void transformUpdate(Ecs* ecs, Entity e);
} // namespace hg

#endif // ECS_HPP
