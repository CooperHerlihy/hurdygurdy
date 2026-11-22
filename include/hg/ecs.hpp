#pragma once

#include "hg/inttypes.hpp"
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
class Entity {
private:
    /**
     * The entity handle
     */
    u32 handle = static_cast<u32>(-1);

public:
    /**
     * The number of bits in a handle used for the index
     */
    static constexpr u32 idxBits = 24;

    /**
     * Construct null
     */
    constexpr Entity() noexcept = default;

    /**
     * Construct from handle
     */
    explicit constexpr Entity(u32 handleVal)
        : handle{handleVal}
    {}

    /**
     * Get the index from a handle
     */
    constexpr u32 idx() const
    {
        return handle & ((u32{1} << idxBits) - u32{1});
    }

    /**
     * Returns a new handle at the same index
     */
    constexpr Entity nextIdx() const
    {
        Entity next;
        next.handle = handle + u32{1};
        return next;
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
        Entity next;
        next.handle = handle + (u32{1} << idxBits);
        return next;
    }

    /**
     * Compare entities
     */
    constexpr bool operator==(Entity other) const
    {
        return handle == other.handle;
    }
};

/**
 * The null entity
 */
static constexpr Entity nullEntity = Entity{};

/**
 * An entity component system
 */
template<typename... Ts>
struct Ecs {
private:
    /**
     * A component in an entity component system
     */
    template<typename T>
    struct System {
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
    };

    /**
     * The component systems
     */
    std::tuple<System<Ts>...> systems{};
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

public:
    /**
     * Despawn all entities
     */
    void clearEntities()
    {
        (clearComponents<Ts>(), ...);
        entities.clear();
        freeEntities.clear();
        nextEntity = Entity{0};
    }

    /**
     * Remove component type from all entities
     */
    template<typename... Us>
    void clearComponents()
    {
        ([&]
        {
            System<Us>& s = std::get<System<Us>>(systems);
            s.indices.clear();
            s.entities.clear();
            s.components.clear();
        }(), ...);
    }

