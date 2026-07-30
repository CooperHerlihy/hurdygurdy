#pragma once

#include "hg/utility.hpp"
#include "hg/concurrency.hpp"
#include "hg/product.hpp"
#include "hg/pool.hpp"
// #include "hg/serialization.hpp"

namespace hg {

/**
 * An entity in the ecs
 */
struct Entity {
    /**
     * The entity handle
     */
    Handle handle = nullHandle;
};

/**
 * The null entity
 */
static constexpr Entity nullEntity = Entity{};

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
 * A component in an entity component system
 */
template<typename T>
struct EcsComponent {
    /**
     * indices[e.handle.idx()] is the index into components, or -1 if none
     */
    Array<u32> indices{};
    /**
     * entities[idx] is the entity that owns that index
     */
    Array<Entity> entities{};
    /**
     * The component data
     */
    Array<T> components{};

    /**
     * Default-construct a component on an entity
     *
     * Note, the entity must not already have the component
     */
    T& add(Entity e)
    {
        HG_ASSERT(!has(e));

        if (e.handle.idx() >= indices.count)
        {
            u64 oldCount = indices.count;
            indices.resize(e.handle.idx() + 1);
            for (u64 i = oldCount; i < indices.count; ++i)
            {
                indices[i] = (u32)-1;
            }
        }

        u64 idx = entities.count;
        indices[e.handle.idx()] = (u32)idx;
        entities.push(e);
        return components.push();
    }

    /**
     * Copy-construct a component on an entity
     *
     * Note, the entity must not already have the component
     */
    T& add(Entity e, const T& val)
    {
        HG_ASSERT(!has(e));

        if (e.handle.idx() >= indices.count)
        {
            u64 oldCount = indices.count;
            indices.resize(e.handle.idx() + 1);
            for (u64 i = oldCount; i < indices.count; ++i)
            {
                indices[i] = (u32)-1;
            }
        }

        u64 idx = entities.count;
        indices[e.handle.idx()] = (u32)idx;
        entities.push(e);
        return components.push(val);
    }

    /**
     * Move-construct a component on an entity
     *
     * Note, the entity must not already have the component
     */
    T& add(Entity e, T&& val)
    {
        HG_ASSERT(!has(e));

        if (e.handle.idx() >= indices.count)
        {
            u64 oldCount = indices.count;
            indices.resize(e.handle.idx() + 1);
            for (u64 i = oldCount; i < indices.count; ++i)
            {
                indices[i] = (u32)-1;
            }
        }

        u64 idx = entities.count;
        indices[e.handle.idx()] = (u32)idx;
        entities.push(e);
        return components.push(std::move(val));
    }

    /**
     * Remove a component from an entity
     *
     * Note, the entity must have the component to remove
     */
    void remove(Entity e)
    {
        u32 idx = indices[e.handle.idx()];
        indices[e.handle.idx()] = (u32)-1;
        entities.removeSwap(idx);
        components.removeSwap(idx);

        if (idx < entities.count)
        {
            Entity moved = entities[idx];
            indices[moved.handle.idx()] = idx;
        }
    }

    /**
     * Returns whether an entity has a component
     */
    bool has(Entity e) const
    {
        return e.handle.idx() < indices.count && indices[e.handle.idx()] != (u32)-1;
    }

    /**
     * Get a component from an entity
     *
     * Note, the entity must have the component
     */
    T& get(Entity e)
    {
        HG_ASSERT(has(e));
        return components[indices[e.handle.idx()]];
    }

    /**
     * Get a component from an entity (const)
     *
     * Note, the entity must have the component
     */
    const T& get(Entity e) const
    {
        HG_ASSERT(has(e));
        return components[indices[e.handle.idx()]];
    }

    /**
     * Get the entity associated with a component
     */
    Entity getEntity(const T& c) const
    {
        return entities[static_cast<u32>(&c - components.vals)];
    }
};

/**
 * An entity component system
 */
template<typename... Ts>
struct Ecs {
    /**
     * The entity pool
     */
    HandlePool entities;
    /**
     * The component systems
     */
    Product<EcsComponent<Ts>...> systems{};

