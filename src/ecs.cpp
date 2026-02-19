#include "hurdygurdy.hpp"

HgEntity* entity_pool = nullptr;
u32 entity_capacity = 0;
HgEntity next_entity = {};

struct Component {
    u32* sparse;
    HgEntity* dense;
    HgDynamicArray components;
};
Component* systems = nullptr;
usize system_count = 0;

static u32& hg_internal_current_component_id() {
    static u32 id = 0;
    return id;
}

u32 hg_create_component_id() {
    u32 id = hg_internal_current_component_id();
    ++hg_internal_current_component_id();
    return id;
}

HgEntity HgEntity::create() {
    hg_assert(next_entity.id < entity_capacity);

    HgEntity entity = next_entity;
    next_entity = entity_pool[entity.id];
    entity_pool[entity.id] = entity;
    return entity;
}

void HgEntity::destroy() {
    hg_assert(alive());

    for (u32 i = 0; i < system_count; ++i) {
        if (hg_ecs_is_registered(i) && has(i))
            remove(i);
    }
    entity_pool[id] = next_entity;
    next_entity = *this;
}

bool HgEntity::alive() {
    return id < entity_capacity && entity_pool[id].id == id;
}

void* HgEntity::add(u32 component_id) {
    hg_assert(alive());
    hg_assert(hg_ecs_is_registered(component_id));
    hg_assert(!has(component_id));

    systems[component_id].sparse[id] = (u32)systems[component_id].components.count;
    systems[component_id].dense[systems[component_id].components.count] = *this;
    return systems[component_id].components.push();
}

void HgEntity::remove(u32 component_id) {
    hg_assert(alive());
    hg_assert(hg_ecs_is_registered(component_id));
    hg_assert(has(component_id));

    u32 index = systems[component_id].sparse[id];
    systems[component_id].sparse[id] = (u32)-1;

    systems[component_id].dense[index] = systems[component_id].dense[systems[component_id].components.count - 1];
    systems[component_id].components.swap_remove(index);
}

bool HgEntity::has(u32 component_id) {
    hg_assert(alive());
    hg_assert(hg_ecs_is_registered(component_id));
    return systems[component_id].sparse[id] < systems[component_id].components.count;
}

void* HgEntity::get(u32 component_id) {
    hg_assert(alive());
    hg_assert(hg_ecs_is_registered(component_id));
    hg_assert(has(component_id));
    return systems[component_id].components.get(systems[component_id].sparse[id]);
}

HgEntity HgEntity::get(const void* component, u32 component_id) {
    hg_assert(component != nullptr);
    hg_assert(hg_ecs_is_registered(component_id));

    usize index = ((uptr)component - (uptr)systems[component_id].components.items)
                / systems[component_id].components.width;
    return systems[component_id].dense[index];
}

void hg_ecs_init(HgArena& arena, u32 max_entities) {
    entity_pool = arena.alloc<HgEntity>(max_entities);
    entity_capacity = max_entities;

    systems = arena.alloc<Component>(hg_internal_current_component_id());
    system_count = hg_internal_current_component_id();

    for (u32 i = 0; i < entity_capacity; ++i) {
        entity_pool[i] = {i + 1};
    }
    next_entity = {0};

    for (u32 i = 0; i < system_count; ++i) {
        systems[i] = {};
    }
}

void hg_ecs_reset() {
    for (u32 i = 0; i < system_count; ++i) {
        if (hg_ecs_is_registered(i)) {
            std::memset(systems[i].sparse, -1, entity_capacity * sizeof(*systems[i].sparse));
            systems[i].components.reset();
        }
    }
    for (u32 i = 0; i < entity_capacity; ++i) {
        entity_pool[i] = {i + 1};
    }
    next_entity = {0};
}

