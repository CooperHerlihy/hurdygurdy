#pragma once

#include "hg/utility.hpp"
#include "hg/memory.hpp"
#include "hg/concurrency.hpp"
#include "hg/serialization.hpp"

#include <span>
#include <tuple>
#include <vector>

namespace hg {

/**
 * An entity in the ecs
 */
struct Entity {
    /**
     * The entity handle
     */
    u32 handle = static_cast<u32>(-1);

    /**
     * The number of bits in a handle used for the index
     */
    static constexpr u32 idxBits = 24;

    /**
     * Get the index from a handle
     */
    constexpr u32 idx() const
    {
        return handle & ((u32{1} << idxBits) - u32{1});
    }

    /**
     * Get the generation from a handle
     */
    constexpr u32 generation() const
    {
        return handle & ~((u32{1} << idxBits) - u32{1});
    }

    /**
     * Returns a new handle at the same index
     */
    constexpr Entity nextGeneration() const
    {
        return {handle + (u32{1} << idxBits)};
    }
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
    return lhs.handle == rhs.handle;
}

/**
 * Compare entities
 */
constexpr bool operator!=(Entity lhs, Entity rhs)
{
    return lhs.handle != rhs.handle;
}

/**
 * A component in an entity component system
 */
template<typename T>
struct EcsComponent {
    /**
     * The component type
     */
    using Type = T;

    /**
     * indices[e.idx()] is the index into components, or -1 if none
     */
    std::vector<u32> indices{};
    /**
     * entities[idx] is the entity that owns that index
     */
    std::vector<Entity> entities{};
    /**
     * The component data
     */
    std::vector<T> components{};

    /**
     * Default-construct a component on an entity
     *
     * Note, the entity must not already have the component
     */
    T& add(Entity e)
    {
        HG_ASSERT(!has(e));

        if (e.idx() >= indices.size())
        {
            u64 oldCount = indices.size();
            indices.resize(e.idx() + 1);
            for (u64 i = oldCount; i < indices.size(); ++i)
            {
                indices[i] = static_cast<u32>(-1);
            }
        }

        u64 idx = entities.size();
        indices[e.idx()] = static_cast<u32>(idx);
        entities.emplace_back(e);
        return components.emplace_back();
    }

    /**
     * Copy-construct a component on an entity
     *
     * Note, the entity must not already have the component
     */
    T& add(Entity e, const T& val)
    {
        HG_ASSERT(!has(e));

        if (e.idx() >= indices.size())
        {
            u64 oldCount = indices.size();
            indices.resize(e.idx() + 1);
            for (u64 i = oldCount; i < indices.size(); ++i)
            {
                indices[i] = static_cast<u32>(-1);
            }
        }

        u64 idx = entities.size();
        indices[e.idx()] = static_cast<u32>(idx);
        entities.push_back(e);
        return components.emplace_back(val);
    }

    /**
     * Move-construct a component on an entity
     *
     * Note, the entity must not already have the component
     */
    T& add(Entity e, T&& val)
    {
        HG_ASSERT(!has(e));

        if (e.idx() >= indices.size())
        {
            u64 oldCount = indices.size();
            indices.resize(e.idx() + 1);
            for (u64 i = oldCount; i < indices.size(); ++i)
            {
                indices[i] = static_cast<u32>(-1);
            }
        }

        u64 idx = entities.size();
        indices[e.idx()] = static_cast<u32>(idx);
        entities.push_back(e);
        return components.emplace_back(std::move(val));
    }

    /**
     * Remove a component from an entity
     *
     * Note, the entity must have the component to remove
     */
    void remove(Entity e)
    {
        u32 idx = indices[e.idx()];
        indices[e.idx()] = static_cast<u32>(-1);
        entities[idx] = std::move(entities.back());
        components[idx] = std::move(components.back());
        entities.pop_back();
        components.pop_back();

        if (idx < entities.size())
        {
            Entity moved = entities[idx];
            indices[moved.idx()] = idx;
        }
    }

    /**
     * Returns whether an entity has a component
     */
    bool has(Entity e) const
    {
        return e.idx() < indices.size() && indices[e.idx()] != static_cast<u32>(-1);
    }

    /**
     * Get a component from an entity
     *
     * Note, the entity must have the component
     */
    T& get(Entity e)
    {
        HG_ASSERT(has(e));
        return components[indices[e.idx()]];
    }

    /**
     * Get a component from an entity (const)
     *
     * Note, the entity must have the component
     */
    const T& get(Entity e) const
    {
        HG_ASSERT(has(e));
        return components[indices[e.idx()]];
    }

    /**
     * Get the entity associated with a component
     */
    Entity getEntity(const T& c) const
    {
        return entities[static_cast<u32>(&c - components.data())];
    }
};

/**
 * An entity component system
 */
template<typename... Ts>
struct Ecs {
    /**
     * The component systems
     */
    std::tuple<EcsComponent<Ts>...> systems{};
    /**
     * The alive entities
     */
    std::vector<Entity> entities;
    /**
     * The freed entities
     */
    std::vector<Entity> freeEntities;
    /**
     * The next entity
     */
    Entity nextEntity{0};