    /**
     * Create an empty entity component system
     */
    static Ecs create()
    {
        Ecs ecs{};
        ecs.entities = HandlePool::create();
        return ecs;
    }

    /**
     * Get the component system for a type
     */
    template<typename T>
    EcsComponent<T>& getComponentSystem()
    {
        return systems.template get<idxOf<T, Ts...>()>();
    }

    /**
     * Get the component system for a type (const)
     */
    template<typename T>
    const EcsComponent<T>& getComponentSystem() const
    {
        return systems.template get<idxOf<T, Ts...>()>();
    }

    /**
     * Despawn all entities
     */
    void reset()
    {
        (clear<Ts>(), ...);
        entities.reset();
    }

    /**
     * Remove component type from all entities
     */
    template<typename T>
    void clear()
    {
        EcsComponent<T>& s = getComponentSystem<T>();
        s.indices.reset();
        s.entities.reset();
        s.components.reset();
    }

    /**
     * Spawn a new entity with no components
     */
    Entity spawn()
    {
        return {entities.alloc()};
    }

    /**
     * Despawn an entity, destroying all components
     */
    void despawn(Entity e)
    {
        HG_ASSERT(alive(e));
        systems.forEach([&](auto& c)
        {
            if (c.has(e))
                c.remove(e);
        });
        entities.free(e.handle);
    }

    /**
     * Returns whether an entity is still alive
     */
    bool alive(Entity e) const
    {
        return entities.alive(e.handle);
    }

    /**
     * Default-construct a component on an entity
     *
     * Note, the entity must not already have the component
     */
    template<typename T>
    T& add(Entity e)
    {
        HG_ASSERT(alive(e));
        return getComponentSystem<T>().add(e);
    }

    /**
     * Copy-construct a component on an entity
     *
     * Note, the entity must not already have the component
     */
    template<typename T>
    T& add(Entity e, const T& val)
    {
        HG_ASSERT(alive(e));
        return getComponentSystem<T>().add(e, val);
    }

    /**
     * Move-construct a component on an entity
     *
     * Note, the entity must not already have the component
     */
    template<typename T>
    T& add(Entity e, T&& val)
    {
        HG_ASSERT(alive(e));
        return getComponentSystem<T>().add(e, std::move(val));
    }

    /**
     * Remove a component from an entity
     *
     * Note, the entity must have the component to remove
     */
    template<typename T>
    void remove(Entity e)
    {
        HG_ASSERT(alive(e));
        getComponentSystem<T>().remove(e);
    }

    /**
     * Default-construct components for multiple types at once
     *
     * Note, the entity must not already have any of the components
     */
    template<typename... Us> requires (sizeof...(Us) > 1)
    void add(Entity e)
    {
        HG_ASSERT(alive(e));
        (getComponentSystem<Us>().add(e), ...);
    }

    /**
     * Forward-construct components for multiple types at once
     *
     * Each argument corresponds positionally to a component type.
     * Note, the entity must not already have any of the components
     */
    template<typename... Us> requires (sizeof...(Us) > 1)
    void add(Entity e, Us&&... vals)
    {
        HG_ASSERT(alive(e));
        (getComponentSystem<Us>().add(e, std::forward<Us>(vals)), ...);
    }

    /**
     * Remove components for multiple types at once
     *
     * Note, the entity must have all the components to remove
     */
    template<typename... Us> requires (sizeof...(Us) > 1)
    void remove(Entity e)
    {
        HG_ASSERT(alive(e));
        (getComponentSystem<Us>().remove(e), ...);
    }

    /**
     * Returns whether an entity has a component
     */
    template<typename T>
    bool has(Entity e) const
    {
        HG_ASSERT(alive(e));
        return getComponentSystem<T>().has(e);
    }

    /**
     * Returns whether an entity has all components in a type list
     */
    template<typename... Us>
    bool hasAll(Entity e) const
    {
        return (has<Us>(e) && ...);
    }

    /**
     * Returns whether an entity has any component in a type list
     */
    template<typename... Us>
    bool hasAny(Entity e) const
    {
        return (has<Us>(e) || ...);
    }