    /**
     * Spawn a new entity with no components
     */
    Entity spawn()
    {
        if (freeEntities.empty())
        {
            Entity e = nextEntity;
            nextEntity = e.nextIdx();
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

        (([&]()
        {
            if (has<Ts>(e))
                remove<Ts>(e);
        }()), ...);
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
    template<typename T, typename... Args>
    T& add(Entity e, Args&&... args)
    {
        HG_ASSERT(alive(e));
        HG_ASSERT(!has<T>(e));

        System<T>& s = std::get<System<T>>(systems);

        if (e.idx() >= s.indices.size())
        {
            u64 oldCount = s.indices.size();
            s.indices.resize(e.idx() + 1);
            for (u64 i = oldCount; i < s.indices.size(); ++i)
            {
                s.indices[i] = static_cast<u32>(-1);
            }
        }

        u64 idx = s.entities.size();
        s.indices[e.idx()] = assertCast<u32>(idx);
        s.entities.emplace_back(e);
        return s.components.emplace_back(std::forward<Args>(args)...);
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
        HG_ASSERT(has<T>(e));

        System<T>& s = std::get<System<T>>(systems);

        u32 idx = s.indices[e.idx()];
        s.indices[e.idx()] = static_cast<u32>(-1);
        s.entities[idx] = std::move(s.entities.back());
        s.components[idx] = std::move(s.components.back());
        s.entities.pop_back();
        s.components.pop_back();

        if (idx < s.entities.size())
        {
            Entity moved = s.entities[idx];
            s.indices[moved.idx()] = idx;
        }
    }

    /**
     * Returns whether an entity has a component
     */
    template<typename T>
    bool has(Entity e) const
    {
        HG_ASSERT(alive(e));
        const System<T>& s = std::get<System<T>>(systems);
        return e.idx() < s.indices.size() && s.indices[e.idx()] != static_cast<u32>(-1);
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
        HG_ASSERT(has<T>(e));

        System<T>& s = std::get<System<T>>(systems);
        return s.components[s.indices[e.idx()]];
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
        HG_ASSERT(has<T>(e));

        const System<T>& s = std::get<System<T>>(systems);
        return s.components[s.indices[e.idx()]];
    }

    /**
     * Get the entity associated with a component
     */
    template<typename T>
    Entity getEntity(const T& c) const
    {
        const System<T>& s = std::get<System<T>>(systems);
        return s.entities[assertCast<u32>(&c - s.components.data())];
    }

    /**
     * Returns all entities
     */
    std::span<const Entity> getEntities() const
    {
        return entities;
    }

    /**
     * Returns all entities with a component type
     */
    template<typename T>
    std::span<const Entity> getEntities() const
    {
        return std::get<System<T>>(systems).entities;
    }

    /**
     * Returns all components of a type
     */
    template<typename T>
    std::span<T> getComponents()
    {
        return std::get<System<T>>(systems).components;
    }

    /**
     * Returns all components of a type (const)
     */
    template<typename T>
    std::span<const T> getComponents() const
    {
        return std::get<System<T>>(systems).components;
    }

    /**
     * Calls a function for each entity with a component type
     */
    template<typename T, typename F> requires std::is_invocable_r_v<void, F, Entity>
    void forEach(F fn)
    {
        System<T>& s = std::get<System<T>>(systems);
        for (Entity e : s.entities)
        {
            fn(e);
        }
    }

    /**
     * Calls a function for each entity with a component type
     */
    template<typename T, typename F> requires std::is_invocable_r_v<void, F, T&>
    void forEach(F fn)
    {
        System<T>& s = std::get<System<T>>(systems);
        for (T& c : s.components)
        {
            fn(c);
        }
    }

    /**
     * Calls a function for each entity with a component type
     */
    template<typename T, typename F> requires std::is_invocable_r_v<void, F, Entity, T&>
    void forEach(F fn)
    {
        System<T>& s = std::get<System<T>>(systems);
        auto* c = s.components.data();
        for (Entity e : s.entities)
        {
            fn(e, *(c++));
        }
    }

    /**
     * Calls a function for each entity with a component type
     */
    template<typename T, typename F> requires std::is_invocable_r_v<void, F, Entity>
    void forEach(F fn) const
    {
        const System<T>& s = std::get<System<T>>(systems);
        for (Entity e : s.entities)
        {
            fn(e);
        }
    }

    /**
     * Calls a function for each entity with a component type
     */
    template<typename T, typename F> requires std::is_invocable_r_v<void, F, T&>
    void forEach(F fn) const
    {
        const System<T>& s = std::get<System<T>>(systems);
        for (const T& c : s.components)
        {
            fn(c);
        }
    }

    /**
     * Calls a function for each entity with a component type
     */
    template<typename T, typename F> requires std::is_invocable_r_v<void, F, Entity, T&>
    void forEach(F fn) const
    {
        const System<T>& s = std::get<System<T>>(systems);
        auto* c = s.components.data();
        for (Entity e : s.entities)
        {
            fn(e, *(c++));
        }
    }

    /**
     * Calls a function in parallel for each entity with a component type
     */
    template<typename T, typename F> requires std::is_invocable_r_v<void, F, Entity>
    void forEachPar(F fn)
    {
        System<T>& s = std::get<System<T>>(systems);
        forPar(0, s.components.size(), [&](u64 idx)
        {
            fn(s.entities[idx]);
        });
    }

    /**
     * Calls a function in parallel for each entity with a component type
     */
    template<typename T, typename F> requires std::is_invocable_r_v<void, F, T&>
    void forEachPar(F fn)
    {
        System<T>& s = std::get<System<T>>(systems);
        forPar(0, s.components.size(), [&](u64 idx)
        {
            fn(s.components[idx]);
        });
    }

    /**
     * Calls a function in parallel for each entity with a component type
     */
    template<typename T, typename F> requires std::is_invocable_r_v<void, F, Entity, T&>
    void forEachPar(F fn)
    {
        System<T>& s = std::get<System<T>>(systems);
        forPar(0, s.components.size(), [&](u64 idx)
        {
            fn(s.entities[idx], s.components[idx]);
        });
    }

    /**
     * Returns the smallest list of entities with at least one of the types (const)
     */
    template<typename U, typename... Us>
    std::span<const Entity> getSmallestEntities() const
    {
        std::span<const Entity> ret = std::get<System<U>>(systems).entities;
        ([&]()
        {
            std::span<const Entity> es = std::get<System<Us>>(systems).entities;
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
class EntitySerializer {
private:
    /**
     * The indices of entities, if writing
     */
    VectorTemp<u32> entityToIdx{};
    /**
     * The entities by index, if reading
     */
    VectorTemp<Entity> idxToEntity{};

public:
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

    /**
     * The serialize function
     */
    template<typename... Ts>
    friend bool serialize(Serializer& s, Ecs<Ts...>* ecs);
};

/**
 * The default serialization for a component, may be overridden
 */
template<typename T>
bool ecsSerialize(Serializer& s, T* val, EntitySerializer*)
{
    return serialize(s, val);
}

/**
 * Entity ecs serialization
 */
template<>
inline bool ecsSerialize(Serializer& s, Entity* e, EntitySerializer* ecs)
{
    if (s.isWriting())
    {
        u32 idx = ecs->getIdx(*e);
        serializeObject(s, &idx);
    }
    else
    {
        u32 idx;
        if (!serializeObject(s, &idx))
            return false;
        *e = ecs->getEntity(idx);
    }
    return true;
}

namespace internal {

/**
 * Serialize one component system of an ecs
 */
template<typename T, typename EcsT>
bool serializeEcsSystem(Serializer& s, EntitySerializer* es, EcsT* ecs)
{
    std::span<const Entity> entities = ecs->template getEntities<T>();
    std::span<T> components = ecs->template getComponents<T>();

    u64 cCount = components.size();
    serialize(s, &cCount);

    if (s.isWriting())
    {
        for (u64 i = 0; i < cCount; ++i)
        {
            u32 idx = es->getIdx(entities[i]);
            if (!serialize(s, &idx) ||
                !ecsSerialize(s, &components[i], es))
                return false;
        }
        return true;
    }
    else
    {
        for (u64 i = 0; i < cCount; ++i)
        {
            u32 idx;
            if (!serialize(s, &idx) ||
                !ecsSerialize(s, &ecs->template add<T>(es->getEntity(idx)), es))
                return false;
        }
        return true;
    }
}

} // namespace internal

/**
 * Ecs serialization
 *
 * Note, if the ecs is not empty, the serialized data is added to the ecs
 */
template<typename... Ts>
bool serialize(Serializer& s, Ecs<Ts...>* ecs)
{
    ArenaScope scratch = getScratch();
    EntitySerializer es{scratch};

    if (s.isWriting())
    {
        es.entityToIdx.resize(ecs->getEntities().size());

        u32 eCount = 0;
        for (u32 i = 0; i < ecs->getEntities().size(); ++i)
        {
            if (ecs->getEntities()[i] != nullEntity)
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

    return (internal::serializeEcsSystem<Ts>(s, &es, ecs) && ...);
}

} // namespace hg