    /**
     * Get the component system for a type
     */
    template<typename T>
    EcsComponent<T>& getComponentSystem()
    {
        return std::get<idxInPack<T, Ts...>()>(systems);
    }

    /**
     * Get the component system for a type (const)
     */
    template<typename T>
    const EcsComponent<T>& getComponentSystem() const
    {
        return std::get<idxInPack<T, Ts...>()>(systems);
    }

    /**
     * Despawn all entities
     */
    void reset()
    {
        (clear<Ts>(), ...);
        entities.clear();
        freeEntities.clear();
        nextEntity = Entity{0};
    }

    /**
     * Remove component type from all entities
     */
    template<typename T>
    void clear()
    {
        EcsComponent<T>& s = getComponentSystem<T>();
        s.indices.clear();
        s.entities.clear();
        s.components.clear();
    }

    /**
     * Spawn a new entity with no components
     */
    Entity spawn()
    {
        if (freeEntities.empty())
        {
            Entity e = nextEntity;
            nextEntity.handle++;
            entities.emplace_back(e);
            return e;
        }
        else
        {
            Entity e = freeEntities.back();
            freeEntities.pop_back();
            entities[e.idx()] = e;
            return e;
        }
    }

    /**
     * Despawn an entity, destroying all components
     */
    void despawn(Entity e)
    {
        HG_ASSERT(alive(e));

        std::apply([&](auto&... system)
        {
            ([&]()
            {
                if (system.has(e))
                    system.remove(e);
            }(), ... );
        },
        systems);
        entities[e.idx()] = {};
        freeEntities.push_back(e.nextGeneration());
    }

    /**
     * Check whether an entity is alive
     */
    bool alive(Entity e) const
    {
        return e.idx() < entities.size() && entities[e.idx()] == e;
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
        HG_ASSERT(alive(e));
        return (has<Us>(e) && ...);
    }

    /**
     * Returns whether an entity has any component in a type list
     */
    template<typename... Us>
    bool hasAny(Entity e) const
    {
        HG_ASSERT(alive(e));
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
        return getComponentSystem<T>().components.size();
    }

    /**
     * Returns all entities with a component type
     */
    template<typename T>
    std::span<const Entity> getEntities() const
    {
        return getComponentSystem<T>().entities;
    }

    /**
     * Returns all components of a type
     */
    template<typename T>
    std::span<T> getComponents()
    {
        return getComponentSystem<T>().components;
    }

    /**
     * Returns all components of a type (const)
     */
    template<typename T>
    std::span<const T> getComponents() const
    {
        return getComponentSystem<T>().components;
    }