    /**
     * Get a component from an entity
     *
     * Note, the entity must have the component
     */
    template<typename T>
    T& get(Entity e)
    {
        HG_ASSERT(alive(e));
        return getComponentSystem<T>().get(e);
    }

    /**
     * Get a component from an entity (const)
     *
     * Note, the entity must have the component
     */
    template<typename T>
    const T& get(Entity e) const
    {
        HG_ASSERT(alive(e));
        return getComponentSystem<T>().get(e);
    }

    /**
     * Get the entity associated with a component
     */
    template<typename T>
    Entity getEntity(const T& c) const
    {
        return getComponentSystem<T>().getEntity(c);
    }

    /**
     * Returns the number of active components of a type
     */
    template<typename T>
    u64 count() const
    {
        return getComponentSystem<T>().components.count;
    }

    /**
     * Returns all entities with a component type
     */
    template<typename T>
    Span<const Entity> getEntities() const
    {
        return getComponentSystem<T>().entities;
    }

    /**
     * Returns all components of a type
     */
    template<typename T>
    Span<T> getComponents()
    {
        return getComponentSystem<T>().components;
    }

    /**
     * Returns all components of a type (const)
     */
    template<typename T>
    Span<const T> getComponents() const
    {
        return getComponentSystem<T>().components;
    }

    /**
     * Calls a function for each entity with a component type
     */
    template<typename F, typename T>
    void forEach(F fn)
    {
        EcsComponent<T>& s = getComponentSystem<T>();

        if constexpr (std::is_invocable_r_v<void, F, Entity>)
        {
            for (Entity e : s.entities)
            {
                fn(e);
            }
        }
        else if constexpr (std::is_invocable_r_v<void, F, T&>)
        {
            for (T& c : s.components)
            {
                fn(c);
            }
        }
        else if constexpr (std::is_invocable_r_v<void, F, Entity, T&>)
        {
            Entity* e = s.entities.vals;
            T* c = s.components.vals;
            T* end = c + s.components.count;
            for (; c != end; ++c, ++e)
            {
                fn(*e, *c);
            }
        }
        else
        {
            static_assert(false, "Invalid lambda in Ecs forEach()");
        }
    }

    /**
     * Calls a function for each entity with a component type (const)
     */
    template<typename F, typename T>
    void forEach(F fn) const
    {
        const EcsComponent<T>& s = getComponentSystem<T>();

        if constexpr (std::is_invocable_r_v<void, F, Entity>)
        {
            for (const Entity& e : s.entities)
                fn(e);
        }
        else if constexpr (std::is_invocable_r_v<void, F, const T&>)
        {
            for (const T& c : s.components)
                fn(c);
        }
        else if constexpr (std::is_invocable_r_v<void, F, Entity, const T&>)
        {
            const Entity* e = s.entities.vals;
            const T* c = s.components.vals;
            const T* end = c + s.components.count;
            for (; c != end; ++c, ++e)
                fn(*e, *c);
        }
        else
        {
            static_assert(false, "Invalid lambda in Ecs forEach() const");
        }
    }

    /**
     * Calls a function in parallel for each entity with a component type
     */
    template<typename F, typename T>
    void forEachPar(F fn)
    {
        EcsComponent<T>& s = getComponentSystem<T>();

        if constexpr (std::is_invocable_r_v<void, F, Entity>)
        {
            Entity* e = s.entities.vals;
            forPar(0, s.components.count, [&](u64 idx)
            {
                fn(e[idx]);
            });
        }
        else if constexpr (std::is_invocable_r_v<void, F, T&>)
        {
            T* c = s.components.vals;
            forPar(0, s.components.count, [&](u64 idx)
            {
                fn(c[idx]);
            });
        }
        else if constexpr (std::is_invocable_r_v<void, F, Entity, T&>)
        {
            Entity* e = s.entities.vals;
            T* c = s.components.vals;
            forPar(0, s.components.count, [&](u64 idx)
            {
                fn(e[idx], c[idx]);
            });
        }
    }