void hg_ecs_resize(HgArena& arena, u32 new_max_entities) {
    entity_pool = arena.realloc(entity_pool, entity_capacity, new_max_entities);
    for (u32 i = (u32)entity_capacity; i < (u32)new_max_entities; ++i) {
        entity_pool[i] = {i + 1};
    }
    for (usize i = 0; i < system_count; ++i) {
        systems[i].sparse = arena.realloc(systems[i].sparse, entity_capacity, new_max_entities);
    }
    entity_capacity = new_max_entities;
}

void hg_ecs_grow(HgArena& arena, f32 factor) {
    hg_ecs_resize(arena, entity_capacity == 0 ? 1 : (u32)((f32)entity_capacity * factor));
}

void hg_ecs_register(HgArena& arena, u32 max_components, u32 width, u32 alignment, u32 component_id) {
    if (hg_ecs_is_registered(component_id))
        return;

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

void hg_ecs_unregister(u32 component_id) {
    if (!hg_ecs_is_registered(component_id))
        return;
    systems[component_id] = {};
}

bool hg_ecs_is_registered(u32 component_id) {
    hg_assert(component_id < system_count);
    return systems[component_id].components.items != nullptr;
}

HgEntity* hg_ecs_entities(u32 component_id) {
    hg_assert(hg_ecs_is_registered(component_id));
    return systems[component_id].dense;
}

void* hg_ecs_components(u32 component_id) {
    hg_assert(hg_ecs_is_registered(component_id));
    return systems[component_id].components.items;
}

u32 hg_ecs_component_count(u32 component_id) {
    hg_assert(hg_ecs_is_registered(component_id));
    return (u32)systems[component_id].components.count;
}

u32 hg_ecs_find_smallest(u32* ids, usize id_count) {
    u32 smallest = ids[0];
    hg_assert(hg_ecs_is_registered(ids[0]));
    for (usize i = 1; i < id_count; ++i) {
        hg_assert(hg_ecs_is_registered(ids[i]));
        if (systems[ids[i]].components.count < systems[smallest].components.count)
            smallest = ids[i];
    }
    return smallest;
}

static void swap_idx_location(u32 lhs, u32 rhs, u32 component_id) {
    hg_assert(hg_ecs_is_registered(component_id));

    Component& system = systems[component_id];
    hg_assert(lhs < system.components.count);
    hg_assert(rhs < system.components.count);

    HgEntity lhs_entity = system.dense[lhs];
    HgEntity rhs_entity = system.dense[rhs];

    hg_assert(lhs_entity.alive());
    hg_assert(rhs_entity.alive());
    hg_assert(lhs_entity.has(component_id));
    hg_assert(rhs_entity.has(component_id));

    system.dense[lhs] = rhs_entity;
    system.dense[rhs] = lhs_entity;
    system.sparse[lhs_entity.id] = rhs;
    system.sparse[rhs_entity.id] = lhs;

    void* temp = alloca(system.components.width);
    std::memcpy(temp, system.components.get(lhs), system.components.width);
    std::memcpy(system.components.get(lhs), system.components.get(rhs), system.components.width);
    std::memcpy(system.components.get(rhs), temp, system.components.width);
}

struct QuicksortData {
    u32 component_id;
    void* data;
    bool (*compare)(void*, HgEntity lhs, HgEntity rhs);
};

static u32 quicksort_inter(QuicksortData* q, u32 pivot, u32 inc, u32 dec) {
    while (inc != dec) {
        while (!q->compare(q->data, systems[q->component_id].dense[dec], systems[q->component_id].dense[pivot])) {
            --dec;
            if (dec == inc)
                goto finish;
        }
        while (!q->compare(q->data, systems[q->component_id].dense[pivot], systems[q->component_id].dense[inc])) {
            ++inc;
            if (inc == dec)
                goto finish;
        }
        swap_idx_location(inc, dec, q->component_id);
    }

finish:
    if (q->compare(q->data, systems[q->component_id].dense[inc], systems[q->component_id].dense[pivot]))
        swap_idx_location(pivot, inc, q->component_id);

    return inc;
}

static void quicksort(QuicksortData* q, u32 begin, u32 end) {
    hg_assert(begin <= end && end <= hg_ecs_component_count(q->component_id));

    if (begin + 1 >= end)
        return;

    u32 middle = quicksort_inter(q, begin, begin + 1, end - 1);
    quicksort(q, begin, middle);
    quicksort(q, middle, end);
}

void hg_ecs_sort_untyped(
    u32 begin,
    u32 end,
    u32 component_id,
    void* data,
    bool (*compare)(void*, HgEntity lhs, HgEntity rhs)
) {
    hg_assert(hg_ecs_is_registered(component_id));

    QuicksortData q;
    q.component_id = component_id;
    q.data = data;
    q.compare = compare;
    quicksort(&q, begin, end);
}

void HgTransform::add_child(HgEntity child) {
    HgTransform& old_first = first_child.get<HgTransform>();
    HgTransform& new_first = child.get<HgTransform>();
    old_first.prev_sibling = child;
    new_first.next_sibling = first_child;
    first_child = child;
}

void HgTransform::detach() {
    if (parent != HgEntity{}) {
        if (first_child == HgEntity{}) {
            if (prev_sibling == HgEntity{}) {
                parent.get<HgTransform>().first_child = next_sibling;
                next_sibling.get<HgTransform>().prev_sibling = HgEntity{};
            } else {
                prev_sibling.get<HgTransform>().next_sibling = next_sibling;
                next_sibling.get<HgTransform>().prev_sibling = prev_sibling;
            }
        } else {
            HgEntity last_child = first_child;
            for (;;) {
                HgTransform& tf = last_child.get<HgTransform>();
                tf.parent = parent;
                if (tf.next_sibling == HgEntity{})
                    break;
                last_child = tf.next_sibling;
            }
            if (prev_sibling == HgEntity{}) {
                parent.get<HgTransform>().first_child = first_child;
                next_sibling.get<HgTransform>().prev_sibling = last_child;
            } else {
                prev_sibling.get<HgTransform>().next_sibling = first_child;
                next_sibling.get<HgTransform>().prev_sibling = last_child;
            }
        }
    } else {
        HgEntity child = first_child;
        while (child != HgEntity{}) {
            HgTransform& tf = child.get<HgTransform>();
            child = tf.next_sibling;
            tf.parent = HgEntity{};
            tf.next_sibling = HgEntity{};
            tf.prev_sibling = HgEntity{};
        }
    }
}

void HgTransform::destroy() {
    HgEntity child = first_child;
    while (child != HgEntity{}) {
        HgTransform& tf = child.get<HgTransform>();
        tf.destroy();
        child = tf.next_sibling;
    }
    if (parent != HgEntity{}) {
        if (prev_sibling != HgEntity{}) {
            prev_sibling.get<HgTransform>().next_sibling = next_sibling;
            if (next_sibling != HgEntity{})
                next_sibling.get<HgTransform>().prev_sibling = prev_sibling;
        } else {
            parent.get<HgTransform>().first_child = next_sibling;
            if (next_sibling != HgEntity{})
                next_sibling.get<HgTransform>().prev_sibling = HgEntity{};
        }
    }
    HgEntity::get(*this).destroy();
}

void HgTransform::set(const HgVec3& p, const HgVec3& s, const HgQuat& r) {
    HgEntity child = first_child;
    while (child != HgEntity{}) {
        HgTransform& tf = child.get<HgTransform>();
        // tf.set(
        //     hg_rotate(r, (tf.position - position) * s / tf.scale + p),
        //     s * tf.scale / scale,
        //     r);
        child = tf.next_sibling;
    }
    position = p;
    scale = s;
    rotation = r;
}

void HgTransform::move(const HgVec3& dp, const HgVec3& ds, const HgQuat& dr) {
    set(position + dp, scale * ds, dr * rotation);
}