    /**
     * Calls a function for each entity with a component type
     */
    template<typename T, typename F>
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
            for (u64 i = 0; i < s.components.size(); ++i)
            {
                fn(s.entities[i], s.components[i]);
            }
        }
        else
        {
            static_assert(sizeof(T) == 0, "Invalid lambda in Ecs forEach()");
        }
    }

    /**
     * Calls a function for each entity with a component type (const)
     */
    template<typename T, typename F>
    void forEach(F fn) const
    {
        const EcsComponent<T>& s = getComponentSystem<T>();

        if constexpr (std::is_invocable_r_v<void, F, Entity>)
        {
            for (const Entity& e : s.entities)
            {
                fn(e);
            }
        }
        else if constexpr (std::is_invocable_r_v<void, F, const T&>)
        {
            for (const T& c : s.components)
            {
                fn(c);
            }
        }
        else if constexpr (std::is_invocable_r_v<void, F, Entity, const T&>)
        {
            for (u64 i = 0; i < s.components.size(); ++i)
            {
                fn(s.entities[i], s.components[i]);
            }
        }
        else
        {
            static_assert(sizeof(T) == 0, "Invalid lambda in Ecs forEach() const");
        }
    }

    /**
     * Calls a function in parallel for each entity with a component type
     */
    template<typename T, typename F>
    void forEachPar(F fn)
    {
        EcsComponent<T>& s = getComponentSystem<T>();

        if constexpr (std::is_invocable_r_v<void, F, Entity>)
        {
            forPar(0, s.components.size(), [&](u64 idx)
            {
                fn(s.entities[idx]);
            });
        }
        else if constexpr (std::is_invocable_r_v<void, F, T&>)
        {
            forPar(0, s.components.size(), [&](u64 idx)
            {
                fn(s.components[idx]);
            });
        }
        else if constexpr (std::is_invocable_r_v<void, F, Entity, T&>)
        {
            forPar(0, s.components.size(), [&](u64 idx)
            {
                fn(s.entities[idx], s.components[idx]);
            });
        }
    }

    /**
     * Returns the smallest list of entities with at least one of the types
     */
    template<typename... Us>
    std::span<Entity> getSmallestEntities()
    {
        std::span<Entity> ret = getComponentSystem<FirstInPackT<Us...>>().entities;
        ([&]()
        {
            std::span<Entity> es = getComponentSystem<Us>().entities;
            if (es.size() < ret.size())
                ret = es;
        }(), ...);
        return ret;
    }

    /**
     * Returns the smallest list of entities with at least one of the types (const)
     */
    template<typename... Us>
    std::span<const Entity> getSmallestEntities() const
    {
        std::span<const Entity> ret = getComponentSystem<FirstInPackT<Us...>>().entities;
        ([&]()
        {
            std::span<const Entity> es = getComponentSystem<Us>().entities;
            if (es.size() < ret.size())
                ret = es;
        }(), ...);
        return ret;
    }

    /**
     * Calls a function for each entity with a list of components
     */
    template<typename... Us, typename F> requires (sizeof...(Us) > 1)
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
    template<typename... Us, typename F> requires (sizeof...(Us) > 1)
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
    template<typename... Us, typename F> requires (sizeof...(Us) > 1)
    void forEachPar(F fn)
    {
        std::span<const Entity> entrySpan = getSmallestEntities<Us...>();
        forPar(0, entrySpan.size(), [&](u64 idx)
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

/**
 * The serializer for an ecs
 */
struct EntitySerializer {
    /**
     * The indices of entities, if writing
     */
    vector_temp<u32> entityToIdx{};
    /**
     * The entities by index, if reading
     */
    vector_temp<Entity> idxToEntity{};

    /**
     * Initialize with arena
     */
    EntitySerializer(Arena* arena)
        : entityToIdx{arena}
        , idxToEntity{arena}
    {}

    /**
     * The the writable index from the entity
     */
    u32 getIdx(Entity e)
    {
        return entityToIdx[e.idx()];
    }

    /**
     * Get the associated entity from the index
     */
    Entity getEntity(u32 idx)
    {
        return idxToEntity[idx];
    }
};

/**
 * The default serialization for a component, may be overridden
 */
template<typename T>
void ecsSerialize(Serializer* s, T* val, EntitySerializer*)
{
    serialize(s, val);
}

/**
 * Entity ecs serialization
 */
template<>
inline void ecsSerialize(Serializer* s, Entity* e, EntitySerializer* ecs)
{
    if (s->writing)
    {
        u32 idx = ecs->getIdx(*e);
        serializeObject(s, &idx);
    }
    else
    {
        u32 idx;
        serializeObject(s, &idx);
        *e = ecs->getEntity(idx);
    }
}

/**
 * Serialize one component system of an ecs
 */
template<typename Sys, typename EcsT>
bool serializeEcsSystem(Serializer* s, Sys& sys, EntitySerializer* es, EcsT* ecs)
{
    using T = typename std::remove_cvref_t<Sys>::Type;

    if (s->writing)
    {
        u32 cCount = static_cast<u32>(sys.components.size());
        if (!serialize(s, &cCount))
            return false;

        for (u32 i = 0; i < sys.components.size(); ++i)
        {
            u32 idx = es->getIdx(sys.entities[i]);
            if (!serialize(s, &idx))
                return false;
            ecsSerialize(s, &sys.components[i], es);
        }
        return true;
    }
    else
    {
        u32 cCount;
        if (!serialize(s, &cCount))
            return false;

        for (u32 i = 0; i < cCount; ++i)
        {
            u32 idx;
            if (!serialize(s, &idx))
                return false;
            ecsSerialize(s, &ecs->template add<T>(es->getEntity(idx)), es);
        }
        return true;
    }
}

/**
 * Ecs serialization
 *
 * Note, if the ecs is not empty, the serialized data is added to the ecs
 */
template<typename... Ts>
bool serialize(Serializer* s, Ecs<Ts...>* ecs)
{
    serializeBegin(s);
    HG_DEFER(serializeEnd(s));

    ArenaScope scratch = getScratch();
    EntitySerializer es{scratch};

    if (s->writing)
    {
        es.entityToIdx.resize(ecs->entities.size());

        u32 eCount = 0;
        for (u32 i = 0; i < ecs->entities.size(); ++i)
        {
            if (ecs->entities[i] != nullEntity)
                es.entityToIdx[i] = eCount++;
            else
                es.entityToIdx[i] = static_cast<u32>(-1);
        }
        serialize(s, &eCount);
    }
    else
    {
        u32 eCount;
        if (!serialize(s, &eCount))
            return false;

        es.idxToEntity.resize(eCount);
        for (u32 i = 0; i < eCount; ++i)
        {
            es.idxToEntity[i] = ecs->spawn();
        }
    }

    return std::apply([&](auto&... sys)
    {
        return (serializeEcsSystem(s, sys, &es, ecs) && ...);
    },
    ecs->systems);
}

} // namespace hg
