#include "hurdygurdy.hpp"

static u32& hg_internal_current_component_id() {
    static u32 id = 0;
    return id;
}

u32 hg_create_component_id() {
    u32 id = hg_internal_current_component_id();
    ++hg_internal_current_component_id();
    return id;
}

HgECS HgECS::create(HgArena& arena, u32 max_entities) {
    HgECS ecs;

    ecs.entity_pool = arena.alloc<HgEntity>(max_entities);
    ecs.entity_capacity = max_entities;

    ecs.systems = arena.alloc<Component>(hg_internal_current_component_id());
    ecs.system_count = hg_internal_current_component_id();

    for (u32 i = 0; i < ecs.entity_capacity; ++i) {
        ecs.entity_pool[i] = {i + 1};
    }
    ecs.next_entity = {0};

    for (u32 i = 0; i < ecs.system_count; ++i) {
        ecs.systems[i] = {};
    }

    return ecs;
}

void HgECS::reset() {
    for (u32 i = 0; i < system_count; ++i) {
        if (is_registered(i)) {
            std::memset(systems[i].sparse, -1, entity_capacity * sizeof(*systems[i].sparse));
            systems[i].components.reset();
        }
    }
    for (u32 i = 0; i < entity_capacity; ++i) {
        entity_pool[i] = {i + 1};
    }
    next_entity = {0};
}

void HgECS::realloc_entities(HgArena& arena, u32 new_capacity) {
    entity_pool = arena.realloc(entity_pool, entity_capacity, new_capacity);
    for (u32 i = (u32)entity_capacity; i < (u32)new_capacity; ++i) {
        entity_pool[i] = {i + 1};
    }
    for (usize i = 0; i < system_count; ++i) {
        systems[i].sparse = arena.realloc(systems[i].sparse, entity_capacity, new_capacity);
    }
    entity_capacity = new_capacity;
}

HgEntity HgECS::spawn() {
    hg_assert(next_entity < entity_capacity);

    HgEntity entity = next_entity;
    next_entity = entity_pool[entity];
    entity_pool[entity] = entity;
    return entity;
}

void HgECS::despawn(HgEntity entity) {
    hg_assert(is_alive(entity));

    for (u32 i = 0; i < system_count; ++i) {
        if (is_registered(i) && has(entity, i))
            remove(entity, i);
    }
    entity_pool[entity] = next_entity;
    next_entity = entity;
}

void HgECS::register_component(HgArena& arena, u32 max_components, u32 width, u32 alignment, u32 component_id) {
    hg_assert(!is_registered(component_id));
    if (component_id >= system_count) {
        systems = arena.realloc(systems, system_count, component_id + 1);
        system_count = component_id + 1;
    }

    Component& system = systems[component_id];
    system.sparse = arena.alloc<u32>(entity_capacity);
    system.dense = arena.alloc<HgEntity>(max_components);
    system.components = system.components.create(arena, width, alignment, 0, max_components);
    std::memset(system.sparse, -1, entity_capacity * sizeof(*system.sparse));
}

void HgECS::unregister_component(u32 component_id) {
    if (!is_registered(component_id))
        return;

    systems[component_id] = {};
}

u32 HgECS::smallest_id(u32* ids, usize id_count) {
    u32 smallest = ids[0];
    hg_assert(is_registered(ids[0]));
    for (usize i = 1; i < id_count; ++i) {
        hg_assert(is_registered(ids[i]));
        if (systems[ids[i]].components.count < systems[smallest].components.count)
            smallest = ids[i];
    }
    return smallest;
}

void* HgECS::add(HgEntity entity, u32 component_id) {
    hg_assert(is_alive(entity));
    hg_assert(is_registered(component_id));
    hg_assert(!has(entity, component_id));

    systems[component_id].sparse[entity] = (u32)systems[component_id].components.count;
    systems[component_id].dense[systems[component_id].components.count] = entity;
    return systems[component_id].components.push();
}

void HgECS::remove(HgEntity entity, u32 component_id) {
    hg_assert(is_alive(entity));
    hg_assert(is_registered(component_id));
    hg_assert(has(entity, component_id));

    u32 index = systems[component_id].sparse[entity];
    systems[component_id].sparse[entity] = (u32)-1;

    systems[component_id].dense[index] = systems[component_id].dense[systems[component_id].components.count - 1];
    systems[component_id].components.swap_remove(index);
}

void HgECS::swap_idx_location(u32 lhs, u32 rhs, u32 component_id) {
    hg_assert(is_registered(component_id));

    Component& system = systems[component_id];
    hg_assert(lhs < system.components.count);
    hg_assert(rhs < system.components.count);

    HgEntity lhs_entity = system.dense[lhs];
    HgEntity rhs_entity = system.dense[rhs];

    hg_assert(is_alive(lhs_entity));
    hg_assert(is_alive(rhs_entity));
    hg_assert(has(lhs_entity, component_id));
    hg_assert(has(rhs_entity, component_id));

    system.dense[lhs] = rhs_entity;
    system.dense[rhs] = lhs_entity;
    system.sparse[lhs_entity] = rhs;
    system.sparse[rhs_entity] = lhs;

    void* temp = alloca(system.components.width);
    std::memcpy(temp, system.components.get(lhs), system.components.width);
    std::memcpy(system.components.get(lhs), system.components.get(rhs), system.components.width);
    std::memcpy(system.components.get(rhs), temp, system.components.width);
}

void HgECS::sort_untyped(
    u32 begin,
    u32 end,
    u32 component_id,
    void* data,
    bool (*compare)(void*, HgEntity lhs, HgEntity rhs)
) {
    hg_assert(is_registered(component_id));

    auto inter = [this, component_id, data, compare](u32 pivot, u32 inc, u32 dec) -> u32 {
        while (inc != dec) {
            while (!compare(data, systems[component_id].dense[dec], systems[component_id].dense[pivot])) {
                --dec;
                if (dec == inc)
                    goto finish;
            }
            while (!compare(data, systems[component_id].dense[pivot], systems[component_id].dense[inc])) {
                ++inc;
                if (inc == dec)
                    goto finish;
            }
            swap_idx_location(inc, dec, component_id);
        }

finish:
        if (compare(data, systems[component_id].dense[inc], systems[component_id].dense[pivot]))
            swap_idx_location(pivot, inc, component_id);

        return inc;
    };

    auto quicksort = [this, component_id, inter](const auto& recurse, u32 qbegin, u32 qend) {
        hg_assert(qbegin <= qend && qend <= component_count(component_id));

        if (qbegin + 1 >= qend)
            return;

        u32 middle = inter(qbegin, qbegin + 1, qend - 1);
        recurse(recurse, qbegin, middle);
        recurse(recurse, middle, qend);
    };

    quicksort(quicksort, begin, end);
}