    /**
     * Returns the smallest list of entities with at least one of the types
     */
    template<typename... Us>
    Span<Entity> getSmallestEntities()
    {
        Span<Entity> ret{nullptr, (u64)-1};
        ([&]()
        {
            Span<Entity> es = getComponentSystem<Us>().entities;
            if (es.count < ret.count)
                ret = es;
        }(), ...);
        return ret;
    }

    /**
     * Returns the smallest list of entities with at least one of the types (const)
     */
    template<typename... Us>
    Span<const Entity> getSmallestEntities() const
    {
        Span<const Entity> ret{nullptr, (u64)-1};
        ([&]()
        {
            Span<const Entity> es = getComponentSystem<Us>().entities;
            if (es.count < ret.count)
                ret = es;
        }(), ...);
        return ret;
    }

    /**
     * Calls a function for each entity with a list of components
     */
    template<typename F, typename... Us> requires (sizeof...(Us) > 1)
    void forEach(F fn)
    {
        for (Entity e : getSmallestEntities<Us...>())
        {
            if (hasAll<Us...>(e))
            {
                if constexpr (std::is_invocable_r_v<void, F, Entity>)
                    fn(e);
                else if constexpr (std::is_invocable_r_v<void, F, decltype(get<Us>(e))...>)
                    fn(get<Us>(e)...);
                else
                    fn(e, get<Us>(e)...);
            }
        }
    }

    /**
     * Calls a function for each entity with a list of components (const)
     */
    template<typename F, typename... Us> requires (sizeof...(Us) > 1)
    void forEach(F fn) const
    {
        for (Entity e : getSmallestEntities<Us...>())
        {
            if (hasAll<Us...>(e))
            {
                if constexpr (std::is_invocable_r_v<void, F, Entity>)
                    fn(e);
                else if constexpr (std::is_invocable_r_v<void, F, decltype(get<Us>(e))...>)
                    fn(get<Us>(e)...);
                else
                    fn(e, get<Us>(e)...);
            }
        }
    }

    /**
     * Calls a function in parallel for each entity with a list of components
     */
    template<typename F, typename... Us> requires (sizeof...(Us) > 1)
    void forEachPar(F fn)
    {
        Span<const Entity> entrySpan = getSmallestEntities<Us...>();
        forPar(0, entrySpan.count, [&](u64 idx)
        {
            Entity e = entrySpan[idx];
            if (hasAll<Us...>(e))
            {
                if constexpr (std::is_invocable_r_v<void, F, Entity>)
                    fn(e);
                else if constexpr (std::is_invocable_r_v<void, F, decltype(get<Us>(e))...>)
                    fn(get<Us>(e)...);
                else
                    fn(e, get<Us>(e)...);
            }
        });
    }
};

// /**
//  * The serializer for an ecs
//  */
// union EntitySerializer {
//     /**
//      * The indices of entities, if writing
//      */
//     u32* entityToIdx;
//     /**
//      * The entities by index, if reading
//      */
//     Entity* idxToEntity;
//
//     constexpr u32 getIdx(Entity e)
//     {
//         return entityToIdx[e.handle.idx()];
//     }
//
//     constexpr Entity getEntity(u32 idx)
//     {
//         return idxToEntity[idx];
//     }
// };
//
// /**
//  * Ecs serialization
//  */
// template<typename... Ts>
// void serialize(Serializer* s, Ecs<Ts...>* ecs)
// {
// }
//
// /**
//  * The default serialization for a component, may be overridden
//  */
// template<typename T>
// void ecsSerialize(Serializer* s, T* val, EntitySerializer* ecs)
// {
//     serialize(s, val);
//     static_cast<void>(ecs);
// }
//
// /**
//  * Entity ecs serialization
//  */
// template<>
// inline void ecsSerialize(Serializer* s, Entity* e, EntitySerializer* ecs)
// {
//     if (s->writing)
//     {
//         u32 idx = ecs->getIdx(*e);
//         serializeObject(s, &idx);
//     }
//     else
//     {
//         u32 idx;
//         serializeObject(s, &idx);
//         *e = ecs->getEntity(idx);
//     }
// }

} // namespace hg
