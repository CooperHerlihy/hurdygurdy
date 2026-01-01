#include "hurdygurdy.hpp"

#if defined(HG_PLATFORM_LINUX)
#include <alloca.h>
#elif defined(HG_PLATFORM_WINDOWS)
#include <malloc.h>
#endif

static void hg_internal_platform_init();
static void hg_internal_platform_exit();

void hg_init(void) {
    hg_vk_load();
    hg_internal_platform_init();
}

void hg_exit(void) {
    hg_internal_platform_exit();
    hg_vk_unload();
}

hg_test(hg_init_and_exit) {
    for (usize i = 0; i < 16; ++i) {
        hg_init();
        hg_init();
        hg_exit();
        hg_exit();
    }
    return true;
}

static HgArray<HgTest>& hg_internal_get_tests() {
    static HgArray<HgTest> internal_tests = internal_tests.create(HgStdAllocator::get(), 0, 1024);
    return internal_tests;
}

HgTest::HgTest(const char *test_name, bool (*test_function)()) : name(test_name), function(test_function) {
    hg_internal_get_tests().push(HgStdAllocator::get(), *this);
}

bool hg_run_tests() {
    HgArray<HgTest>& tests = hg_internal_get_tests();

    bool all_succeeded = true;
    std::printf("HurdyGurdy: Tests Begun\n");
    for (usize i = 0; i < tests.count; ++i) {
        std::printf("%s...\n", tests[i].name);
        if (tests[i].function()) {
            std::printf("\x1b[32mSuccess\n\x1b[0m");
        } else {
            all_succeeded = false;
            std::printf("\x1b[31mFailure\n\x1b[0m");
        }
    }
    if (all_succeeded) {
        std::printf("HurdyGurdy: Tests Complete \x1b[32m[Success]\x1b[0m\n");
    } else {
        std::printf("HurdyGurdy: Tests Complete \x1b[31m[Failure]\x1b[0m\n");
    }
    return all_succeeded;
}

hg_test(hg_test) {
    hg_test_assert(true);
    return true;
}

hg_test(hg_matrix_mul) {
    HgMat2f mat{
        {1.0f, 0.0f},
        {1.0f, 0.0f},
    };
    HgVec2f vec{1.0f, 1.0f};

    HgMat2f identity{
        {1.0f, 0.0f},
        {0.0f, 1.0f},
    };
    hg_test_assert(identity * mat == mat);
    hg_test_assert(identity * vec == vec);

    HgMat2f mat_rotated{
        {0.0f, 1.0f},
        {0.0f, 1.0f},
    };
    HgVec2f vec_rotated{-1.0f, 1.0f};

    HgMat2f rotation{
        {0.0f, 1.0f},
        {-1.0f, 0.0f},
    };
    hg_test_assert(rotation * mat == mat_rotated);
    hg_test_assert(rotation * vec == vec_rotated);

    hg_test_assert((identity * rotation) * mat == identity * (rotation * mat));
    hg_test_assert((identity * rotation) * vec == identity * (rotation * vec));
    hg_test_assert((rotation * rotation) * mat == rotation * (rotation * mat));
    hg_test_assert((rotation * rotation) * vec == rotation * (rotation * vec));

    return true;
}

hg_test(hg_quat) {
    HgMat3f identity_mat = 1.0f;
    HgVec3f up_vec{0.0f, -1.0f, 0.0f};
    HgQuatf rotation = hg_axis_angle<f32>({0.0f, 0.0f, -1.0f}, -(f32)HgPi * 0.5f);

    HgVec3f rotated_vec = hg_rotate(rotation, up_vec);
    HgMat3f rotated_mat = hg_rotate(rotation, identity_mat);

    HgVec3f mat_rotated_vec = rotated_mat * up_vec;

    hg_test_assert(std::abs(rotated_vec.x - 1.0f) < FLT_EPSILON
                && std::abs(rotated_vec.y - 0.0f) < FLT_EPSILON
                && std::abs(rotated_vec.y - 0.0f) < FLT_EPSILON);

    hg_test_assert(std::abs(mat_rotated_vec.x - rotated_vec.x) < FLT_EPSILON
                && std::abs(mat_rotated_vec.y - rotated_vec.y) < FLT_EPSILON
                && std::abs(mat_rotated_vec.y - rotated_vec.z) < FLT_EPSILON);

    return true;
}

HgMat4f hg_model_matrix_2d(const HgVec3f& position, const HgVec2f& scale, f32 rotation) {
    HgMat2f m2{{scale.x, 0.0f}, {0.0f, scale.y}};
    f32 rot_sin = std::sin(rotation);
    f32 rot_cos = std::cos(rotation);
    HgMat2f rot{{rot_cos, rot_sin}, {-rot_sin, rot_cos}};
    HgMat4 m4 = rot * m2;
    m4.w.x = position.x;
    m4.w.y = position.y;
    m4.w.z = position.z;
    return m4;
}

HgMat4f hg_model_matrix_3d(const HgVec3f& position, const HgVec3f& scale, const HgQuatf& rotation) {
    HgMat3f m3{};
    m3.x.x = scale.x;
    m3.y.y = scale.y;
    m3.z.z = scale.z;
    m3 = hg_rotate(rotation, m3);
    HgMat4f m4{m3};
    m4.w.x = position.x;
    m4.w.y = position.y;
    m4.w.z = position.z;
    return m4;
}

HgMat4f hg_view_matrix(const HgVec3f& position, f32 zoom, const HgQuatf& rotation) {
    HgMat4f rot{hg_rotate(hg_conj(rotation), HgMat3f{1.0f})};
    HgMat4f pos{1.0f};
    pos.x.x = zoom;
    pos.y.y = zoom;
    pos.w.x = -position.x;
    pos.w.y = -position.y;
    pos.w.z = -position.z;
    return rot * pos;
}

HgMat4f hg_projection_orthographic(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far) {
    return {
        {2.0f / (right - left), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (bottom - top), 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f / (far - near), 0.0f},
        {-(right + left) / (right - left), -(bottom + top) / (bottom - top), -(near) / (far - near), 1.0f},
    };
}

HgMat4f hg_projection_perspective(f32 fov, f32 aspect, f32 near, f32 far) {
    hg_assert(near > 0.0f);
    hg_assert(far > near);
    f32 scale = 1.0f / std::tan(fov * 0.5f);
    return {
        {scale / aspect, 0.0f, 0.0f, 0.0f},
        {0.0f, scale, 0.0f, 0.0f},
        {0.0f, 0.0f, far / (far - near), 1.0f},
        {0.0f, 0.0f, -(far * near) / (far - near), 0.0f},
    };
}

void *HgStdAllocator::alloc_fn(usize size, usize alignment) {
    (void)alignment;
    return std::malloc(size);
}

void *HgStdAllocator::realloc_fn(void *allocation, usize old_size, usize new_size, usize alignment) {
    (void)old_size;
    (void)alignment;
    return std::realloc(allocation, new_size);
}

void HgStdAllocator::free_fn(void *allocation, usize size, usize alignment) {
    (void)size;
    (void)alignment;
    std::free(allocation);
}

HgArena HgArena::create(HgAllocator& parent, usize capacity) {
    HgArena arena;
    arena.memory = parent.alloc(capacity, 16);
    arena.head = arena.memory.data;
    return arena;
}

void HgArena::destroy(HgAllocator& parent) {
    parent.free(memory, 16);
}

void HgArena::reset() {
    head = memory.data;
}

void *HgArena::alloc_fn(usize size, usize alignment) {
    void *allocation = (void *)hg_align((usize)head, alignment);

    uptr new_head = (uptr)allocation + size;
    if (new_head > (uptr)memory.data + memory.count)
        return nullptr;
    head = (void *)new_head;

    return allocation;
}

void *HgArena::realloc_fn(void *allocation, usize old_size, usize new_size, usize alignment) {
    if ((uptr)allocation + old_size == (uptr)head) {
        uptr new_head = (uptr)allocation + new_size;
        if (new_head > (uptr)memory.data + memory.count)
            return nullptr;
        head = (void *)new_head;
        return allocation;
    }

    if (new_size < old_size)
        return allocation;

    void *new_allocation = alloc_fn(new_size, alignment);
    if (allocation != nullptr && new_allocation != nullptr)
        std::memcpy(new_allocation, allocation, std::min(old_size, new_size));
    return new_allocation;
}

void HgArena::free_fn(void *allocation, usize size, usize alignment) {
    (void)allocation;
    (void)size;
    (void)alignment;
}

hg_test(hg_arena) {
    HgStdAllocator mem;

    HgArena arena = arena.create(mem, 1024);
    hg_defer(arena.destroy(mem));

    for (usize i = 0; i < 3; ++i) {
        hg_test_assert(arena.memory != nullptr);
        hg_test_assert(arena.memory.count == 1024);
        hg_test_assert(arena.head == arena.memory.data);

        u32 *alloc_u32 = arena.alloc<u32>();
        hg_test_assert(alloc_u32 == arena.memory.data);

        void *head = arena.head;
        arena.free(alloc_u32);
        hg_test_assert(alloc_u32 == arena.memory.data);
        hg_test_assert(head == arena.head);

        HgSpan<u64> alloc_u64 = arena.alloc<u64>(2);
        hg_test_assert((u8 *)alloc_u64.data == (u8 *)alloc_u32 + 8);

        u8 *alloc_u8 = arena.alloc<u8>();
        hg_test_assert(alloc_u8 == (u8 *)alloc_u32 + 24);

        struct Big {
            u8 data[32];
        };
        Big *alloc_big = arena.alloc<Big>();
        hg_test_assert((u8 *)alloc_big == (u8 *)alloc_u32 + 25);

        HgSpan<Big> realloc_big = arena.realloc(HgSpan<Big>{alloc_big, 1}, 2);
        hg_test_assert(realloc_big.data == alloc_big);

        memset(realloc_big.data, 2, realloc_big.size());

        HgSpan<Big> realloc_big2 = arena.realloc(realloc_big, 2);
        hg_test_assert(realloc_big2.data == realloc_big.data);
        hg_test_assert(memcmp(realloc_big.data, realloc_big2.data, realloc_big2.size()) == 0);

        u8 *alloc_little = arena.alloc<u8>();
        arena.free(alloc_little);

        realloc_big2 = arena.realloc(realloc_big, 2);
        hg_test_assert(realloc_big2.data != realloc_big.data);
        hg_test_assert(memcmp(realloc_big.data, realloc_big2.data, realloc_big2.size()) == 0);

        HgSpan<Big> big_alloc_big = arena.alloc<Big>(100);
        hg_test_assert(big_alloc_big == nullptr);

        arena.reset();
    }

    return true;
}

HgStack HgStack::create(HgAllocator& parent, usize capacity) {
    HgStack stack;
    stack.memory = parent.alloc(capacity, 16);
    stack.head = 0;
    return stack;
}

void HgStack::destroy(HgAllocator& parent) {
    parent.free(memory, 16);
}

void HgStack::reset() {
    head = 0;
}

void *HgStack::alloc_fn(usize size, usize alignment) {
    (void)alignment;

    usize new_head = head + hg_align(size, 16);
    if (new_head > memory.count)
        return nullptr;

    void *allocation = (u8 *)memory.data + head;
    head = new_head;
    return allocation;
}

void *HgStack::realloc_fn(void *allocation, usize old_size, usize new_size, usize alignment) {
    (void)alignment;

    if ((uptr)allocation + hg_align(old_size, 16) == head) {
        usize new_head = (uptr)allocation - (uptr)memory.data + hg_align(new_size, 16);
        if (new_head > memory.count)
            return nullptr;
        head = new_head;
        return allocation;
    }

    void *new_allocation = alloc_fn(new_size, 16);
    std::memcpy(new_allocation, allocation, std::min(old_size, new_size));
    return new_allocation;
}

void HgStack::free_fn(void *allocation, usize size, usize alignment) {
    (void)alignment;

    if ((uptr)allocation + hg_align(size, 16) == (uptr)memory.data + head) {
        head = (uptr)allocation - (uptr)memory.data;
    } else {
        hg_warn("Attempt to free a stack allocation not at the head\n");
    }
}

hg_test(hg_stack) {
    HgStdAllocator mem;

    HgStack stack = stack.create(mem, 1024);
    hg_defer(stack.destroy(mem));

    for (usize i = 0; i < 3; ++i) {
        hg_test_assert(stack.memory != nullptr);
        hg_test_assert(stack.memory.count == 1024);
        hg_test_assert(stack.head == 0);

        u8 *alloc_u8_1 = stack.alloc<u8>();
        hg_test_assert(alloc_u8_1 == (u8 *)stack.memory.data);

        u8 *alloc_u8_2 = stack.alloc<u8>();
        hg_test_assert(alloc_u8_2 == alloc_u8_1 + 16);

        stack.free(alloc_u8_2);
        u8 *alloc_u8_3 = stack.alloc<u8>();
        hg_test_assert(alloc_u8_3 == alloc_u8_2);

        HgSpan<u64> alloc_u64 = stack.alloc<u64>(2);
        hg_test_assert((u8 *)alloc_u64.data == alloc_u8_3 + 16);

        HgSpan<u64> realloc_u64 = stack.realloc(alloc_u64, 3);
        hg_test_assert(realloc_u64.data = alloc_u64.data);

        std::fill_n(realloc_u64.data, realloc_u64.count, 2);
        (void)stack.alloc<u8>();
        HgSpan<u64> realloc_u64_2 = stack.realloc(realloc_u64, 3);
        hg_test_assert(realloc_u64_2.data != realloc_u64.data);
        hg_test_assert(memcmp(realloc_u64_2.data, realloc_u64.data, realloc_u64.size()) == 0);

        stack.reset();
    }

    return true;
}

hg_test(hg_array) {
    HgStdAllocator mem;

    HgArray<u16> arr_u16 = arr_u16.create(mem, 0, 2);
    hg_defer(arr_u16.destroy(mem));
    hg_test_assert(arr_u16.items != nullptr);
    hg_test_assert(arr_u16.capacity == 2);
    hg_test_assert(arr_u16.count == 0);

    arr_u16.push(mem, (u16)2);
    hg_test_assert(arr_u16[0] == 2);
    hg_test_assert(arr_u16.count >= 1);
    arr_u16.push(mem, (u16)4);
    hg_test_assert(arr_u16[1] == 4);
    hg_test_assert(arr_u16.count == 2);
    hg_test_assert(arr_u16.capacity >= 2);
    arr_u16.push(mem, (u16)8);
    hg_test_assert(arr_u16[2] == 8);
    hg_test_assert(arr_u16.count == 3);
    hg_test_assert(arr_u16.capacity >= 3);

    arr_u16.pop();
    hg_test_assert(arr_u16.count == 2);
    hg_test_assert(arr_u16.capacity >= 3);

    arr_u16.insert(mem, 0, (u16)1);
    hg_test_assert(arr_u16.count == 3);
    hg_test_assert(arr_u16.capacity >= 3);
    hg_test_assert(arr_u16[0] == 1);
    hg_test_assert(arr_u16[1] == 2);
    hg_test_assert(arr_u16[2] == 4);

    arr_u16.remove(1);
    hg_test_assert(arr_u16.count == 2);
    hg_test_assert(arr_u16.capacity >= 3);
    hg_test_assert(arr_u16[0] == 1);
    hg_test_assert(arr_u16[1] == 4);

    for (usize i = 0; i < 100; ++i) {
        arr_u16.push(mem, (u16)i);
    }
    hg_test_assert(arr_u16.count == 102);
    hg_test_assert(arr_u16.capacity >= 102);

    arr_u16.swap_remove(2);
    hg_test_assert(arr_u16.count == 101);
    hg_test_assert(arr_u16[2] == 99);
    hg_test_assert(arr_u16[arr_u16.count - 1] == 98);

    arr_u16.swap_insert(mem, 0, (u16)42);
    hg_test_assert(arr_u16.count == 102);
    hg_test_assert(arr_u16[0] == 42);
    hg_test_assert(arr_u16[1] == 4);
    hg_test_assert(arr_u16[2] == 99);
    hg_test_assert(arr_u16[arr_u16.count - 1] == 1);

    return true;
}

hg_test(hg_hash_map) {
    HgStdAllocator mem;

    constexpr usize count = 128;

    HgHashMap<u32, u32> map = map.create(mem, count);
    hg_defer(map.destroy(mem));

    hg_test_assert(map.load == 0);
    hg_test_assert(!map.has(0));
    hg_test_assert(!map.has(1));
    hg_test_assert(!map.has(12));
    hg_test_assert(!map.has(42));
    hg_test_assert(!map.has(100000));

    map.insert(1, 1);
    hg_test_assert(map.load == 1);
    hg_test_assert(map.has(1));
    hg_test_assert(map.get(1) == 1);
    hg_test_assert(map.try_get(1) != nullptr);
    hg_test_assert(*map.try_get(1) == 1);

    map.remove(1);
    hg_test_assert(map.load == 0);
    hg_test_assert(!map.has(1));
    hg_test_assert(map.try_get(1) == nullptr);

    hg_test_assert(!map.has(12));
    hg_test_assert(!map.has(12 + count));

    map.insert(12, 42);
    hg_test_assert(map.load == 1);
    hg_test_assert(map.has(12) && map.get(12) == 42);
    hg_test_assert(!map.has(12 + count));

    map.insert(12 + count, 100);
    hg_test_assert(map.load == 2);
    hg_test_assert(map.has(12) && map.get(12) == 42);
    hg_test_assert(map.has(12 + count) && map.get(12 + count) == 100);

    map.insert(12 + count * 2, 200);
    hg_test_assert(map.load == 3);
    hg_test_assert(map.has(12) && map.get(12) == 42);
    hg_test_assert(map.has(12 + count) && map.get(12 + count) == 100);
    hg_test_assert(map.has(12 + count * 2) && map.get(12 + count * 2) == 200);

    map.remove(12);
    hg_test_assert(map.load == 2);
    hg_test_assert(!map.has(12));
    hg_test_assert(map.has(12 + count) && map.get(12 + count) == 100);

    map.insert(42, 12);
    hg_test_assert(map.load == 3);
    hg_test_assert(map.has(42) && map.get(42) == 12);

    map.remove(12 + count);
    hg_test_assert(map.load == 2);
    hg_test_assert(!map.has(12));
    hg_test_assert(!map.has(12 + count));

    map.remove(42);
    hg_test_assert(map.load == 1);
    hg_test_assert(!map.has(42));

    map.remove(12 + count * 2);
    hg_test_assert(map.load == 0);
    hg_test_assert(!map.has(12));
    hg_test_assert(!map.has(12 + count));
    hg_test_assert(!map.has(12 + count * 2));

    return true;
}

static u32& hg_internal_current_component_id() {
    static u32 id = 0;
    return id;
}

u32 hg_create_component_id() {
    u32 id = hg_internal_current_component_id();
    ++hg_internal_current_component_id();
    return id;
}

HgECS HgECS::create_ecs(
    HgAllocator& mem,
    u32 max_entities
) {
    HgECS ecs{};
    ecs.entity_pool = mem.alloc<HgEntity>(max_entities);
    ecs.systems = mem.alloc<Component>(hg_internal_current_component_id());

    for (u32 i = 0; i < ecs.entity_pool.count; ++i) {
        ecs.entity_pool[i] = {i + 1};
    }
    ecs.entity_next = {0};

    std::fill_n(ecs.systems.data, ecs.systems.count, Component{});

    return ecs;
}

void HgECS::destroy_ecs(HgAllocator& mem) {
    for (u32 i = 0; i < systems.count; ++i) {
        if (is_registered(i))
            unregister_component(mem, i);
    }
    mem.free(systems);
    mem.free(entity_pool);
}

void HgECS::reset() {
    for (u32 i = 0; i < entity_pool.count; ++i) {
        if (entity_pool[i] == i)
            destroy_entity({i});
        entity_pool[i] = {i + 1};
    }
    entity_next = {0};
}

HgEntity HgECS::create_entity() {
    if (entity_next >= entity_pool.count)
        return {0};

    HgEntity entity = entity_next;
    entity_next = entity_pool[entity];
    entity_pool[entity] = entity;
    return entity;
}

void HgECS::destroy_entity(HgEntity entity) {
    if (!is_alive(entity))
        return;

    for (u32 i = 0; i < systems.count; ++i) {
        if (is_registered(i) && has(entity, i))
            destroy(entity, i);
    }
    entity_pool[entity] = entity_next;
    entity_next = entity;
}

void HgECS::register_component(
    HgAllocator& mem,
    u32 max_components,
    u32 component_size,
    u32 component_alignment,
    u32 component_id
) {
    if (component_id >= systems.count)
        systems = mem.realloc(systems, component_id + 1);

    hg_assert(!is_registered(component_id));

    Component& system = systems[component_id];

    system.user_data = nullptr;
    system.ctor = hg_default_component_ctor;
    system.dtor = hg_default_component_dtor;
    system.entity_indices = mem.alloc<u32>(entity_pool.count);
    system.component_entities = mem.alloc<HgEntity>(max_components);
    system.components = mem.alloc(max_components * component_size, component_alignment);
    system.component_size = component_size;
    system.component_alignment = component_alignment;
    system.component_count = 0;

    std::fill_n(system.entity_indices.data, system.entity_indices.count, (u32)-1);
    std::fill_n(system.component_entities.data, system.component_entities.count, HgEntity{(u32)-1});
}

void HgECS::unregister_component(HgAllocator& mem, u32 component_id) {
    if (!is_registered(component_id))
        return;

    for (u32 i = 0; i < systems[component_id].component_count; ++i) {
        HgEntity entity = systems[component_id].component_entities[i];
        hg_assert(is_alive(entity));
        hg_assert(has(entity, component_id));
        destroy(entity, component_id);
    }

    mem.free(systems[component_id].entity_indices);
    mem.free(systems[component_id].component_entities);
    mem.free(systems[component_id].components, systems[component_id].component_alignment);

    systems[component_id] = {};
}

void HgECS::override_component(void *user_data, Component::Ctor ctor, Component::Dtor dtor, u32 component_id) {
    hg_assert(is_registered(component_id));

    systems[component_id].user_data = user_data;
    if (ctor != nullptr)
        systems[component_id].ctor = ctor;
    if (dtor != nullptr)
        systems[component_id].dtor = dtor;
}

u32 HgECS::smallest_system_untyped(HgSpan<u32> ids) {
    u32 smallest = ids[0];
    hg_assert(is_registered(ids[0]));
    for (usize i = 1; i < ids.count; ++i) {
        hg_assert(is_registered(ids[i]));
        if (systems[ids[i]].component_count < systems[smallest].component_count)
            smallest = ids[i];
    }
    return smallest;
}

void *HgECS::place(u32 index, HgEntity entity, u32 component_id) {
    hg_assert(is_alive(entity));
    hg_assert(is_registered(component_id));
    hg_assert(index < systems[component_id].component_count);

    hg_assert(systems[component_id].entity_indices[entity] == (u32)-1);
    hg_assert(systems[component_id].component_entities[index] == (u32)-1);

    systems[component_id].entity_indices[entity] = index;
    systems[component_id].component_entities[index] = entity;

    return (u8 *)systems[component_id].components.data + index * systems[component_id].component_size;
}

void HgECS::erase(HgEntity entity, u32 component_id) {
    hg_assert(is_alive(entity));
    hg_assert(is_registered(component_id));
    hg_assert(has(entity, component_id));

    u32 index = systems[component_id].entity_indices[entity];
    systems[component_id].entity_indices[entity] = (u32)-1;
    systems[component_id].component_entities[index] = {(u32)-1};
}

void HgECS::move(u32 dst, u32 src, u32 component_id) {
    hg_assert(is_registered(component_id));
    hg_assert(dst < systems[component_id].component_count);
    hg_assert(src < systems[component_id].component_count);

    if (dst == src)
        return;

    hg_assert(systems[component_id].component_entities[dst] == (u32)-1);

    HgEntity entity = std::exchange(systems[component_id].component_entities[src], {(u32)-1});
    systems[component_id].component_entities[dst] = entity;
    systems[component_id].entity_indices[entity] = dst;

    usize size = systems[component_id].component_size;
    void *dst_p = (u8 *)systems[component_id].components.data + dst * size;
    void *src_p = (u8 *)systems[component_id].components.data + src * size;
    memcpy(dst_p, src_p, size);
}

void HgECS::swap(HgEntity lhs, HgEntity rhs, u32 component_id) {
    hg_assert(is_registered(component_id));
    hg_assert(is_alive(lhs));
    hg_assert(is_alive(rhs));
    hg_assert(has(lhs, component_id));
    hg_assert(has(rhs, component_id));

    swap_idx(
        systems[component_id].entity_indices[lhs],
        systems[component_id].entity_indices[rhs],
        component_id);
}

void HgECS::swap_idx(u32 lhs, u32 rhs, u32 component_id) {
    hg_assert(is_registered(component_id));
    Component& system = systems[component_id];
    hg_assert(lhs < system.component_count);
    hg_assert(rhs < system.component_count);

    usize size = system.component_size;
    void *temp = alloca(size);
    memcpy(temp, (u8 *)system.components.data + lhs * size, size);
    memcpy((u8 *)system.components.data + lhs * size, (u8 *)system.components.data + rhs * size, size);
    memcpy((u8 *)system.components.data + rhs * size, temp, size);
}

void HgECS::swap_location(HgEntity lhs, HgEntity rhs, u32 component_id) {
    hg_assert(is_registered(component_id));
    Component& system = systems[component_id];
    hg_assert(is_alive(lhs));
    hg_assert(is_alive(rhs));
    hg_assert(has(lhs, component_id));
    hg_assert(has(rhs, component_id));

    u32 lhs_index = system.entity_indices[lhs];
    u32 rhs_index = system.entity_indices[rhs];

    system.component_entities[lhs_index] = rhs;
    system.component_entities[rhs_index] = lhs;
    system.entity_indices[lhs] = rhs_index;
    system.entity_indices[rhs] = lhs_index;

    swap_idx(lhs_index, rhs_index, component_id);
}

void HgECS::swap_location_idx(u32 lhs, u32 rhs, u32 component_id) {
    hg_assert(is_registered(component_id));
    Component& system = systems[component_id];
    hg_assert(lhs < system.component_count);
    hg_assert(rhs < system.component_count);

    HgEntity lhs_entity = system.component_entities[lhs];
    HgEntity rhs_entity = system.component_entities[rhs];

    hg_assert(is_alive(lhs_entity));
    hg_assert(is_alive(rhs_entity));
    hg_assert(has(lhs_entity, component_id));
    hg_assert(has(rhs_entity, component_id));

    system.component_entities[lhs] = rhs_entity;
    system.component_entities[rhs] = lhs_entity;
    system.entity_indices[lhs_entity] = rhs;
    system.entity_indices[rhs_entity] = lhs;

    swap_idx(lhs, rhs, component_id);
}

void *hg_default_component_ctor(void *user_data, HgECS& ecs, HgEntity entity, u32 component_id) {
    (void)user_data;

    hg_assert(ecs.is_alive(entity));
    hg_assert(ecs.is_registered(component_id));

    u32 index = ecs.systems[component_id].component_count++;
    return ecs.place(index, entity, component_id);
}

void hg_default_component_dtor(void *user_data, HgECS& ecs, HgEntity entity, u32 component_id) {
    (void)user_data;

    hg_assert(ecs.is_alive(entity));
    hg_assert(ecs.is_registered(component_id));
    hg_assert(ecs.has(entity, component_id));

    u32 index = ecs.systems[component_id].entity_indices[entity];
    u32 last_index = ecs.systems[component_id].component_count - 1;

    ecs.erase(entity, component_id);
    ecs.move(index, last_index, component_id);
    --ecs.systems[component_id].component_count;
}

static void *hg_internal_test_component_ctor(void *user_data, HgECS& ecs, HgEntity entity, u32 component_id) {
    ++*(u32 *)user_data;
    return hg_default_component_ctor(user_data, ecs, entity, component_id);
}

static void hg_internal_test_component_dtor(void *user_data, HgECS& ecs, HgEntity entity, u32 component_id) {
    --*(u32 *)user_data;
    hg_default_component_dtor(user_data, ecs, entity, component_id);
}

hg_test(hg_ecs) {
    HgStdAllocator mem;

    HgECS ecs = ecs.create_ecs(mem, 1 << 16);

    ecs.register_component<u32>(mem, 1 << 16);
    ecs.register_component<u64>(mem, 1 << 16);

    u32 u32_comp_count = 0;
    ecs.override_component<u32>(&u32_comp_count, hg_internal_test_component_ctor, hg_internal_test_component_dtor);
    u32 u64_comp_count = 0;
    ecs.override_component<u64>(&u64_comp_count, hg_internal_test_component_ctor, hg_internal_test_component_dtor);

    HgEntity e1 = ecs.create_entity();
    HgEntity e2 = ecs.create_entity();
    HgEntity e3;
    hg_test_assert(e1 == 0);
    hg_test_assert(e2 == 1);
    hg_test_assert(ecs.is_alive(e1));
    hg_test_assert(ecs.is_alive(e2));
    hg_test_assert(!ecs.is_alive(e3));

    ecs.destroy_entity(e1);
    hg_test_assert(!ecs.is_alive(e1));
    e3 = ecs.create_entity();
    hg_test_assert(ecs.is_alive(e3));
    hg_test_assert(e3 == e1);

    e1 = ecs.create_entity();
    hg_test_assert(ecs.is_alive(e1));
    hg_test_assert(e1 == 2);

    bool has_unknown = false;
    ecs.for_each<u32>([&](HgEntity, u32&) {
        has_unknown = true;
    });
    hg_test_assert(!has_unknown);

    hg_test_assert(ecs.component_count<u32>() == 0);
    hg_test_assert(ecs.component_count<u64>() == 0);

    hg_assert(u32_comp_count == 0);
    ecs.create<u32>(e1) = 12;
    ecs.create<u32>(e2) = 42;
    ecs.create<u32>(e3) = 100;
    hg_assert(u32_comp_count == 3);
    hg_test_assert(ecs.component_count<u32>() == 3);
    hg_test_assert(ecs.component_count<u64>() == 0);

    bool has_12 = false;
    bool has_42 = false;
    bool has_100 = false;
    for (auto [e, c] : ecs.component_iter<u32>()) {
        switch (c) {
            case 12:
                has_12 = e == e1;
                break;
            case 42:
                has_42 = e == e2;
                break;
            case 100:
                has_100 = e == e3;
                break;
            default:
                has_unknown = true;
                break;
        }
    }
    hg_test_assert(has_12);
    hg_test_assert(has_42);
    hg_test_assert(has_100);
    hg_test_assert(!has_unknown);

    hg_assert(u64_comp_count == 0);
    ecs.create<u64>(e2) = 2042;
    ecs.create<u64>(e3) = 2100;
    hg_assert(u64_comp_count == 2);
    hg_test_assert(ecs.component_count<u32>() == 3);
    hg_test_assert(ecs.component_count<u64>() == 2);

    has_12 = false;
    has_42 = false;
    has_100 = false;
    bool has_2042 = false;
    bool has_2100 = false;
    ecs.for_each<u32, u64>([&](HgEntity e, u32& comp32, u64& comp64) {
        switch (comp32) {
            case 12:
                has_12 = e == e1;
                break;
            case 42:
                has_42 = e == e2;
                break;
            case 100:
                has_100 = e == e3;
                break;
            default:
                has_unknown = true;
                break;
        }
        switch (comp64) {
            case 2042:
                has_2042 = e == e2;
                break;
            case 2100:
                has_2100 = e == e3;
                break;
            default:
                has_unknown = true;
                break;
        }
    });
    hg_test_assert(!has_12);
    hg_test_assert(has_42);
    hg_test_assert(has_100);
    hg_test_assert(has_2042);
    hg_test_assert(has_2100);
    hg_test_assert(!has_unknown);

    hg_assert(u32_comp_count == 3);
    ecs.destroy_entity(e1);
    hg_assert(u32_comp_count == 2);
    hg_test_assert(ecs.component_count<u32>() == 2);
    hg_test_assert(ecs.component_count<u64>() == 2);

    has_12 = false;
    has_42 = false;
    has_100 = false;
    ecs.for_each<u32>([&](HgEntity e, u32& c) {
        switch (c) {
            case 12:
                has_12 = e == e1;
                break;
            case 42:
                has_42 = e == e2;
                break;
            case 100:
                has_100 = e == e3;
                break;
            default:
                has_unknown = true;
                break;
        }
    });
    hg_test_assert(!has_12);
    hg_test_assert(has_42);
    hg_test_assert(has_100);
    hg_test_assert(!has_unknown);

    hg_assert(u32_comp_count == 2);
    hg_assert(u64_comp_count == 2);
    ecs.destroy_entity(e2);
    hg_assert(u32_comp_count == 1);
    hg_assert(u64_comp_count == 1);
    hg_test_assert(ecs.component_count<u32>() == 1);
    hg_test_assert(ecs.component_count<u64>() == 1);

    ecs.destroy_ecs(mem);
    hg_test_assert(u32_comp_count == 0);
    hg_test_assert(u64_comp_count == 0);

    return true;
}

hg_test(hg_ecs_quicksort) {
    HgStdAllocator mem;

    HgECS ecs = ecs.create_ecs(mem, 128);
    hg_defer(ecs.destroy_ecs(mem));

    u32 elem;
    ecs.register_component<u32>(mem, 128);

    auto comparison = [](u32 lhs, u32 rhs) {
        return lhs < rhs;
    };

    ecs.create<u32>(ecs.create_entity()) = 42;
    ecs.quicksort_components<u32>(comparison);

    for (auto [e, c] : ecs.component_iter<u32>()) {
        (void) e;
        hg_test_assert(c == 42);
    }

    ecs.reset();

    u32 small_scramble_1[] = {1, 0};
    for (u32 i = 0; i < hg_countof(small_scramble_1); ++i) {
        ecs.create<u32>(ecs.create_entity()) = small_scramble_1[i];
    }
    ecs.quicksort_components<u32>(comparison);

    elem = 0;
    for (auto [e, c] : ecs.component_iter<u32>()) {
        (void)e;
        hg_test_assert(c == elem);
        ++elem;
    }
    ecs.quicksort_components<u32>(comparison);

    elem = 0;
    for (auto [e, c] : ecs.component_iter<u32>()) {
        (void)e;
        hg_test_assert(c == elem);
        ++elem;
    }

    ecs.reset();

    u32 medium_scramble_1[] = {8, 9, 1, 6, 0, 3, 7, 2, 5, 4};
    for (u32 i = 0; i < hg_countof(medium_scramble_1); ++i) {
        ecs.create<u32>(ecs.create_entity()) = medium_scramble_1[i];
    }
    ecs.quicksort_components<u32>(comparison);

    elem = 0;
    for (auto [e, c] : ecs.component_iter<u32>()) {
        (void)e;
        hg_test_assert(c == elem);
        ++elem;
    }

    ecs.reset();

    u32 medium_scramble_2[] = {3, 9, 7, 6, 8, 5, 0, 1, 2, 4};
    for (u32 i = 0; i < hg_countof(medium_scramble_2); ++i) {
        ecs.create<u32>(ecs.create_entity()) = medium_scramble_2[i];
    }
    ecs.quicksort_components<u32>(comparison);
    ecs.quicksort_components<u32>(comparison);

    elem = 0;
    for (auto [e, c] : ecs.component_iter<u32>()) {
        (void)e;
        hg_test_assert(c == elem);
        ++elem;
    }

    ecs.reset();

    for (u32 i = 127; i < 128; --i) {
        ecs.create<u32>(ecs.create_entity()) = i;
    }
    ecs.quicksort_components<u32>(comparison);

    elem = 0;
    for (auto [e, c] : ecs.component_iter<u32>()) {
        (void)e;
        hg_test_assert(c == elem);
        ++elem;
    }

    ecs.reset();

    for (u32 i = 127; i < 128; --i) {
        ecs.create<u32>(ecs.create_entity()) = i / 2;
    }
    ecs.quicksort_components<u32>(comparison);
    ecs.quicksort_components<u32>(comparison);

    elem = 0;
    for (auto [e, c] : ecs.component_iter<u32>()) {
        (void)e;
        hg_test_assert(c == elem / 2);
        ++elem;
    }

    return true;
}

f64 HgClock::tick() {
    auto prev = std::exchange(time, std::chrono::high_resolution_clock::now());
    return std::chrono::duration<f64>{time - prev}.count();
}

HgSpan<void> hg_file_load_binary(HgAllocator& allocator, const char *path) {
    FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        hg_warn("Could not find file to read binary: %s\n", path);
        return {};
    }
    hg_defer(std::fclose(file));

    if (std::fseek(file, 0, SEEK_END) != 0) {
        hg_warn("Failed to read binary from file: %s\n", path);
        return {};
    }

    HgSpan<void> data = allocator.alloc((usize)std::ftell(file), 16);
    std::rewind(file);

    if (std::fread(data.data, 1, data.count, file) != data.count) {
        hg_warn("Failed to read binary from file: %s\n", path);
        return {};
    }

    return data;
}

void hg_file_unload_binary(HgAllocator& allocator, HgSpan<void> data) {
    allocator.free(data, 16);
}

bool hg_file_save_binary(HgSpan<const void> data, const char *path) {
    FILE* file = std::fopen(path, "wb");
    if (file == nullptr) {
        hg_warn("Failed to create file to write binary: %s\n", path);
        return false;
    }
    hg_defer(std::fclose(file));

    if (std::fwrite(data.data, 1, data.count, file) != data.count) {
        hg_warn("Failed to write binary data to file: %s\n", path);
        return false;
    }

    return true;
}

hg_test(hg_file_binary) {
    HgStdAllocator mem;

    u32 save_data[] = {12, 42, 100, 128};

    hg_test_assert(!hg_file_save_binary({save_data, sizeof(save_data)}, "dir/does/not/exist.bin"));
    hg_test_assert(hg_file_load_binary(mem, "file_does_not_exist.bin") == nullptr);

    hg_test_assert(hg_file_save_binary({save_data, sizeof(save_data)}, "hg_file_test.bin"));
    HgSpan<void> load_data = hg_file_load_binary(mem, "hg_file_test.bin");
    hg_defer(hg_file_unload_binary(mem, load_data));

    hg_test_assert(load_data != nullptr);
    hg_test_assert(load_data.size() == sizeof(save_data));
    hg_test_assert(memcmp(save_data, load_data.data, load_data.size()) == 0);

    return true;
}

#define HG_MAKE_VULKAN_FUNC(name) PFN_##name name

struct HgVulkanFuncs {
    HG_MAKE_VULKAN_FUNC(vkGetInstanceProcAddr);
    HG_MAKE_VULKAN_FUNC(vkGetDeviceProcAddr);

    HG_MAKE_VULKAN_FUNC(vkCreateInstance);
    HG_MAKE_VULKAN_FUNC(vkDestroyInstance);

    HG_MAKE_VULKAN_FUNC(vkCreateDebugUtilsMessengerEXT);
    HG_MAKE_VULKAN_FUNC(vkDestroyDebugUtilsMessengerEXT);

    HG_MAKE_VULKAN_FUNC(vkEnumeratePhysicalDevices);
    HG_MAKE_VULKAN_FUNC(vkEnumerateDeviceExtensionProperties);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceProperties);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);

    HG_MAKE_VULKAN_FUNC(vkDestroySurfaceKHR);
    HG_MAKE_VULKAN_FUNC(vkCreateDevice);

    HG_MAKE_VULKAN_FUNC(vkDestroyDevice);
    HG_MAKE_VULKAN_FUNC(vkDeviceWaitIdle);

    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    HG_MAKE_VULKAN_FUNC(vkCreateSwapchainKHR);
    HG_MAKE_VULKAN_FUNC(vkDestroySwapchainKHR);
    HG_MAKE_VULKAN_FUNC(vkGetSwapchainImagesKHR);
    HG_MAKE_VULKAN_FUNC(vkAcquireNextImageKHR);

    HG_MAKE_VULKAN_FUNC(vkCreateSemaphore);
    HG_MAKE_VULKAN_FUNC(vkDestroySemaphore);
    HG_MAKE_VULKAN_FUNC(vkCreateFence);
    HG_MAKE_VULKAN_FUNC(vkDestroyFence);
    HG_MAKE_VULKAN_FUNC(vkResetFences);
    HG_MAKE_VULKAN_FUNC(vkWaitForFences);

    HG_MAKE_VULKAN_FUNC(vkGetDeviceQueue);
    HG_MAKE_VULKAN_FUNC(vkQueueWaitIdle);
    HG_MAKE_VULKAN_FUNC(vkQueueSubmit);
    HG_MAKE_VULKAN_FUNC(vkQueuePresentKHR);

    HG_MAKE_VULKAN_FUNC(vkCreateCommandPool);
    HG_MAKE_VULKAN_FUNC(vkDestroyCommandPool);
    HG_MAKE_VULKAN_FUNC(vkAllocateCommandBuffers);
    HG_MAKE_VULKAN_FUNC(vkFreeCommandBuffers);

    HG_MAKE_VULKAN_FUNC(vkCreateDescriptorPool);
    HG_MAKE_VULKAN_FUNC(vkDestroyDescriptorPool);
    HG_MAKE_VULKAN_FUNC(vkResetDescriptorPool);
    HG_MAKE_VULKAN_FUNC(vkAllocateDescriptorSets);
    HG_MAKE_VULKAN_FUNC(vkFreeDescriptorSets);
    HG_MAKE_VULKAN_FUNC(vkUpdateDescriptorSets);

    HG_MAKE_VULKAN_FUNC(vkCreateDescriptorSetLayout);
    HG_MAKE_VULKAN_FUNC(vkDestroyDescriptorSetLayout);
    HG_MAKE_VULKAN_FUNC(vkCreatePipelineLayout);
    HG_MAKE_VULKAN_FUNC(vkDestroyPipelineLayout);
    HG_MAKE_VULKAN_FUNC(vkCreateShaderModule);
    HG_MAKE_VULKAN_FUNC(vkDestroyShaderModule);
    HG_MAKE_VULKAN_FUNC(vkCreateGraphicsPipelines);
    HG_MAKE_VULKAN_FUNC(vkCreateComputePipelines);
    HG_MAKE_VULKAN_FUNC(vkDestroyPipeline);

    HG_MAKE_VULKAN_FUNC(vkCreateBuffer);
    HG_MAKE_VULKAN_FUNC(vkDestroyBuffer);
    HG_MAKE_VULKAN_FUNC(vkCreateImage);
    HG_MAKE_VULKAN_FUNC(vkDestroyImage);
    HG_MAKE_VULKAN_FUNC(vkCreateImageView);
    HG_MAKE_VULKAN_FUNC(vkDestroyImageView);
    HG_MAKE_VULKAN_FUNC(vkCreateSampler);
    HG_MAKE_VULKAN_FUNC(vkDestroySampler);

    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceMemoryProperties);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceMemoryProperties2);
    HG_MAKE_VULKAN_FUNC(vkGetBufferMemoryRequirements);
    HG_MAKE_VULKAN_FUNC(vkGetBufferMemoryRequirements2);
    HG_MAKE_VULKAN_FUNC(vkGetImageMemoryRequirements);
    HG_MAKE_VULKAN_FUNC(vkGetImageMemoryRequirements2);
    HG_MAKE_VULKAN_FUNC(vkGetDeviceBufferMemoryRequirements);
    HG_MAKE_VULKAN_FUNC(vkGetDeviceImageMemoryRequirements);

    HG_MAKE_VULKAN_FUNC(vkAllocateMemory);
    HG_MAKE_VULKAN_FUNC(vkFreeMemory);
    HG_MAKE_VULKAN_FUNC(vkBindBufferMemory);
    HG_MAKE_VULKAN_FUNC(vkBindBufferMemory2);
    HG_MAKE_VULKAN_FUNC(vkBindImageMemory);
    HG_MAKE_VULKAN_FUNC(vkBindImageMemory2);
    HG_MAKE_VULKAN_FUNC(vkMapMemory);
    HG_MAKE_VULKAN_FUNC(vkUnmapMemory);
    HG_MAKE_VULKAN_FUNC(vkFlushMappedMemoryRanges);
    HG_MAKE_VULKAN_FUNC(vkInvalidateMappedMemoryRanges);

    HG_MAKE_VULKAN_FUNC(vkBeginCommandBuffer);
    HG_MAKE_VULKAN_FUNC(vkEndCommandBuffer);
    HG_MAKE_VULKAN_FUNC(vkResetCommandBuffer);

    HG_MAKE_VULKAN_FUNC(vkCmdCopyBuffer);
    HG_MAKE_VULKAN_FUNC(vkCmdCopyImage);
    HG_MAKE_VULKAN_FUNC(vkCmdBlitImage);
    HG_MAKE_VULKAN_FUNC(vkCmdCopyBufferToImage);
    HG_MAKE_VULKAN_FUNC(vkCmdCopyImageToBuffer);
    HG_MAKE_VULKAN_FUNC(vkCmdPipelineBarrier2);

    HG_MAKE_VULKAN_FUNC(vkCmdBeginRendering);
    HG_MAKE_VULKAN_FUNC(vkCmdEndRendering);
    HG_MAKE_VULKAN_FUNC(vkCmdSetViewport);
    HG_MAKE_VULKAN_FUNC(vkCmdSetScissor);
    HG_MAKE_VULKAN_FUNC(vkCmdBindPipeline);
    HG_MAKE_VULKAN_FUNC(vkCmdBindDescriptorSets);
    HG_MAKE_VULKAN_FUNC(vkCmdPushConstants);
    HG_MAKE_VULKAN_FUNC(vkCmdBindVertexBuffers);
    HG_MAKE_VULKAN_FUNC(vkCmdBindIndexBuffer);
    HG_MAKE_VULKAN_FUNC(vkCmdDraw);
    HG_MAKE_VULKAN_FUNC(vkCmdDrawIndexed);
    HG_MAKE_VULKAN_FUNC(vkCmdDispatch);
};

static HgVulkanFuncs hg_internal_vulkan_funcs{};

PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance instance, const char *pName) {
    return hg_internal_vulkan_funcs.vkGetInstanceProcAddr(instance, pName);
}

PFN_vkVoidFunction vkGetDeviceProcAddr(VkDevice device, const char *pName) {
    return hg_internal_vulkan_funcs.vkGetDeviceProcAddr(device, pName);
}

VkResult vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) {
    return hg_internal_vulkan_funcs.vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

void vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyInstance(instance, pAllocator);
}

VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger) {
    return hg_internal_vulkan_funcs.vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

void vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

VkResult vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *pCount, VkPhysicalDevice *pDevices) {
    return hg_internal_vulkan_funcs.vkEnumeratePhysicalDevices(instance, pCount, pDevices);
}

VkResult vkEnumerateDeviceExtensionProperties(VkPhysicalDevice device, const char *pLayerName, uint32_t *pCount, VkExtensionProperties *pProps) {
    return hg_internal_vulkan_funcs.vkEnumerateDeviceExtensionProperties(device, pLayerName, pCount, pProps);
}

void vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties) {
    hg_internal_vulkan_funcs.vkGetPhysicalDeviceProperties(physicalDevice, pProperties);
}

void vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice device, uint32_t *pCount, VkQueueFamilyProperties *pProps) {
    hg_internal_vulkan_funcs.vkGetPhysicalDeviceQueueFamilyProperties(device, pCount, pProps);
}

void vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroySurfaceKHR(instance, surface, pAllocator);
}

VkResult vkCreateDevice(VkPhysicalDevice device, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    return hg_internal_vulkan_funcs.vkCreateDevice(device, pCreateInfo, pAllocator, pDevice);
}

void vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyDevice(device, pAllocator);
}

VkResult vkDeviceWaitIdle(VkDevice device) {
    return hg_internal_vulkan_funcs.vkDeviceWaitIdle(device);
}

VkResult vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t *pCount, VkSurfaceFormatKHR *pFormats) {
    return hg_internal_vulkan_funcs.vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, pCount, pFormats);
}

VkResult vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t *pCount, VkPresentModeKHR *pModes) {
    return hg_internal_vulkan_funcs.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, pCount, pModes);
}

VkResult vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice device, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pCaps) {
    return hg_internal_vulkan_funcs.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, pCaps);
}

VkResult vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain) {
    return hg_internal_vulkan_funcs.vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
}

void vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroySwapchainKHR(device, swapchain, pAllocator);
}

VkResult vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pCount, VkImage *pImages) {
    return hg_internal_vulkan_funcs.vkGetSwapchainImagesKHR(device, swapchain, pCount, pImages);
}

VkResult vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore sem, VkFence fence, uint32_t *pIndex) {
    return hg_internal_vulkan_funcs.vkAcquireNextImageKHR(device, swapchain, timeout, sem, fence, pIndex);
}

VkResult vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSemaphore *pSemaphore) {
    return hg_internal_vulkan_funcs.vkCreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
}

void vkDestroySemaphore(VkDevice device, VkSemaphore sem, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroySemaphore(device, sem, pAllocator);
}

VkResult vkCreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFence *pFence) {
    return hg_internal_vulkan_funcs.vkCreateFence(device, pCreateInfo, pAllocator, pFence);
}

void vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyFence(device, fence, pAllocator);
}

VkResult vkResetFences(VkDevice device, uint32_t count, const VkFence *pFences) {
    return hg_internal_vulkan_funcs.vkResetFences(device, count, pFences);
}

VkResult vkWaitForFences(VkDevice device, uint32_t count, const VkFence *pFences, VkBool32 waitAll, uint64_t timeout) {
    return hg_internal_vulkan_funcs.vkWaitForFences(device, count, pFences, waitAll, timeout);
}

void vkGetDeviceQueue(VkDevice device, uint32_t family, uint32_t index, VkQueue *pQueue) {
    hg_internal_vulkan_funcs.vkGetDeviceQueue(device, family, index, pQueue);
}

VkResult vkQueueWaitIdle(VkQueue queue) {
    return hg_internal_vulkan_funcs.vkQueueWaitIdle(queue);
}

VkResult vkQueueSubmit(VkQueue queue, uint32_t count, const VkSubmitInfo *pSubmits, VkFence fence) {
    return hg_internal_vulkan_funcs.vkQueueSubmit(queue, count, pSubmits, fence);
}

VkResult vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pInfo) {
    return hg_internal_vulkan_funcs.vkQueuePresentKHR(queue, pInfo);
}

VkResult vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCommandPool *pPool) {
    return hg_internal_vulkan_funcs.vkCreateCommandPool(device, pCreateInfo, pAllocator, pPool);
}

void vkDestroyCommandPool(VkDevice device, VkCommandPool pool, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyCommandPool(device, pool, pAllocator);
}

VkResult vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pInfo, VkCommandBuffer *pBufs) {
    return hg_internal_vulkan_funcs.vkAllocateCommandBuffers(device, pInfo, pBufs);
}

void vkFreeCommandBuffers(VkDevice device, VkCommandPool pool, uint32_t count, const VkCommandBuffer *pBufs) {
    hg_internal_vulkan_funcs.vkFreeCommandBuffers(device, pool, count, pBufs);
}

VkResult vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorPool *pPool) {
    return hg_internal_vulkan_funcs.vkCreateDescriptorPool(device, pInfo, pAllocator, pPool);
}

void vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool pool, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyDescriptorPool(device, pool, pAllocator);
}

VkResult vkResetDescriptorPool(VkDevice device, VkDescriptorPool pool, uint32_t flags) {
    return hg_internal_vulkan_funcs.vkResetDescriptorPool(device, pool, flags);
}

VkResult vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pInfo, VkDescriptorSet *pSets) {
    return hg_internal_vulkan_funcs.vkAllocateDescriptorSets(device, pInfo, pSets);
}

VkResult vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets) {
    return hg_internal_vulkan_funcs.vkFreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

void vkUpdateDescriptorSets(VkDevice device, uint32_t writeCount, const VkWriteDescriptorSet *pWrites, uint32_t copyCount, const VkCopyDescriptorSet *pCopies) {
    hg_internal_vulkan_funcs.vkUpdateDescriptorSets(device, writeCount, pWrites, copyCount, pCopies);
}

VkResult vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorSetLayout *pLayout) {
    return hg_internal_vulkan_funcs.vkCreateDescriptorSetLayout(device, pInfo, pAllocator, pLayout);
}

void vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout layout, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyDescriptorSetLayout(device, layout, pAllocator);
}

VkResult vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pLayout) {
    return hg_internal_vulkan_funcs.vkCreatePipelineLayout(device, pInfo, pAllocator, pLayout);
}

void vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout layout, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyPipelineLayout(device, layout, pAllocator);
}

VkResult vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkShaderModule *pModule) {
    return hg_internal_vulkan_funcs.vkCreateShaderModule(device, pInfo, pAllocator, pModule);
}

void vkDestroyShaderModule(VkDevice device, VkShaderModule module, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyShaderModule(device, module, pAllocator);
}

VkResult vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache cache, uint32_t count, const VkGraphicsPipelineCreateInfo *pInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    return hg_internal_vulkan_funcs.vkCreateGraphicsPipelines(device, cache, count, pInfos, pAllocator, pPipelines);
}

VkResult vkCreateComputePipelines(VkDevice device, VkPipelineCache cache, uint32_t count, const VkComputePipelineCreateInfo *pInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    return hg_internal_vulkan_funcs.vkCreateComputePipelines(device, cache, count, pInfos, pAllocator, pPipelines);
}

void vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyPipeline(device, pipeline, pAllocator);
}

VkResult vkCreateBuffer(VkDevice device, const VkBufferCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkBuffer *pBuf) {
    return hg_internal_vulkan_funcs.vkCreateBuffer(device, pInfo, pAllocator, pBuf);
}

void vkDestroyBuffer(VkDevice device, VkBuffer buf, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyBuffer(device, buf, pAllocator);
}

VkResult vkCreateImage(VkDevice device, const VkImageCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkImage *pImage) {
    return hg_internal_vulkan_funcs.vkCreateImage(device, pInfo, pAllocator, pImage);
}

void vkDestroyImage(VkDevice device, VkImage img, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyImage(device, img, pAllocator);
}

VkResult vkCreateImageView(VkDevice device, const VkImageViewCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkImageView *pView) {
    return hg_internal_vulkan_funcs.vkCreateImageView(device, pInfo, pAllocator, pView);
}

void vkDestroyImageView(VkDevice device, VkImageView view, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroyImageView(device, view, pAllocator);
}

VkResult vkCreateSampler(VkDevice device, const VkSamplerCreateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkSampler *pSampler) {
    return hg_internal_vulkan_funcs.vkCreateSampler(device, pInfo, pAllocator, pSampler);
}

void vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkDestroySampler(device, sampler, pAllocator);
}

void vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties *pMemoryProperties) {
    hg_internal_vulkan_funcs.vkGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}

void vkGetPhysicalDeviceMemoryProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2*pMemoryProperties) {
    hg_internal_vulkan_funcs.vkGetPhysicalDeviceMemoryProperties2(physicalDevice, pMemoryProperties);
}

void vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements *pMemoryRequirements) {
    hg_internal_vulkan_funcs.vkGetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}

void vkGetBufferMemoryRequirements2(VkDevice device, const VkBufferMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements) {
    hg_internal_vulkan_funcs.vkGetBufferMemoryRequirements2(device, pInfo, pMemoryRequirements);
}

void vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements *pMemoryRequirements) {
    hg_internal_vulkan_funcs.vkGetImageMemoryRequirements(device, image, pMemoryRequirements);
}

void vkGetImageMemoryRequirements2(VkDevice device, const VkImageMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements) {
    hg_internal_vulkan_funcs.vkGetImageMemoryRequirements2(device, pInfo, pMemoryRequirements);
}

void vkGetDeviceBufferMemoryRequirements(VkDevice device, const VkDeviceBufferMemoryRequirements* pInfo, VkMemoryRequirements2* pMemoryRequirements) {
    hg_internal_vulkan_funcs.vkGetDeviceBufferMemoryRequirements(device, pInfo, pMemoryRequirements);
}

void vkGetDeviceImageMemoryRequirements(VkDevice device, const VkDeviceImageMemoryRequirements* pInfo, VkMemoryRequirements2* pMemoryRequirements) {
    hg_internal_vulkan_funcs.vkGetDeviceImageMemoryRequirements(device, pInfo, pMemoryRequirements);
}

VkResult vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pInfo, const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory) {
    return hg_internal_vulkan_funcs.vkAllocateMemory(device, pInfo, pAllocator, pMemory);
}

void vkFreeMemory(VkDevice device, VkDeviceMemory mem, const VkAllocationCallbacks *pAllocator) {
    hg_internal_vulkan_funcs.vkFreeMemory(device, mem, pAllocator);
}

VkResult vkBindBufferMemory(VkDevice device, VkBuffer buf, VkDeviceMemory mem, VkDeviceSize offset) {
    return hg_internal_vulkan_funcs.vkBindBufferMemory(device, buf, mem, offset);
}

VkResult vkBindBufferMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo* pBindInfos) {
    return hg_internal_vulkan_funcs.vkBindBufferMemory2(device, bindInfoCount, pBindInfos);
}

VkResult vkBindImageMemory(VkDevice device, VkImage img, VkDeviceMemory mem, VkDeviceSize offset) {
    return hg_internal_vulkan_funcs.vkBindImageMemory(device, img, mem, offset);
}

VkResult vkBindImageMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo* pBindInfos) {
    return hg_internal_vulkan_funcs.vkBindImageMemory2(device, bindInfoCount, pBindInfos);
}

VkResult vkMapMemory(VkDevice device, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void **ppData) {
    return hg_internal_vulkan_funcs.vkMapMemory(device, mem, offset, size, flags, ppData);
}

void vkUnmapMemory(VkDevice device, VkDeviceMemory mem) {
    hg_internal_vulkan_funcs.vkUnmapMemory(device, mem);
}

VkResult vkFlushMappedMemoryRanges(VkDevice device, uint32_t count, const VkMappedMemoryRange *pRanges) {
    return hg_internal_vulkan_funcs.vkFlushMappedMemoryRanges(device, count, pRanges);
}

VkResult vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t count, const VkMappedMemoryRange *pRanges) {
    return hg_internal_vulkan_funcs.vkInvalidateMappedMemoryRanges(device, count, pRanges);
}

VkResult vkBeginCommandBuffer(VkCommandBuffer cmd, const VkCommandBufferBeginInfo *pInfo) {
    return hg_internal_vulkan_funcs.vkBeginCommandBuffer(cmd, pInfo);
}

VkResult vkEndCommandBuffer(VkCommandBuffer cmd) {
    return hg_internal_vulkan_funcs.vkEndCommandBuffer(cmd);
}

VkResult vkResetCommandBuffer(VkCommandBuffer cmd, VkCommandBufferResetFlags flags) {
    return hg_internal_vulkan_funcs.vkResetCommandBuffer(cmd, flags);
}

void vkCmdCopyBuffer(VkCommandBuffer cmd, VkBuffer src, VkBuffer dst, uint32_t count, const VkBufferCopy *pRegions) {
    hg_internal_vulkan_funcs.vkCmdCopyBuffer(cmd, src, dst, count, pRegions);
}

void vkCmdCopyImage(VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkImage dst, VkImageLayout dstLayout, uint32_t count, const VkImageCopy *pRegions) {
    hg_internal_vulkan_funcs.vkCmdCopyImage(cmd, src, srcLayout, dst, dstLayout, count, pRegions);
}

void vkCmdBlitImage(VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkImage dst, VkImageLayout dstLayout, uint32_t count, const VkImageBlit *pRegions, VkFilter filter) {
    hg_internal_vulkan_funcs.vkCmdBlitImage(cmd, src, srcLayout, dst, dstLayout, count, pRegions, filter);
}

void vkCmdCopyBufferToImage(VkCommandBuffer cmd, VkBuffer src, VkImage dst, VkImageLayout dstLayout, uint32_t count, const VkBufferImageCopy *pRegions) {
    hg_internal_vulkan_funcs.vkCmdCopyBufferToImage(cmd, src, dst, dstLayout, count, pRegions);
}

void vkCmdCopyImageToBuffer(VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkBuffer dst, uint32_t count, const VkBufferImageCopy *pRegions) {
    hg_internal_vulkan_funcs.vkCmdCopyImageToBuffer(cmd, src, srcLayout, dst, count, pRegions);
}

void vkCmdPipelineBarrier2(VkCommandBuffer cmd, const VkDependencyInfo *pInfo) {
    hg_internal_vulkan_funcs.vkCmdPipelineBarrier2(cmd, pInfo);
}

void vkCmdBeginRendering(VkCommandBuffer cmd, const VkRenderingInfo *pInfo) {
    hg_internal_vulkan_funcs.vkCmdBeginRendering(cmd, pInfo);
}

void vkCmdEndRendering(VkCommandBuffer cmd) {
    hg_internal_vulkan_funcs.vkCmdEndRendering(cmd);
}

void vkCmdSetViewport(VkCommandBuffer cmd, uint32_t first, uint32_t count, const VkViewport *pViewports) {
    hg_internal_vulkan_funcs.vkCmdSetViewport(cmd, first, count, pViewports);
}

void vkCmdSetScissor(VkCommandBuffer cmd, uint32_t first, uint32_t count, const VkRect2D *pScissors) {
    hg_internal_vulkan_funcs.vkCmdSetScissor(cmd, first, count, pScissors);
}

void vkCmdBindPipeline(VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, VkPipeline pipeline) {
    hg_internal_vulkan_funcs.vkCmdBindPipeline(cmd, bindPoint, pipeline);
}

void vkCmdBindDescriptorSets(VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t count, const VkDescriptorSet *pSets, uint32_t dynCount, const uint32_t *pDyn) {
    hg_internal_vulkan_funcs.vkCmdBindDescriptorSets(cmd, bindPoint, layout, firstSet, count, pSets, dynCount, pDyn);
}

void vkCmdPushConstants(VkCommandBuffer cmd, VkPipelineLayout layout, VkShaderStageFlags stages, uint32_t offset, uint32_t size, const void *pData) {
    hg_internal_vulkan_funcs.vkCmdPushConstants(cmd, layout, stages, offset, size, pData);
}

void vkCmdBindVertexBuffers(VkCommandBuffer cmd, uint32_t first, uint32_t count, const VkBuffer *pBufs, const VkDeviceSize *pOffsets) {
    hg_internal_vulkan_funcs.vkCmdBindVertexBuffers(cmd, first, count, pBufs, pOffsets);
}

void vkCmdBindIndexBuffer(VkCommandBuffer cmd, VkBuffer buf, VkDeviceSize offset, VkIndexType type) {
    hg_internal_vulkan_funcs.vkCmdBindIndexBuffer(cmd, buf, offset, type);
}

void vkCmdDraw(VkCommandBuffer cmd, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    hg_internal_vulkan_funcs.vkCmdDraw(cmd, vertexCount, instanceCount, firstVertex, firstInstance);
}

void vkCmdDrawIndexed(VkCommandBuffer cmd, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    hg_internal_vulkan_funcs.vkCmdDrawIndexed(cmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void vkCmdDispatch(VkCommandBuffer cmd, uint32_t x, uint32_t y, uint32_t z) {
    hg_internal_vulkan_funcs.vkCmdDispatch(cmd, x, y, z);
}

#define HG_LOAD_VULKAN_INSTANCE_FUNC(instance, name) \
    hg_internal_vulkan_funcs. name = (PFN_##name)hg_internal_vulkan_funcs.vkGetInstanceProcAddr(instance, #name); \
    if (hg_internal_vulkan_funcs. name == nullptr) { hg_error("Could not load " #name "\n"); }

void hg_vk_load_instance(VkInstance instance) {
    hg_assert(instance != nullptr);

    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetDeviceProcAddr);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkDestroyInstance);
    hg_debug_mode(HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkCreateDebugUtilsMessengerEXT));
    hg_debug_mode(HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkDestroyDebugUtilsMessengerEXT));
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkEnumeratePhysicalDevices);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkEnumerateDeviceExtensionProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceQueueFamilyProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceMemoryProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceMemoryProperties2);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfaceFormatsKHR);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfacePresentModesKHR);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfaceCapabilitiesKHR);

    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkDestroySurfaceKHR)
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkCreateDevice)
}

#undef HG_LOAD_VULKAN_INSTANCE_FUNC

#define HG_LOAD_VULKAN_DEVICE_FUNC(device, name) \
    hg_internal_vulkan_funcs. name = (PFN_##name)hg_internal_vulkan_funcs.vkGetDeviceProcAddr(device, #name); \
    if (hg_internal_vulkan_funcs. name == nullptr) { hg_error("Could not load " #name "\n"); }

void hg_vk_load_device(VkDevice device) {
    hg_assert(device != nullptr);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyDevice)
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDeviceWaitIdle)

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateSwapchainKHR)
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroySwapchainKHR)
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetSwapchainImagesKHR)
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkAcquireNextImageKHR)

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateSemaphore);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroySemaphore);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateFence);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyFence);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkResetFences);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkWaitForFences);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetDeviceQueue);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkQueueWaitIdle);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkQueueSubmit);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkQueuePresentKHR);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateCommandPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyCommandPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkAllocateCommandBuffers);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkFreeCommandBuffers);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateDescriptorPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyDescriptorPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkResetDescriptorPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkAllocateDescriptorSets);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkFreeDescriptorSets);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkUpdateDescriptorSets);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateDescriptorSetLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyDescriptorSetLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreatePipelineLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyPipelineLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateShaderModule);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyShaderModule);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateGraphicsPipelines);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateComputePipelines);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyPipeline);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateImageView);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyImageView);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateSampler);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroySampler);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetBufferMemoryRequirements);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetBufferMemoryRequirements2);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetImageMemoryRequirements);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetImageMemoryRequirements2);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetDeviceBufferMemoryRequirements);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetDeviceImageMemoryRequirements);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkAllocateMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkFreeMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBindBufferMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBindBufferMemory2);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBindImageMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBindImageMemory2);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkMapMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkUnmapMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkFlushMappedMemoryRanges);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkInvalidateMappedMemoryRanges);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBeginCommandBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkEndCommandBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkResetCommandBuffer);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdCopyBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdCopyImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBlitImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdCopyBufferToImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdCopyImageToBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdPipelineBarrier2);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBeginRendering);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdEndRendering);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdSetViewport);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdSetScissor);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBindPipeline);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBindDescriptorSets);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdPushConstants);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBindVertexBuffers);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBindIndexBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdDraw);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdDrawIndexed);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdDispatch);
}

#undef HG_LOAD_VULKAN_DEVICE_FUNC

const char *hg_vk_result_string(VkResult result) {
    switch (result) {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN:
            return "VK_ERROR_UNKNOWN";
        case VK_ERROR_VALIDATION_FAILED:
            return "VK_ERROR_VALIDATION_FAILED";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_ERROR_FRAGMENTATION:
            return "VK_ERROR_FRAGMENTATION";
        case VK_PIPELINE_COMPILE_REQUIRED:
            return "VK_PIPELINE_COMPILE_REQUIRED";
        case VK_ERROR_NOT_PERMITTED:
            return "VK_ERROR_NOT_PERMITTED";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_THREAD_IDLE_KHR:
            return "VK_THREAD_IDLE_KHR";
        case VK_THREAD_DONE_KHR:
            return "VK_THREAD_DONE_KHR";
        case VK_OPERATION_DEFERRED_KHR:
            return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return "VK_OPERATION_NOT_DEFERRED_KHR";
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
            return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
        case VK_INCOMPATIBLE_SHADER_BINARY_EXT:
            return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
        case VK_PIPELINE_BINARY_MISSING_KHR:
            return "VK_PIPELINE_BINARY_MISSING_KHR";
        case VK_ERROR_NOT_ENOUGH_SPACE_KHR:
            return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
        case VK_RESULT_MAX_ENUM:
            return "VK_RESULT_MAX_ENUM";
    }
    return "Unrecognized Vulkan result";
}

u32 hg_vk_format_to_size(VkFormat format) {
    switch (format) {
        case VK_FORMAT_UNDEFINED:
            return 0;

        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_USCALED:
        case VK_FORMAT_R8_SSCALED:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_A8_UNORM:
        case VK_FORMAT_R8_BOOL_ARM:
            return 1;

        case VK_FORMAT_R4G4_UNORM_PACK8: return 1;
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
        case VK_FORMAT_B5G6R5_UNORM_PACK16:
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
        case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
        case VK_FORMAT_A4B4G4R4_UNORM_PACK16:
        case VK_FORMAT_A1B5G5R5_UNORM_PACK16:
            return 2;

        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_USCALED:
        case VK_FORMAT_R16_SSCALED:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SFLOAT:
            return 2;

        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_USCALED:
        case VK_FORMAT_R8G8_SSCALED:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8_SRGB:
            return 2;

        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_USCALED:
        case VK_FORMAT_R8G8B8_SSCALED:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_B8G8R8_SNORM:
        case VK_FORMAT_B8G8R8_USCALED:
        case VK_FORMAT_B8G8R8_SSCALED:
        case VK_FORMAT_B8G8R8_UINT:
        case VK_FORMAT_B8G8R8_SINT:
        case VK_FORMAT_B8G8R8_SRGB:
            return 3;

        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_USCALED:
        case VK_FORMAT_R8G8B8A8_SSCALED:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_B8G8R8A8_USCALED:
        case VK_FORMAT_B8G8R8A8_SSCALED:
        case VK_FORMAT_B8G8R8A8_UINT:
        case VK_FORMAT_B8G8R8A8_SINT:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
            return 4;

        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:
            return 4;

        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_USCALED:
        case VK_FORMAT_R16G16_SSCALED:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SFLOAT:
            return 4;

        case VK_FORMAT_R16G16B16_UNORM:
        case VK_FORMAT_R16G16B16_SNORM:
        case VK_FORMAT_R16G16B16_USCALED:
        case VK_FORMAT_R16G16B16_SSCALED:
        case VK_FORMAT_R16G16B16_UINT:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16_SFLOAT:
            return 6;

        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_USCALED:
        case VK_FORMAT_R16G16B16A16_SSCALED:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return 8;

        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
            return 4;

        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
            return 8;

        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
            return 12;

        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 16;

        case VK_FORMAT_R64_UINT:
        case VK_FORMAT_R64_SINT:
        case VK_FORMAT_R64_SFLOAT:
            return 8;

        case VK_FORMAT_R64G64_UINT:
        case VK_FORMAT_R64G64_SINT:
        case VK_FORMAT_R64G64_SFLOAT:
            return 16;

        case VK_FORMAT_R64G64B64_UINT:
        case VK_FORMAT_R64G64B64_SINT:
        case VK_FORMAT_R64G64B64_SFLOAT:
            return 24;

        case VK_FORMAT_R64G64B64A64_UINT:
        case VK_FORMAT_R64G64B64A64_SINT:
        case VK_FORMAT_R64G64B64A64_SFLOAT:
            return 32;

        case VK_FORMAT_D16_UNORM: return 2;
        case VK_FORMAT_X8_D24_UNORM_PACK32: return 4;
        case VK_FORMAT_D32_SFLOAT: return 4;
        case VK_FORMAT_S8_UINT: return 1;
        case VK_FORMAT_D16_UNORM_S8_UINT: return 3;
        case VK_FORMAT_D24_UNORM_S8_UINT: return 4;
        case VK_FORMAT_D32_SFLOAT_S8_UINT: return 5;

        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
            return 8;

        case VK_FORMAT_BC2_UNORM_BLOCK:
        case VK_FORMAT_BC2_SRGB_BLOCK:
        case VK_FORMAT_BC3_UNORM_BLOCK:
        case VK_FORMAT_BC3_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_BC4_UNORM_BLOCK:
        case VK_FORMAT_BC4_SNORM_BLOCK:
        case VK_FORMAT_BC5_UNORM_BLOCK:
        case VK_FORMAT_BC5_SNORM_BLOCK:
            return 16;

        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        case VK_FORMAT_BC7_UNORM_BLOCK:
        case VK_FORMAT_BC7_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
            return 8;

        case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
            return 8;

        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_EAC_R11_UNORM_BLOCK:
        case VK_FORMAT_EAC_R11_SNORM_BLOCK:
            return 8;

        case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
        case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
            return 16;

        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
            return 8;
        case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
            return 8;
        case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
            return 8;
        case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
            return 8;

        case VK_FORMAT_G8B8G8R8_422_UNORM:
        case VK_FORMAT_B8G8R8G8_422_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
        case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
        case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
        case VK_FORMAT_G16B16G16R16_422_UNORM:
        case VK_FORMAT_B16G16R16G16_422_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
        case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
        case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
            return 0;

        default:
            hg_warn("Unrecognized Vulkan format value\n");
            return 0;
    }
}

static VkBool32 hg_internal_debug_callback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    const VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
) {
    (void)type;
    (void)user_data;

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        (void)std::fprintf(stderr, "Vulkan Error: %s\n", callback_data->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        (void)std::fprintf(stderr, "Vulkan Warning: %s\n", callback_data->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        (void)std::fprintf(stderr, "Vulkan Info: %s\n", callback_data->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        (void)std::fprintf(stderr, "Vulkan Verbose: %s\n", callback_data->pMessage);
    } else {
        (void)std::fprintf(stderr, "Vulkan Unknown: %s\n", callback_data->pMessage);
    }
    return VK_FALSE;
}

static const VkDebugUtilsMessengerCreateInfoEXT hg_internal_debug_utils_messenger_info{
    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    nullptr, 0,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    hg_internal_debug_callback, nullptr,
};

VkInstance hg_vk_create_instance(const char *app_name) {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = app_name != nullptr ? app_name : "Hurdy Gurdy Application",
    app_info.applicationVersion = 0;
    app_info.pEngineName = "Hurdy Gurdy Engine";
    app_info.engineVersion = 0;
    app_info.apiVersion = VK_API_VERSION_1_3;

#ifdef HG_DEBUG_MODE
    const char* layers[]{
        "VK_LAYER_KHRONOS_validation",
    };
#endif

    const char *exts[]{
#ifdef HG_DEBUG_MODE
        "VK_EXT_debug_utils",
#endif
        "VK_KHR_surface",
#if defined(HG_PLATFORM_LINUX)
        "VK_KHR_xlib_surface",
#elif defined(HG_PLATFORM_WINDOWS)
        "VK_KHR_win32_surface",
#endif
    };

    VkInstanceCreateInfo instance_info{};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifdef HG_DEBUG_MODE
    instance_info.pNext = &hg_internal_debug_utils_messenger_info;
#endif
    instance_info.flags = 0;
    instance_info.pApplicationInfo = &app_info;
#ifdef HG_DEBUG_MODE
    instance_info.enabledLayerCount = hg_countof(layers);
    instance_info.ppEnabledLayerNames = layers;
#endif
    instance_info.enabledExtensionCount = hg_countof(exts);
    instance_info.ppEnabledExtensionNames = exts;

    VkInstance instance = nullptr;
    VkResult result = vkCreateInstance(&instance_info, nullptr, &instance);
    if (instance == nullptr)
        hg_error("Failed to create Vulkan instance: %s\n", hg_vk_result_string(result));

    hg_vk_load_instance(instance);
    return instance;
}

VkDebugUtilsMessengerEXT hg_vk_create_debug_messenger(VkInstance instance) {
    hg_assert(instance != nullptr);

    VkDebugUtilsMessengerEXT messenger = nullptr;
    VkResult result = vkCreateDebugUtilsMessengerEXT(
        instance, &hg_internal_debug_utils_messenger_info, nullptr, &messenger);
    if (messenger == nullptr)
        hg_error("Failed to create Vulkan debug messenger: %s\n", hg_vk_result_string(result));

    return messenger;
}

std::optional<u32> hg_vk_find_queue_family(VkPhysicalDevice gpu, VkQueueFlags queue_flags) {
    hg_assert(gpu != nullptr);

    u32 family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, nullptr);
    VkQueueFamilyProperties *families = (VkQueueFamilyProperties *)alloca(family_count * sizeof(*families));
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, families);

    for (u32 i = 0; i < family_count; ++i) {
        if (families[i].queueFlags & queue_flags) {
            return i;
        }
    }
    return std::nullopt;
}

static const char *const hg_internal_vk_device_extensions[]{
    "VK_KHR_swapchain",
};

static VkPhysicalDevice hg_internal_find_single_queue_gpu(VkInstance instance, u32 *queue_family) {
    hg_assert(instance != nullptr);

    HgStdAllocator mem; // replace with temporary allocator : TODO

    u32 gpu_count;
    vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr);
    VkPhysicalDevice *gpus = (VkPhysicalDevice *)alloca(gpu_count * sizeof(*gpus));
    vkEnumeratePhysicalDevices(instance, &gpu_count, gpus);

    HgSpan<VkExtensionProperties> ext_props{};
    hg_defer(mem.free(ext_props));

    for (u32 i = 0; i < gpu_count; ++i) {
        VkPhysicalDevice gpu = gpus[i];
        std::optional<u32> family;

        u32 new_prop_count = 0;
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &new_prop_count, nullptr);
        if (new_prop_count > ext_props.count) {
            ext_props = mem.realloc(ext_props, new_prop_count);
        }
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &new_prop_count, ext_props.data);

        for (usize j = 0; j < hg_countof(hg_internal_vk_device_extensions); j++) {
            for (usize k = 0; k < new_prop_count; k++) {
                if (strcmp(hg_internal_vk_device_extensions[j], ext_props[k].extensionName) == 0)
                    goto next_ext;
            }
            goto next_gpu;
next_ext:
            continue;
        }

        family = hg_vk_find_queue_family(gpu, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
        if (!family.has_value())
            goto next_gpu;

        *queue_family = *family;
        return gpu;

next_gpu:
        continue;
    }

    hg_warn("Could not find a suitable gpu\n");
    return nullptr;
}

static VkDevice hg_internal_create_single_queue_device(VkPhysicalDevice gpu, u32 queue_family) {
    hg_assert(gpu != nullptr);

    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_feature{};
    dynamic_rendering_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamic_rendering_feature.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceSynchronization2Features synchronization2_feature{};
    synchronization2_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization2_feature.pNext = &dynamic_rendering_feature;
    synchronization2_feature.synchronization2 = VK_TRUE;

    VkPhysicalDeviceFeatures features{};

    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = queue_family;
    queue_info.queueCount = 1;
    float queue_priority = 1.0f;
    queue_info.pQueuePriorities = &queue_priority;

    VkDeviceCreateInfo device_info{};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = &synchronization2_feature;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = hg_countof(hg_internal_vk_device_extensions);
    device_info.ppEnabledExtensionNames = hg_internal_vk_device_extensions;
    device_info.pEnabledFeatures = &features;

    VkDevice device = nullptr;
    VkResult result = vkCreateDevice(gpu, &device_info, nullptr, &device);

    if (device == nullptr)
        hg_error("Could not create Vulkan device: %s\n", hg_vk_result_string(result));
    return device;
}

HgSingleQueueDeviceData hg_vk_create_single_queue_device(VkInstance instance) {
    hg_assert(instance != nullptr);

    HgSingleQueueDeviceData device{};
    device.gpu = hg_internal_find_single_queue_gpu(instance, &device.queue_family);
    device.handle = hg_internal_create_single_queue_device(device.gpu, device.queue_family);
    hg_vk_load_device(device.handle);
    vkGetDeviceQueue(device.handle, device.queue_family, 0, &device.queue);

    return device;
}

VkPipeline hg_vk_create_graphics_pipeline(VkDevice device, const HgVkPipelineConfig& config) {
    hg_assert(device != nullptr);
    if (config.color_attachment_formats.count > 0)
        hg_assert(config.color_attachment_formats.data != nullptr);
    hg_assert(config.shader_stages != nullptr);
    hg_assert(config.layout != nullptr);
    if (config.vertex_bindings.count > 0)
        hg_assert(config.vertex_bindings.data != nullptr);

    VkPipelineVertexInputStateCreateInfo vertex_input_state{};
    vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state.vertexBindingDescriptionCount = (u32)config.vertex_bindings.count;
    vertex_input_state.pVertexBindingDescriptions = config.vertex_bindings.data;
    vertex_input_state.vertexAttributeDescriptionCount = (u32)config.vertex_attributes.count;
    vertex_input_state.pVertexAttributeDescriptions = config.vertex_attributes.data;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state{};
    input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state.topology = config.topology;
    input_assembly_state.primitiveRestartEnable = VK_FALSE;

    VkPipelineTessellationStateCreateInfo tessellation_state{};
    tessellation_state.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tessellation_state.patchControlPoints = config.tesselation_patch_control_points;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization_state{};
    rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state.depthClampEnable = VK_FALSE;
    rasterization_state.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state.polygonMode = config.polygon_mode;
    rasterization_state.cullMode = config.cull_mode;
    rasterization_state.frontFace = config.front_face;
    rasterization_state.depthBiasEnable = VK_FALSE;
    rasterization_state.depthBiasConstantFactor = 0.0f;
    rasterization_state.depthBiasClamp = 0.0f;
    rasterization_state.depthBiasSlopeFactor = 0.0f;
    rasterization_state.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state{};
    multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.rasterizationSamples = config.multisample_count != 0
        ? config.multisample_count
        : VK_SAMPLE_COUNT_1_BIT,
    multisample_state.sampleShadingEnable = VK_FALSE;
    multisample_state.minSampleShading = 1.0f;
    multisample_state.pSampleMask = nullptr;
    multisample_state.alphaToCoverageEnable = VK_FALSE;
    multisample_state.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
    depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state.depthTestEnable = config.depth_attachment_format != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE;
    depth_stencil_state.depthWriteEnable = config.depth_attachment_format != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE;
    depth_stencil_state.depthCompareOp = config.enable_color_blend
            ? VK_COMPARE_OP_LESS_OR_EQUAL
            : VK_COMPARE_OP_LESS;
    depth_stencil_state.depthBoundsTestEnable = config.depth_attachment_format != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE;
    depth_stencil_state.stencilTestEnable = config.stencil_attachment_format != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE,
    // depth_stencil_state.front = {}; : TODO
    // depth_stencil_state.back = {}; : TODO
    depth_stencil_state.minDepthBounds = 0.0f;
    depth_stencil_state.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.blendEnable = config.enable_color_blend ? VK_TRUE : VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.colorWriteMask
        = VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend_state{};
    color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state.logicOpEnable = VK_FALSE;
    color_blend_state.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state.attachmentCount = 1;
    color_blend_state.pAttachments = &color_blend_attachment;
    color_blend_state.blendConstants[0] = {1.0f};
    color_blend_state.blendConstants[1] = {1.0f};
    color_blend_state.blendConstants[2] = {1.0f};
    color_blend_state.blendConstants[3] = {1.0f};

    VkDynamicState dynamic_states[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = hg_countof(dynamic_states);
    dynamic_state.pDynamicStates = dynamic_states;

    VkPipelineRenderingCreateInfo rendering_info{};
    rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_info.colorAttachmentCount = (u32)config.color_attachment_formats.count;
    rendering_info.pColorAttachmentFormats = config.color_attachment_formats.data;
    rendering_info.depthAttachmentFormat = config.depth_attachment_format;
    rendering_info.stencilAttachmentFormat = config.stencil_attachment_format;

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.pNext = &rendering_info;
    pipeline_info.stageCount = (u32)config.shader_stages.count;
    pipeline_info.pStages = config.shader_stages.data;
    pipeline_info.pVertexInputState = &vertex_input_state;
    pipeline_info.pInputAssemblyState = &input_assembly_state;
    pipeline_info.pTessellationState = &tessellation_state;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterization_state;
    pipeline_info.pMultisampleState = &multisample_state;
    pipeline_info.pDepthStencilState = &depth_stencil_state;
    pipeline_info.pColorBlendState = &color_blend_state;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = config.layout;
    pipeline_info.basePipelineHandle = nullptr;
    pipeline_info.basePipelineIndex = -1;

    VkPipeline pipeline = nullptr;
    VkResult result = vkCreateGraphicsPipelines(device, nullptr, 1, &pipeline_info, nullptr, &pipeline);
    if (pipeline == nullptr)
        hg_error("Failed to create Vulkan graphics pipeline: %s\n", hg_vk_result_string(result));

    return pipeline;
}

VkPipeline hg_vk_create_compute_pipeline(VkDevice device, const HgVkPipelineConfig& config) {
    hg_assert(device != nullptr);
    hg_assert(config.color_attachment_formats == nullptr);
    hg_assert(config.depth_attachment_format == VK_FORMAT_UNDEFINED);
    hg_assert(config.stencil_attachment_format == VK_FORMAT_UNDEFINED);
    hg_assert(config.shader_stages != nullptr);
    hg_assert(config.shader_stages.count == 1);
    hg_assert(config.shader_stages[0].stage == VK_SHADER_STAGE_COMPUTE_BIT);
    hg_assert(config.layout != nullptr);
    hg_assert(config.vertex_bindings == nullptr);

    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage = config.shader_stages[0];
    pipeline_info.layout = config.layout;
    pipeline_info.basePipelineHandle = nullptr;
    pipeline_info.basePipelineIndex = -1;

    VkPipeline pipeline = nullptr;
    VkResult result = vkCreateComputePipelines(device, nullptr, 1, &pipeline_info, nullptr, &pipeline);
    if (pipeline == nullptr)
        hg_error("Failed to create Vulkan compute pipeline: %s\n", hg_vk_result_string(result));

    return pipeline;
}

static VkFormat hg_internal_vk_find_swapchain_format(VkPhysicalDevice gpu, VkSurfaceKHR surface) {
    hg_assert(gpu != nullptr);
    hg_assert(surface != nullptr);

    u32 format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, nullptr);
    VkSurfaceFormatKHR *formats = (VkSurfaceFormatKHR *)alloca(format_count * sizeof(*formats));
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, formats);

    for (usize i = 0; i < format_count; ++i) {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
            return VK_FORMAT_R8G8B8A8_SRGB;
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
            return VK_FORMAT_B8G8R8A8_SRGB;
    }
    hg_error("No supported swapchain formats\n");
}

static VkPresentModeKHR hg_internal_vk_find_swapchain_present_mode(
    VkPhysicalDevice gpu,
    VkSurfaceKHR surface,
    VkPresentModeKHR desired_mode
) {
    hg_assert(gpu != nullptr);
    hg_assert(surface != nullptr);

    if (desired_mode == VK_PRESENT_MODE_FIFO_KHR)
        return desired_mode;

    u32 mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &mode_count, nullptr);
    VkPresentModeKHR *present_modes = (VkPresentModeKHR *)alloca(mode_count * sizeof(*present_modes));
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &mode_count, present_modes);

    for (usize i = 0; i < mode_count; ++i) {
        if (present_modes[i] == desired_mode)
            return desired_mode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

HgSwapchainData hg_vk_create_swapchain(
    VkDevice device,
    VkPhysicalDevice gpu,
    VkSwapchainKHR old_swapchain,
    VkSurfaceKHR surface,
    VkImageUsageFlags image_usage,
    VkPresentModeKHR desired_mode
) {
    hg_assert(device != nullptr);
    hg_assert(gpu != nullptr);
    hg_assert(surface != nullptr);
    hg_assert(image_usage != 0);

    HgSwapchainData swapchain{};

    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &surface_capabilities);

    if (surface_capabilities.currentExtent.width == 0 ||
        surface_capabilities.currentExtent.height == 0 ||
        surface_capabilities.currentExtent.width < surface_capabilities.minImageExtent.width ||
        surface_capabilities.currentExtent.height < surface_capabilities.minImageExtent.height ||
        surface_capabilities.currentExtent.width > surface_capabilities.maxImageExtent.width ||
        surface_capabilities.currentExtent.height > surface_capabilities.maxImageExtent.height
    ) {
        hg_warn("Could not create swapchain of the surface's size\n");
        return swapchain;
    }

    swapchain.width = surface_capabilities.currentExtent.width;
    swapchain.height = surface_capabilities.currentExtent.height;
    swapchain.format = hg_internal_vk_find_swapchain_format(gpu, surface);

    VkSwapchainCreateInfoKHR swapchain_info{};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = surface;
    swapchain_info.minImageCount = surface_capabilities.minImageCount;
    swapchain_info.imageFormat = swapchain.format;
    swapchain_info.imageExtent = surface_capabilities.currentExtent;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = image_usage;
    swapchain_info.preTransform = surface_capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = hg_internal_vk_find_swapchain_present_mode(gpu, surface, desired_mode);
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = old_swapchain;

    VkResult result = vkCreateSwapchainKHR(device, &swapchain_info, nullptr, &swapchain.handle);
    if (swapchain.handle == nullptr)
        hg_error("Failed to create swapchain: %s\n", hg_vk_result_string(result));

    return swapchain;
}

HgSwapchainCommands HgSwapchainCommands::create(VkDevice device, VkSwapchainKHR swapchain, VkCommandPool cmd_pool) {
    hg_assert(device != nullptr);
    hg_assert(cmd_pool != nullptr);
    hg_assert(swapchain != nullptr);

    HgStdAllocator mem;

    HgSwapchainCommands sync;
    sync.cmd_pool = cmd_pool;
    sync.swapchain = swapchain;

    vkGetSwapchainImagesKHR(device, swapchain, &sync.frame_count, nullptr);

    void *allocation = mem.alloc_fn(
        sync.frame_count * sizeof(*sync.cmds) +
        sync.frame_count * sizeof(*sync.frame_finished) +
        sync.frame_count * sizeof(*sync.image_available) +
        sync.frame_count * sizeof(*sync.ready_to_present),
        16);

    sync.cmds = (VkCommandBuffer *)allocation;
    VkCommandBufferAllocateInfo cmd_alloc_info{};
    cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc_info.pNext = nullptr;
    cmd_alloc_info.commandPool = cmd_pool;
    cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_alloc_info.commandBufferCount = sync.frame_count;

    vkAllocateCommandBuffers(device, &cmd_alloc_info, sync.cmds);

    sync.frame_finished = (VkFence *)(sync.cmds + sync.frame_count);
    for (usize i = 0; i < sync.frame_count; ++i) {
        VkFenceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(device, &info, nullptr, &sync.frame_finished[i]);
    }

    sync.image_available = (VkSemaphore *)(sync.frame_finished + sync.frame_count);
    for (usize i = 0; i < sync.frame_count; ++i) {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(device, &info, nullptr, &sync.image_available[i]);
    }

    sync.ready_to_present = (VkSemaphore *)(sync.image_available + sync.frame_count);
    for (usize i = 0; i < sync.frame_count; ++i) {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(device, &info, nullptr, &sync.ready_to_present[i]);
    }

    return sync;
}

void HgSwapchainCommands::destroy(VkDevice device) {
    hg_assert(device != nullptr);

    HgStdAllocator mem;

    vkFreeCommandBuffers(device, cmd_pool, frame_count, cmds);
    for (usize i = 0; i < frame_count; ++i) {
        vkDestroyFence(device, frame_finished[i], nullptr);
    }
    for (usize i = 0; i < frame_count; ++i) {
        vkDestroySemaphore(device, image_available[i], nullptr);
    }
    for (usize i = 0; i < frame_count; ++i) {
        vkDestroySemaphore(device, ready_to_present[i], nullptr);
    }

    mem.free_fn(
        cmds,
        frame_count * sizeof(*cmds) +
        frame_count * sizeof(*frame_finished) +
        frame_count * sizeof(*image_available) +
        frame_count * sizeof(*ready_to_present),
        16);

    std::memset(this, 0, sizeof(*this));
}

VkCommandBuffer HgSwapchainCommands::acquire_and_record(VkDevice device) {
    hg_assert(device != nullptr);

    current_frame = (current_frame + 1) % frame_count;

    vkWaitForFences(device, 1, &frame_finished[current_frame], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &frame_finished[current_frame]);

    VkResult result = vkAcquireNextImageKHR(
        device, swapchain, UINT64_MAX, image_available[current_frame], nullptr, &current_image);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
        return nullptr;


    VkCommandBuffer cmd = cmds[current_frame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);
    return cmd;
}

void HgSwapchainCommands::end_and_present(VkQueue queue) {
    hg_assert(queue != nullptr);

    VkCommandBuffer cmd = cmds[current_frame];
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &image_available[current_frame];
    VkPipelineStageFlags stage_flags{VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT};
    submit.pWaitDstStageMask = &stage_flags;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &ready_to_present[current_image];

    vkQueueSubmit(queue, 1, &submit, frame_finished[current_frame]);

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &ready_to_present[current_image];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &current_image;

    vkQueuePresentKHR(queue, &present_info);
}

u32 hg_vk_find_memory_type_index(
    VkPhysicalDevice gpu,
    u32 bitmask,
    VkMemoryPropertyFlags desired_flags,
    VkMemoryPropertyFlags undesired_flags
) {
    hg_assert(gpu != nullptr);
    hg_assert(bitmask != 0);

    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(gpu, &mem_props);

    for (u32 i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((bitmask & (1 << i)) == 0)
            continue;
        if ((mem_props.memoryTypes[i].propertyFlags & undesired_flags) != 0)
            continue;
        if ((mem_props.memoryTypes[i].propertyFlags & desired_flags) == 0)
            continue;
        return i;
    }
    for (u32 i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((bitmask & (1 << i)) == 0)
            continue;
        if ((mem_props.memoryTypes[i].propertyFlags & desired_flags) == 0)
            continue;
        hg_warn("Could not find Vulkan memory type without undesired flags\n");
        return i;
    }
    for (u32 i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((bitmask & (1 << i)) == 0)
            continue;
        hg_warn("Could not find Vulkan memory type with desired flags\n");
        return i;
    }
    hg_error("Could not find Vulkan memory type\n");
}

void hg_vk_buffer_staging_write(
    VkDevice device,
    VmaAllocator allocator,
    VkQueue transfer_queue,
    VkCommandPool cmd_pool,
    VkBuffer dst,
    usize offset,
    HgSpan<const void> src
) {
    hg_assert(device != nullptr);
    hg_assert(allocator != nullptr);
    hg_assert(cmd_pool != nullptr);
    hg_assert(transfer_queue != nullptr);
    hg_assert(dst != nullptr);
    hg_assert(src != nullptr);

    VkBufferCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stage_info.size = src.count;
    stage_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stage_alloc_info{};
    stage_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    stage_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    VkBuffer stage = nullptr;
    VmaAllocation stage_alloc = nullptr;
    vmaCreateBuffer(allocator, &stage_info, &stage_alloc_info, &stage, &stage_alloc, nullptr);
    vmaCopyMemoryToAllocation(allocator, src.data, stage_alloc, offset, src.count);
    hg_defer(vmaDestroyBuffer(allocator, stage, stage_alloc));

    VkCommandBufferAllocateInfo cmd_info{};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_info.commandPool = cmd_pool;
    cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_info.commandBufferCount = 1;

    VkCommandBuffer cmd = nullptr;
    vkAllocateCommandBuffers(device, &cmd_info, &cmd);
    hg_defer(vkFreeCommandBuffers(device, cmd_pool, 1, &cmd));

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);
    VkBufferCopy region{};
    region.dstOffset = offset;
    region.size = src.count;

    vkCmdCopyBuffer(cmd, stage, dst, 1, &region);
    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = nullptr;
    vkCreateFence(device, &fence_info, nullptr, &fence);
    hg_defer(vkDestroyFence(device, fence, nullptr));

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    vkQueueSubmit(transfer_queue, 1, &submit, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
}

void hg_vk_buffer_staging_read(
    VkDevice device,
    VmaAllocator allocator,
    VkQueue transfer_queue,
    VkCommandPool cmd_pool,
    HgSpan<void> dst,
    VkBuffer src,
    usize offset
) {
    hg_assert(device != nullptr);
    hg_assert(allocator != nullptr);
    hg_assert(cmd_pool != nullptr);
    hg_assert(transfer_queue != nullptr);
    hg_assert(dst != nullptr);
    hg_assert(src != nullptr);

    VkBufferCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stage_info.size = dst.count;
    stage_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo stage_alloc_info{};
    stage_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    stage_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    VkBuffer stage = nullptr;
    VmaAllocation stage_alloc = nullptr;
    vmaCreateBuffer(allocator, &stage_info, &stage_alloc_info, &stage, &stage_alloc, nullptr);
    hg_defer(vmaDestroyBuffer(allocator, stage, stage_alloc));

    VkCommandBufferAllocateInfo cmd_info{};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_info.commandPool = cmd_pool;
    cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_info.commandBufferCount = 1;

    VkCommandBuffer cmd = nullptr;
    vkAllocateCommandBuffers(device, &cmd_info, &cmd);
    hg_defer(vkFreeCommandBuffers(device, cmd_pool, 1, &cmd));

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);
    VkBufferCopy region{};
    region.srcOffset = offset;
    region.size = dst.count;

    vkCmdCopyBuffer(cmd, src, stage, 1, &region);
    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = nullptr;
    vkCreateFence(device, &fence_info, nullptr, &fence);
    hg_defer(vkDestroyFence(device, fence, nullptr));

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    vkQueueSubmit(transfer_queue, 1, &submit, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    vmaCopyAllocationToMemory(allocator, stage_alloc, offset, dst.data, dst.count);
}

void hg_vk_image_staging_write(
    VkDevice device,
    VmaAllocator allocator,
    VkQueue transfer_queue,
    VkCommandPool cmd_pool,
    const HgVkImageStagingWriteConfig& config
) {
    hg_assert(device != nullptr);
    hg_assert(allocator != nullptr);
    hg_assert(cmd_pool != nullptr);
    hg_assert(transfer_queue != nullptr);
    hg_assert(config.dst_image != nullptr);
    hg_assert(config.src_data != nullptr);
    hg_assert(config.width > 0);
    hg_assert(config.height > 0);
    hg_assert(config.depth > 0);
    hg_assert(config.format != VK_FORMAT_UNDEFINED);

    usize size = config.width
               * config.height
               * config.depth
               * hg_vk_format_to_size(config.format);

    VkBufferCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stage_info.size = size;
    stage_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stage_alloc_info{};
    stage_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    stage_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    VkBuffer stage = nullptr;
    VmaAllocation stage_alloc = nullptr;
    vmaCreateBuffer(allocator, &stage_info, &stage_alloc_info, &stage, &stage_alloc, nullptr);
    vmaCopyMemoryToAllocation(allocator, config.src_data, stage_alloc, 0, size);
    hg_defer(vmaDestroyBuffer(allocator, stage, stage_alloc));

    VkCommandBufferAllocateInfo cmd_info{};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_info.commandPool = cmd_pool;
    cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_info.commandBufferCount = 1;

    VkCommandBuffer cmd = nullptr;
    vkAllocateCommandBuffers(device, &cmd_info, &cmd);
    hg_defer(vkFreeCommandBuffers(device, cmd_pool, 1, &cmd));

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);

    VkImageMemoryBarrier2 transfer_barrier{};
    transfer_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transfer_barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transfer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transfer_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    transfer_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transfer_barrier.image = config.dst_image;
    transfer_barrier.subresourceRange.aspectMask = config.subresource.aspectMask;
    transfer_barrier.subresourceRange.baseMipLevel = config.subresource.mipLevel;
    transfer_barrier.subresourceRange.levelCount = 1;
    transfer_barrier.subresourceRange.baseArrayLayer = config.subresource.baseArrayLayer;
    transfer_barrier.subresourceRange.layerCount = config.subresource.layerCount;

    VkDependencyInfo transfer_dep{};
    transfer_dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transfer_dep.imageMemoryBarrierCount = 1;
    transfer_dep.pImageMemoryBarriers = &transfer_barrier;

    vkCmdPipelineBarrier2(cmd, &transfer_dep);

    VkBufferImageCopy region{};
    region.imageSubresource = config.subresource;
    region.imageExtent = {config.width, config.height, config.depth};

    vkCmdCopyBufferToImage(cmd, stage, config.dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    if (config.layout != VK_IMAGE_LAYOUT_UNDEFINED) {
        VkImageMemoryBarrier2 end_barrier{};
        end_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        end_barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        end_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        end_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        end_barrier.newLayout = config.layout;
        end_barrier.image = config.dst_image;
        end_barrier.subresourceRange.aspectMask = config.subresource.aspectMask;
        end_barrier.subresourceRange.baseMipLevel = config.subresource.mipLevel;
        end_barrier.subresourceRange.levelCount = 1;
        end_barrier.subresourceRange.baseArrayLayer = config.subresource.baseArrayLayer;
        end_barrier.subresourceRange.layerCount = config.subresource.layerCount;

        VkDependencyInfo end_dep{};
        end_dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        end_dep.imageMemoryBarrierCount = 1;
        end_dep.pImageMemoryBarriers = &end_barrier;

        vkCmdPipelineBarrier2(cmd, &end_dep);
    }

    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence = nullptr;
    vkCreateFence(device, &fence_info, nullptr, &fence);
    hg_defer(vkDestroyFence(device, fence, nullptr));

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    vkQueueSubmit(transfer_queue, 1, &submit, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
}

void hg_vk_image_staging_read(
    VkDevice device,
    VmaAllocator allocator,
    VkQueue transfer_queue,
    VkCommandPool cmd_pool,
    const HgVkImageStagingReadConfig& config
) {
    hg_assert(device != nullptr);
    hg_assert(allocator != nullptr);
    hg_assert(cmd_pool != nullptr);
    hg_assert(transfer_queue != nullptr);
    hg_assert(config.src_image != nullptr);
    hg_assert(config.layout != VK_IMAGE_LAYOUT_UNDEFINED);
    hg_assert(config.dst != nullptr);
    hg_assert(config.width > 0);
    hg_assert(config.height > 0);
    hg_assert(config.depth > 0);
    hg_assert(config.format != VK_FORMAT_UNDEFINED);

    usize size = config.width
               * config.height
               * config.depth
               * hg_vk_format_to_size(config.format);

    VkBufferCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stage_info.size = size;
    stage_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo stage_alloc_info{};
    stage_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    stage_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    VkBuffer stage = nullptr;
    VmaAllocation stage_alloc = nullptr;
    vmaCreateBuffer(allocator, &stage_info, &stage_alloc_info, &stage, &stage_alloc, nullptr);
    hg_defer(vmaDestroyBuffer(allocator, stage, stage_alloc));

    VkCommandBufferAllocateInfo cmd_info{};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_info.commandPool = cmd_pool;
    cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_info.commandBufferCount = 1;

    VkCommandBuffer cmd = nullptr;
    vkAllocateCommandBuffers(device, &cmd_info, &cmd);
    hg_defer(vkFreeCommandBuffers(device, cmd_pool, 1, &cmd));

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);

    VkImageMemoryBarrier2 transfer_barrier{};
    transfer_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transfer_barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transfer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transfer_barrier.oldLayout = config.layout;
    transfer_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transfer_barrier.image = config.src_image;
    transfer_barrier.subresourceRange.aspectMask = config.subresource.aspectMask;
    transfer_barrier.subresourceRange.baseMipLevel = config.subresource.mipLevel;
    transfer_barrier.subresourceRange.levelCount = 1;
    transfer_barrier.subresourceRange.baseArrayLayer = config.subresource.baseArrayLayer;
    transfer_barrier.subresourceRange.layerCount = config.subresource.layerCount;

    VkDependencyInfo transfer_dep{};
    transfer_dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transfer_dep.imageMemoryBarrierCount = 1;
    transfer_dep.pImageMemoryBarriers = &transfer_barrier;

    vkCmdPipelineBarrier2(cmd, &transfer_dep);

    VkBufferImageCopy region{};
    region.imageSubresource = config.subresource;
    region.imageExtent = {config.width, config.height, config.depth};

    vkCmdCopyImageToBuffer(cmd, config.src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stage, 1, &region);

    VkImageMemoryBarrier2 end_barrier{};
    end_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    end_barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    end_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    end_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    end_barrier.newLayout = config.layout;
    end_barrier.image = config.src_image;
    end_barrier.subresourceRange.aspectMask = config.subresource.aspectMask;
    end_barrier.subresourceRange.baseMipLevel = config.subresource.mipLevel;
    end_barrier.subresourceRange.levelCount = 1;
    end_barrier.subresourceRange.baseArrayLayer = config.subresource.baseArrayLayer;
    end_barrier.subresourceRange.layerCount = config.subresource.layerCount;

    VkDependencyInfo end_dep{};
    end_dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    end_dep.imageMemoryBarrierCount = 1;
    end_dep.pImageMemoryBarriers = &end_barrier;

    vkCmdPipelineBarrier2(cmd, &end_dep);

    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = nullptr;
    vkCreateFence(device, &fence_info, nullptr, &fence);
    hg_defer(vkDestroyFence(device, fence, nullptr));

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    vkQueueSubmit(transfer_queue, 1, &submit, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    vmaCopyAllocationToMemory(allocator, stage_alloc, 0, config.dst, size);
}

void hg_vk_image_generate_mipmaps(
    VkDevice device,
    VkQueue transfer_queue,
    VkCommandPool cmd_pool,
    VkImage image,
    VkImageAspectFlags aspect_mask,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    u32 width,
    u32 height,
    u32 depth,
    u32 mip_count
) {
    hg_assert(device != nullptr);
    hg_assert(transfer_queue != nullptr);
    hg_assert(cmd_pool != nullptr);
    hg_assert(image != nullptr);
    hg_assert(old_layout != VK_IMAGE_LAYOUT_UNDEFINED);
    hg_assert(new_layout != VK_IMAGE_LAYOUT_UNDEFINED);
    hg_assert(width > 0);
    hg_assert(height > 0);
    hg_assert(depth > 0);
    hg_assert(mip_count > 0);
    if (mip_count == 1)
        return;

    VkCommandBufferAllocateInfo cmd_info{};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_info.commandPool = cmd_pool;
    cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_info.commandBufferCount = 1;

    VkCommandBuffer cmd = nullptr;
    vkAllocateCommandBuffers(device, &cmd_info, &cmd);
    hg_defer(vkFreeCommandBuffers(device, cmd_pool, 1, &cmd));

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);

    VkOffset3D mip_offset{};
    mip_offset.x = (i32)width;
    mip_offset.y = (i32)height;
    mip_offset.z = (i32)depth;

    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    barrier.srcStageMask = VK_PIPELINE_STAGE_NONE;
    barrier.srcAccessMask = VK_ACCESS_NONE;
    barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.oldLayout = old_layout;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspect_mask;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(cmd, &dep);

    for (u32 level = 0; level < mip_count - 1; ++level) {
        barrier.srcStageMask = VK_PIPELINE_STAGE_NONE;
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.subresourceRange.aspectMask = aspect_mask;
        barrier.subresourceRange.baseMipLevel = level + 1;

        vkCmdPipelineBarrier2(cmd, &dep);

        VkImageBlit blit{};
        blit.srcSubresource.aspectMask = aspect_mask;
        blit.srcSubresource.mipLevel = level;
        blit.srcSubresource.layerCount = 1;
        blit.srcOffsets[1] = mip_offset;
        if (mip_offset.x > 1)
            mip_offset.x /= 2;
        if (mip_offset.y > 1)
            mip_offset.y /= 2;
        if (mip_offset.z > 1)
            mip_offset.z /= 2;
        blit.dstSubresource.aspectMask = aspect_mask;
        blit.dstSubresource.mipLevel = level + 1;
        blit.dstSubresource.layerCount = 1;
        blit.dstOffsets[1] = mip_offset;

        vkCmdBlitImage(cmd,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.subresourceRange.aspectMask = aspect_mask;
        barrier.subresourceRange.baseMipLevel = level + 1;

        vkCmdPipelineBarrier2(cmd, &dep);
    }

    barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_NONE;
    barrier.dstAccessMask = VK_ACCESS_NONE;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = new_layout;
    barrier.subresourceRange.aspectMask = aspect_mask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mip_count;

    vkCmdPipelineBarrier2(cmd, &dep);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;

    vkQueueSubmit(transfer_queue, 1, &submit_info, nullptr);
}

#include "sprite.frag.spv.h"
#include "sprite.vert.spv.h"

struct HgPipelineSpriteVPUniform {
    HgMat4f proj;
    HgMat4f view;
};

HgPipelineSprite hg_pipeline_sprite_create(
    VkDevice device,
    VmaAllocator allocator,
    VkFormat color_format,
    VkFormat depth_format
) {
    hg_assert(device != nullptr);
    hg_assert(color_format != VK_FORMAT_UNDEFINED);

    HgPipelineSprite pipeline;
    pipeline.device = device;
    pipeline.allocator = allocator;

    VkDescriptorSetLayoutBinding vp_bindings[1]{};
    vp_bindings[0].binding = 0;
    vp_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vp_bindings[0].descriptorCount = 1;
    vp_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo vp_layout_info{};
    vp_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    vp_layout_info.bindingCount = hg_countof(vp_bindings);
    vp_layout_info.pBindings = vp_bindings;

    vkCreateDescriptorSetLayout(device, &vp_layout_info, nullptr, &pipeline.vp_layout);

    VkDescriptorSetLayoutBinding image_bindings[1]{};
    image_bindings[0].binding = 0;
    image_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    image_bindings[0].descriptorCount = 1;
    image_bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo image_layout_info{};
    image_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    image_layout_info.bindingCount = hg_countof(image_bindings);
    image_layout_info.pBindings = image_bindings;

    vkCreateDescriptorSetLayout(device, &image_layout_info, nullptr, &pipeline.image_layout);

    VkDescriptorSetLayout set_layouts[]{pipeline.vp_layout, pipeline.image_layout};
    VkPushConstantRange push_ranges[1]{};
    push_ranges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_ranges[0].size = sizeof(HgPipelineSpritePush);

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = hg_countof(set_layouts);
    layout_info.pSetLayouts = set_layouts;
    layout_info.pushConstantRangeCount = hg_countof(push_ranges);
    layout_info.pPushConstantRanges = push_ranges;

    vkCreatePipelineLayout(device, &layout_info, nullptr, &pipeline.pipeline_layout);

    VkShaderModuleCreateInfo vertex_shader_info{};
    vertex_shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_info.codeSize = sprite_vert_spv_size;
    vertex_shader_info.pCode = (u32 *)sprite_vert_spv;

    VkShaderModule vertex_shader;
    vkCreateShaderModule(device, &vertex_shader_info, nullptr, &vertex_shader);

    VkShaderModuleCreateInfo fragment_shader_info{};
    fragment_shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragment_shader_info.codeSize = sprite_frag_spv_size;
    fragment_shader_info.pCode = (u32 *)sprite_frag_spv;

    VkShaderModule fragment_shader;
    vkCreateShaderModule(device, &fragment_shader_info, nullptr, &fragment_shader);

    VkPipelineShaderStageCreateInfo shader_stages[2]{};
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vertex_shader;
    shader_stages[0].pName = "main";
    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = fragment_shader;
    shader_stages[1].pName = "main";

    HgVkPipelineConfig pipeline_config{};
    pipeline_config.color_attachment_formats = {&color_format, 1};
    pipeline_config.depth_attachment_format = depth_format;
    pipeline_config.stencil_attachment_format = VK_FORMAT_UNDEFINED;
    pipeline_config.shader_stages = {shader_stages, hg_countof(shader_stages)};
    pipeline_config.layout = pipeline.pipeline_layout;
    pipeline_config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    pipeline_config.enable_color_blend = true;

    pipeline.pipeline = hg_vk_create_graphics_pipeline(device, pipeline_config);

    vkDestroyShaderModule(device, fragment_shader, nullptr);
    vkDestroyShaderModule(device, vertex_shader, nullptr);

    VkDescriptorPoolSize desc_pool_sizes[2]{};
    desc_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    desc_pool_sizes[0].descriptorCount = 1;
    desc_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    desc_pool_sizes[1].descriptorCount = 255;

    VkDescriptorPoolCreateInfo desc_pool_info{};
    desc_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    desc_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    desc_pool_info.maxSets = 256;
    desc_pool_info.poolSizeCount = hg_countof(desc_pool_sizes);
    desc_pool_info.pPoolSizes = desc_pool_sizes;

    vkCreateDescriptorPool(device, &desc_pool_info, nullptr, &pipeline.descriptor_pool);

    VkDescriptorSetAllocateInfo vp_set_alloc_info{};
    vp_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    vp_set_alloc_info.descriptorPool = pipeline.descriptor_pool;
    vp_set_alloc_info.descriptorSetCount = 1;
    vp_set_alloc_info.pSetLayouts = &pipeline.vp_layout;

    vkAllocateDescriptorSets(device, &vp_set_alloc_info, &pipeline.vp_set);

    VkBufferCreateInfo vp_buffer_info{};
    vp_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vp_buffer_info.size = sizeof(HgPipelineSpriteVPUniform);
    vp_buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VmaAllocationCreateInfo vp_alloc_info{};
    vp_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    vp_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateBuffer(
        allocator,
        &vp_buffer_info,
        &vp_alloc_info,
        &pipeline.vp_buffer,
        &pipeline.vp_buffer_allocation,
        nullptr);

    HgPipelineSpriteVPUniform vp_data{};
    vp_data.proj = 1.0f;
    vp_data.view = 1.0f;

    vmaCopyMemoryToAllocation(allocator, &vp_data, pipeline.vp_buffer_allocation, 0, sizeof(vp_data));

    VkDescriptorBufferInfo desc_info{};
    desc_info.buffer = pipeline.vp_buffer;
    desc_info.offset = 0;
    desc_info.range = sizeof(HgPipelineSpriteVPUniform);

    VkWriteDescriptorSet desc_write{};
    desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    desc_write.dstSet = pipeline.vp_set;
    desc_write.dstBinding = 0;
    desc_write.descriptorCount = 1;
    desc_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    desc_write.pBufferInfo = &desc_info;

    vkUpdateDescriptorSets(device, 1, &desc_write, 0, nullptr);

    return pipeline;
}

void hg_pipeline_sprite_destroy(HgPipelineSprite *pipeline) {
    if (pipeline == nullptr)
        return;

    vmaDestroyBuffer(pipeline->allocator, pipeline->vp_buffer, pipeline->vp_buffer_allocation);
    vkFreeDescriptorSets(pipeline->device, pipeline->descriptor_pool, 1, &pipeline->vp_set);
    vkDestroyDescriptorPool(pipeline->device, pipeline->descriptor_pool, nullptr);
    vkDestroyPipeline(pipeline->device, pipeline->pipeline, nullptr);
    vkDestroyPipelineLayout(pipeline->device, pipeline->pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(pipeline->device, pipeline->image_layout, nullptr);
    vkDestroyDescriptorSetLayout(pipeline->device, pipeline->vp_layout, nullptr);
}

void hg_pipeline_sprite_update_projection(HgPipelineSprite *pipeline, HgMat4f *projection) {
    hg_assert(pipeline != nullptr);
    hg_assert(projection != nullptr);

    vmaCopyMemoryToAllocation(
        pipeline->allocator,
        projection,
        pipeline->vp_buffer_allocation,
        offsetof(HgPipelineSpriteVPUniform, proj),
        sizeof(*projection));
}

void hg_pipeline_sprite_update_view(HgPipelineSprite *pipeline, HgMat4f *view) {
    hg_assert(pipeline != nullptr);
    hg_assert(view != nullptr);

    vmaCopyMemoryToAllocation(
        pipeline->allocator,
        view,
        pipeline->vp_buffer_allocation,
        offsetof(HgPipelineSpriteVPUniform, view),
        sizeof(*view));
}

HgPipelineSpriteTexture hg_pipeline_sprite_create_texture(
    HgPipelineSprite *pipeline,
    VkCommandPool cmd_pool,
    VkQueue transfer_queue,
    HgPipelineSpriteTextureConfig *config
) {
    hg_assert(pipeline != nullptr);
    hg_assert(config != nullptr);
    hg_assert(config->tex_data != nullptr);
    hg_assert(config->width > 0);
    hg_assert(config->height > 0);
    hg_assert(config->format != VK_FORMAT_UNDEFINED);

    HgPipelineSpriteTexture tex;

    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = config->format;
    image_info.extent = {config->width, config->height, 1};
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateImage(pipeline->allocator, &image_info, &alloc_info, &tex.image, &tex.allocation, nullptr);

    HgVkImageStagingWriteConfig staging_config{};
    staging_config.dst_image = tex.image;
    staging_config.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    staging_config.subresource.layerCount = 1;
    staging_config.src_data = config->tex_data;
    staging_config.width = config->width;
    staging_config.height = config->height;
    staging_config.depth = 1;
    staging_config.format = config->format;
    staging_config.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hg_vk_image_staging_write(pipeline->device, pipeline->allocator, transfer_queue, cmd_pool, staging_config);

    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = tex.image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = config->format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.layerCount = 1;

    vkCreateImageView(pipeline->device, &view_info, nullptr, &tex.view);

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = config->filter;
    sampler_info.minFilter = config->filter;
    sampler_info.addressModeU = config->edge_mode;
    sampler_info.addressModeV = config->edge_mode;
    sampler_info.addressModeW = config->edge_mode;

    vkCreateSampler(pipeline->device, &sampler_info, nullptr, &tex.sampler);

    VkDescriptorSetAllocateInfo set_info{};
    set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_info.descriptorPool = pipeline->descriptor_pool;
    set_info.descriptorSetCount = 1;
    set_info.pSetLayouts = &pipeline->image_layout;

    vkAllocateDescriptorSets(pipeline->device, &set_info, &tex.set);

    VkDescriptorImageInfo desc_info{};
    desc_info.sampler = tex.sampler;
    desc_info.imageView = tex.view;
    desc_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet desc_write{};
    desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    desc_write.dstSet = tex.set;
    desc_write.dstBinding = 0;
    desc_write.descriptorCount = 1;
    desc_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    desc_write.pImageInfo = &desc_info;

    vkUpdateDescriptorSets(pipeline->device, 1, &desc_write, 0, nullptr);

    return tex;
}

void hg_pipeline_sprite_destroy_texture(HgPipelineSprite *pipeline, HgPipelineSpriteTexture *texture) {
    hg_assert(pipeline != nullptr);
    hg_assert(texture != nullptr);

    vkFreeDescriptorSets(pipeline->device, pipeline->descriptor_pool, 1, &texture->set);
    vkDestroySampler(pipeline->device, texture->sampler, nullptr);
    vkDestroyImageView(pipeline->device, texture->view, nullptr);
    vmaDestroyImage(pipeline->allocator, texture->image, texture->allocation);
}

void hg_pipeline_sprite_bind(HgPipelineSprite *pipeline, VkCommandBuffer cmd) {
    hg_assert(cmd != nullptr);
    hg_assert(pipeline != nullptr);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->pipeline_layout,
        0,
        1,
        &pipeline->vp_set,
        0,
        nullptr);
}

void hg_pipeline_sprite_draw(
    HgPipelineSprite *pipeline,
    VkCommandBuffer cmd,
    HgPipelineSpriteTexture *texture,
    HgPipelineSpritePush *push_data
) {
    hg_assert(cmd != nullptr);

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->pipeline_layout,
        1,
        1,
        &texture->set,
        0,
        nullptr);

    vkCmdPushConstants(
        cmd,
        pipeline->pipeline_layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(*push_data),
        push_data);

    vkCmdDraw(cmd, 4, 1, 0, 0);
}

struct HgWindowInput {
    u32 width;
    u32 height;
    f64 mouse_pos_x;
    f64 mouse_pos_y;
    f64 mouse_delta_x;
    f64 mouse_delta_y;
    bool was_resized;
    bool was_closed;
    bool keys_down[HG_KEY_COUNT];
    bool keys_pressed[HG_KEY_COUNT];
    bool keys_released[HG_KEY_COUNT];
};

#if defined(HG_PLATFORM_LINUX)

#include <dlfcn.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <vulkan/vulkan_xlib.h>

struct HgX11Funcs {
    Display *(*XOpenDisplay)(_Xconst char*);
    int (*XCloseDisplay)(Display*);
    Window (*XCreateWindow)(Display*, Window, int, int, unsigned int, unsigned int,
        unsigned int, int, unsigned int, Visual*, unsigned long, XSetWindowAttributes*);
    int (*XDestroyWindow)(Display*, Window);
    int (*XStoreName)(Display*, Window, _Xconst char*);
    Atom (*XInternAtom)(Display*, _Xconst char*, Bool);
    Status (*XSetWMProtocols)(Display*, Window, Atom*, int);
    int (*XMapWindow)(Display*, Window);
    Status (*XSendEvent)(Display*, Window, Bool, long, XEvent*);
    int (*XFlush)(Display*);
    int (*XNextEvent)(Display*, XEvent*);
    int (*XPending)(Display*);
    KeySym (*XLookupKeysym)(XKeyEvent*, int);
};

static void *hg_internal_libx11 = nullptr;
static HgX11Funcs hg_internal_x11_funcs{};

Display *XOpenDisplay(_Xconst char *name) {
    return hg_internal_x11_funcs.XOpenDisplay(name);
}

int XCloseDisplay(Display *dpy) {
    return hg_internal_x11_funcs.XCloseDisplay(dpy);
}

Window XCreateWindow(
    Display *dpy,
    Window parent,
    int x,
    int y,
    unsigned int width,
    unsigned int height,
    unsigned int border_width,
    int depth,
    unsigned int xclass,
    Visual *visual,
    unsigned long valuemask,
    XSetWindowAttributes *attributes
) {
    return hg_internal_x11_funcs.XCreateWindow(
        dpy, parent, x, y, width, height, border_width,
        depth, xclass, visual, valuemask, attributes
    );
}

int XDestroyWindow(Display *dpy, Window w) {
    return hg_internal_x11_funcs.XDestroyWindow(dpy, w);
}

int XStoreName(Display *dpy, Window w, _Xconst char *name) {
    return hg_internal_x11_funcs.XStoreName(dpy, w, name);
}

Atom XInternAtom(Display *dpy, _Xconst char *name, Bool only_if_exists) {
    return hg_internal_x11_funcs.XInternAtom(dpy, name, only_if_exists);
}

Status XSetWMProtocols(Display *dpy, Window w, Atom *protocols, int count) {
    return hg_internal_x11_funcs.XSetWMProtocols(dpy, w, protocols, count);
}

int XMapWindow(Display *dpy, Window w) {
    return hg_internal_x11_funcs.XMapWindow(dpy, w);
}

Status XSendEvent(Display *dpy, Window w, Bool propagate, long event_mask, XEvent *event) {
    return hg_internal_x11_funcs.XSendEvent(dpy, w, propagate, event_mask, event);
}

int XFlush(Display *dpy) {
    return hg_internal_x11_funcs.XFlush(dpy);
}

int XNextEvent(Display *dpy, XEvent *event) {
    return hg_internal_x11_funcs.XNextEvent(dpy, event);
}

int XPending(Display *dpy) {
    return hg_internal_x11_funcs.XPending(dpy);
}

KeySym XLookupKeysym(XKeyEvent *key_event, int index) {
    return hg_internal_x11_funcs.XLookupKeysym(key_event, index);
}

#define HG_LOAD_X11_FUNC(name) *(void **)&hg_internal_x11_funcs. name \
    = dlsym(hg_internal_libx11, #name); \
    if (hg_internal_x11_funcs. name == nullptr) { hg_error("Could not load Xlib function: \n" #name); }

Display *hg_internal_x11_display = nullptr;

static void hg_internal_platform_init() {
    hg_internal_libx11 = dlopen("libX11.so.6", RTLD_LAZY);
    if (hg_internal_libx11 == nullptr)
        hg_error("Could not open Xlib\n");

    HG_LOAD_X11_FUNC(XOpenDisplay);
    HG_LOAD_X11_FUNC(XCloseDisplay);
    HG_LOAD_X11_FUNC(XCreateWindow);
    HG_LOAD_X11_FUNC(XDestroyWindow);
    HG_LOAD_X11_FUNC(XStoreName);
    HG_LOAD_X11_FUNC(XInternAtom);
    HG_LOAD_X11_FUNC(XSetWMProtocols);
    HG_LOAD_X11_FUNC(XMapWindow);
    HG_LOAD_X11_FUNC(XSendEvent);
    HG_LOAD_X11_FUNC(XFlush);
    HG_LOAD_X11_FUNC(XPending);
    HG_LOAD_X11_FUNC(XNextEvent);
    HG_LOAD_X11_FUNC(XLookupKeysym);

    hg_internal_x11_display = XOpenDisplay(nullptr);
    if (hg_internal_x11_display == nullptr)
        hg_error("Could not open X display\n");
}

static void hg_internal_platform_exit() {
    if (hg_internal_x11_display != nullptr) {
        XCloseDisplay(hg_internal_x11_display);
        hg_internal_x11_display = nullptr;
    }
    if (hg_internal_libx11 != nullptr) {
        dlclose(hg_internal_libx11);
        hg_internal_libx11 = nullptr;
    }
}

static Window hg_internal_create_x11_window(
    Display *display,
    u32 width,
    u32 height,
    const char *title
) {
    XSetWindowAttributes window_attributes{};
    window_attributes.event_mask
        = KeyPressMask | KeyReleaseMask
        | ButtonPressMask
        | ButtonReleaseMask
        | PointerMotionMask
        | StructureNotifyMask;
    Window window = XCreateWindow(
        display, RootWindow(display, DefaultScreen(display)),
        0, 0,
        width,
        height,
        1,
        CopyFromParent,
        InputOutput,
        CopyFromParent,
        CWEventMask,
        &window_attributes
    );
    if (window == ~0U)
        hg_error("X11 could not create window\n");

    if (title != nullptr) {
        int name_result = XStoreName(display, window, title);
        if (name_result == 0)
            hg_error("X11 could not set window title\n");
    }

    int map_result = XMapWindow(display, window);
    if (map_result == 0)
        hg_error("X11 could not map window\n");

    return window;
}

static Atom hg_internal_set_delete_behavior(
    Display* display,
    Window window
) {
    Atom delete_atom = XInternAtom(display, "WM_DELETE_WINDOW", False);
    if (delete_atom == None)
        hg_error("X11 could not get WM_DELETE_WINDOW atom\n");

    int set_protocols_result = XSetWMProtocols(
        display,
        window,
        &delete_atom,
        1
    );
    if (set_protocols_result == 0)
        hg_error("X11 could not set WM_DELETE_WINDOW protocol\n");

    return delete_atom;
}

static void hg_internal_set_fullscreen(
    Display* display,
    Window window
) {
    Atom state_atom = XInternAtom(display, "_NET_WM_STATE", False);
    if (state_atom == None)
        hg_error("X11 failed to get state atom\n");

    Atom fullscreen_atom = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    if (fullscreen_atom == None)
        hg_error("X11 failed to get fullscreen atom\n");

    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = state_atom;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 1;
    event.xclient.data.l[1] = (long)fullscreen_atom;

    int fullscreen_result = XSendEvent(
        display,
        RootWindow(display, DefaultScreen(display)),
        False,
        SubstructureRedirectMask | SubstructureNotifyMask,
        &event
    );
    if (fullscreen_result == 0)
        hg_error("X11 could not send fullscreen message\n");
}

struct HgWindow::Internals {
    HgWindowInput input;
    Window x11_window;
    Atom delete_atom;
};

HgWindow HgWindow::create(const HgWindow::Config& config) {
    HgStdAllocator mem;

    u32 width = config.windowed ? config.width
        : (u32)DisplayWidth(hg_internal_x11_display, DefaultScreen(hg_internal_x11_display));
    u32 height = config.windowed ? config.height
        : (u32)DisplayHeight(hg_internal_x11_display, DefaultScreen(hg_internal_x11_display));

    HgWindow window;
    window.internals = mem.alloc<HgWindow::Internals>();
    *window.internals = {};

    window.internals->input.width = width;
    window.internals->input.height = height;

    window.internals->x11_window = hg_internal_create_x11_window(
        hg_internal_x11_display, width, height, config.title);
    window.internals->delete_atom = hg_internal_set_delete_behavior(
        hg_internal_x11_display, window.internals->x11_window);

    if (!config.windowed)
        hg_internal_set_fullscreen(hg_internal_x11_display, window.internals->x11_window);

    int flush_result = XFlush(hg_internal_x11_display);
    if (flush_result == 0)
        hg_error("X11 could not flush window\n");

    return window;
}

void HgWindow::destroy() {
    if (internals != nullptr) {
        HgStdAllocator mem;

        XDestroyWindow(hg_internal_x11_display, internals->x11_window);
        mem.free(internals);
    }
    XFlush(hg_internal_x11_display);
}

void HgWindow::set_icon(u32 *icon_data, u32 width, u32 height) {
    // window set_icon : TODO
    (void)icon_data;
    (void)width;
    (void)height;
}

bool HgWindow::is_fullscreen() {
    // window is_fullscreen : TODO
    return false;
}

void HgWindow::set_fullscreen(bool fullscreen) {
    // window set_fullscreen : TODO
    (void)fullscreen;
}

void HgWindow::set_cursor(HgWindow::Cursor cursor) {
    // window set_cursor : TODO
    (void)cursor;
}

void HgWindow::set_cursor_image(u32 *data, u32 width, u32 height) {
    // window set_cursor_image : TODO
    (void)data;
    (void)width;
    (void)height;
}

VkSurfaceKHR hg_vk_create_surface(VkInstance instance, HgWindow window) {
    hg_assert(instance != nullptr);
    hg_assert(window.internals != nullptr);

    PFN_vkCreateXlibSurfaceKHR pfn_vkCreateXlibSurfaceKHR
        = (PFN_vkCreateXlibSurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateXlibSurfaceKHR");
    if (pfn_vkCreateXlibSurfaceKHR == nullptr)
        hg_error("Could not load vkCreateXlibSurfaceKHR\n");

    VkXlibSurfaceCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    info.dpy = hg_internal_x11_display;
    info.window = window.internals->x11_window;

    VkSurfaceKHR surface = nullptr;
    VkResult result = pfn_vkCreateXlibSurfaceKHR(instance, &info, nullptr, &surface);
    if (surface == nullptr)
        hg_error("Failed to create Vulkan surface: %s\n", hg_vk_result_string(result));

    return surface;
}

void hg_process_window_events(HgSpan<const HgWindow> windows) {
    hg_assert(windows != nullptr);

    if (windows.count > 1)
        hg_error("Multiple windows unsupported\n"); // : TODO
    HgWindow window = windows[0];

    std::memset(window.internals->input.keys_pressed, 0, sizeof(window.internals->input.keys_pressed));
    std::memset(window.internals->input.keys_released, 0, sizeof(window.internals->input.keys_released));
    window.internals->input.was_resized = false;

    u32 old_window_width = window.internals->input.width;
    u32 old_window_height = window.internals->input.height;
    f64 old_mouse_pos_x = window.internals->input.mouse_pos_x;
    f64 old_mouse_pos_y = window.internals->input.mouse_pos_y;

    while (XPending(hg_internal_x11_display)) {
        XEvent event;
        int event_result = XNextEvent(hg_internal_x11_display, &event);
        if (event_result != 0)
            hg_error("X11 could not get next event\n");

        switch (event.type) {
            case ClientMessage:
                if ((Atom)event.xclient.data.l[0] == window.internals->delete_atom)
                    window.internals->input.was_closed = true;
                break;
            case ConfigureNotify:
                window.internals->input.width = (u32)event.xconfigure.width;
                window.internals->input.height = (u32)event.xconfigure.height;
                break;
            case KeyPress:
            case KeyRelease: {
                HgKey key = HG_KEY_NONE;
                switch (XLookupKeysym(&event.xkey, 0)) {
                    case XK_0:
                        key = HG_KEY_0;
                        break;
                    case XK_1:
                        key = HG_KEY_1;
                        break;
                    case XK_2:
                        key = HG_KEY_2;
                        break;
                    case XK_3:
                        key = HG_KEY_3;
                        break;
                    case XK_4:
                        key = HG_KEY_4;
                        break;
                    case XK_5:
                        key = HG_KEY_5;
                        break;
                    case XK_6:
                        key = HG_KEY_6;
                        break;
                    case XK_7:
                        key = HG_KEY_7;
                        break;
                    case XK_8:
                        key = HG_KEY_8;
                        break;
                    case XK_9:
                        key = HG_KEY_9;
                        break;

                    case XK_q:
                    case XK_Q:
                        key = HG_KEY_Q;
                        break;
                    case XK_w:
                    case XK_W:
                        key = HG_KEY_W;
                        break;
                    case XK_e:
                    case XK_E:
                        key = HG_KEY_E;
                        break;
                    case XK_r:
                    case XK_R:
                        key = HG_KEY_R;
                        break;
                    case XK_t:
                    case XK_T:
                        key = HG_KEY_T;
                        break;
                    case XK_y:
                    case XK_Y:
                        key = HG_KEY_Y;
                        break;
                    case XK_u:
                    case XK_U:
                        key = HG_KEY_U;
                        break;
                    case XK_i:
                    case XK_I:
                        key = HG_KEY_I;
                        break;
                    case XK_o:
                    case XK_O:
                        key = HG_KEY_O;
                        break;
                    case XK_p:
                    case XK_P:
                        key = HG_KEY_P;
                        break;
                    case XK_a:
                    case XK_A:
                        key = HG_KEY_A;
                        break;
                    case XK_s:
                    case XK_S:
                        key = HG_KEY_S;
                        break;
                    case XK_d:
                    case XK_D:
                        key = HG_KEY_D;
                        break;
                    case XK_f:
                    case XK_F:
                        key = HG_KEY_F;
                        break;
                    case XK_g:
                    case XK_G:
                        key = HG_KEY_G;
                        break;
                    case XK_h:
                    case XK_H:
                        key = HG_KEY_H;
                        break;
                    case XK_j:
                    case XK_J:
                        key = HG_KEY_J;
                        break;
                    case XK_k:
                    case XK_K:
                        key = HG_KEY_K;
                        break;
                    case XK_l:
                    case XK_L:
                        key = HG_KEY_L;
                        break;
                    case XK_z:
                    case XK_Z:
                        key = HG_KEY_Z;
                        break;
                    case XK_x:
                    case XK_X:
                        key = HG_KEY_X;
                        break;
                    case XK_c:
                    case XK_C:
                        key = HG_KEY_C;
                        break;
                    case XK_v:
                    case XK_V:
                        key = HG_KEY_V;
                        break;
                    case XK_b:
                    case XK_B:
                        key = HG_KEY_B;
                        break;
                    case XK_n:
                    case XK_N:
                        key = HG_KEY_N;
                        break;
                    case XK_m:
                    case XK_M:
                        key = HG_KEY_M;
                        break;

                    case XK_semicolon:
                        key = HG_KEY_SEMICOLON;
                        break;
                    case XK_colon:
                        key = HG_KEY_COLON;
                        break;
                    case XK_apostrophe:
                        key = HG_KEY_APOSTROPHE;
                        break;
                    case XK_quotedbl:
                        key = HG_KEY_QUOTATION;
                        break;
                    case XK_comma:
                        key = HG_KEY_COMMA;
                        break;
                    case XK_period:
                        key = HG_KEY_PERIOD;
                        break;
                    case XK_question:
                        key = HG_KEY_QUESTION;
                        break;
                    case XK_grave:
                        key = HG_KEY_GRAVE;
                        break;
                    case XK_asciitilde:
                        key = HG_KEY_TILDE;
                        break;
                    case XK_exclam:
                        key = HG_KEY_EXCLAMATION;
                        break;
                    case XK_at:
                        key = HG_KEY_AT;
                        break;
                    case XK_numbersign:
                        key = HG_KEY_HASH;
                        break;
                    case XK_dollar:
                        key = HG_KEY_DOLLAR;
                        break;
                    case XK_percent:
                        key = HG_KEY_PERCENT;
                        break;
                    case XK_asciicircum:
                        key = HG_KEY_CAROT;
                        break;
                    case XK_ampersand:
                        key = HG_KEY_AMPERSAND;
                        break;
                    case XK_asterisk:
                        key = HG_KEY_ASTERISK;
                        break;
                    case XK_parenleft:
                        key = HG_KEY_LPAREN;
                        break;
                    case XK_parenright:
                        key = HG_KEY_RPAREN;
                        break;
                    case XK_bracketleft:
                        key = HG_KEY_LBRACKET;
                        break;
                    case XK_bracketright:
                        key = HG_KEY_RBRACKET;
                        break;
                    case XK_braceleft:
                        key = HG_KEY_LBRACE;
                        break;
                    case XK_braceright:
                        key = HG_KEY_RBRACE;
                        break;
                    case XK_equal:
                        key = HG_KEY_EQUAL;
                        break;
                    case XK_less:
                        key = HG_KEY_LESS;
                        break;
                    case XK_greater:
                        key = HG_KEY_GREATER;
                        break;
                    case XK_plus:
                        key = HG_KEY_PLUS;
                        break;
                    case XK_minus:
                        key = HG_KEY_MINUS;
                        break;
                    case XK_slash:
                        key = HG_KEY_SLASH;
                        break;
                    case XK_backslash:
                        key = HG_KEY_BACKSLASH;
                        break;
                    case XK_underscore:
                        key = HG_KEY_UNDERSCORE;
                        break;
                    case XK_bar:
                        key = HG_KEY_BAR;
                        break;

                    case XK_Up:
                        key = HG_KEY_UP;
                        break;
                    case XK_Down:
                        key = HG_KEY_DOWN;
                        break;
                    case XK_Left:
                        key = HG_KEY_LEFT;
                        break;
                    case XK_Right:
                        key = HG_KEY_RIGHT;
                        break;
                    case XK_Escape:
                        key = HG_KEY_ESCAPE;
                        break;
                    case XK_space:
                        key = HG_KEY_SPACE;
                        break;
                    case XK_Return:
                        key = HG_KEY_ENTER;
                        break;
                    case XK_BackSpace:
                        key = HG_KEY_BACKSPACE;
                        break;
                    case XK_Delete:
                        key = HG_KEY_DELETE;
                        break;
                    case XK_Insert:
                        key = HG_KEY_INSERT;
                        break;
                    case XK_Tab:
                        key = HG_KEY_TAB;
                        break;
                    case XK_Home:
                        key = HG_KEY_HOME;
                        break;
                    case XK_End:
                        key = HG_KEY_END;
                        break;

                    case XK_F1:
                        key = HG_KEY_F1;
                        break;
                    case XK_F2:
                        key = HG_KEY_F2;
                        break;
                    case XK_F3:
                        key = HG_KEY_F3;
                        break;
                    case XK_F4:
                        key = HG_KEY_F4;
                        break;
                    case XK_F5:
                        key = HG_KEY_F5;
                        break;
                    case XK_F6:
                        key = HG_KEY_F6;
                        break;
                    case XK_F7:
                        key = HG_KEY_F7;
                        break;
                    case XK_F8:
                        key = HG_KEY_F8;
                        break;
                    case XK_F9:
                        key = HG_KEY_F9;
                        break;
                    case XK_F10:
                        key = HG_KEY_F10;
                        break;
                    case XK_F11:
                        key = HG_KEY_F11;
                        break;
                    case XK_F12:
                        key = HG_KEY_F12;
                        break;

                    case XK_Shift_L:
                        key = HG_KEY_LSHIFT;
                        break;
                    case XK_Shift_R:
                        key = HG_KEY_RSHIFT;
                        break;
                    case XK_Control_L:
                        key = HG_KEY_LCTRL;
                        break;
                    case XK_Control_R:
                        key = HG_KEY_RCTRL;
                        break;
                    case XK_Meta_L:
                        key = HG_KEY_LMETA;
                        break;
                    case XK_Meta_R:
                        key = HG_KEY_RMETA;
                        break;
                    case XK_Alt_L:
                        key = HG_KEY_LALT;
                        break;
                    case XK_Alt_R:
                        key = HG_KEY_RALT;
                        break;
                    case XK_Super_L:
                        key = HG_KEY_LSUPER;
                        break;
                    case XK_Super_R:
                        key = HG_KEY_RSUPER;
                        break;
                    case XK_Caps_Lock:
                        key = HG_KEY_CAPSLOCK;
                        break;
                }
                if (event.type == KeyPress) {
                    window.internals->input.keys_pressed[key] = true;
                    window.internals->input.keys_down[key] = true;
                } else if (event.type == KeyRelease) {
                    window.internals->input.keys_released[key] = true;
                    window.internals->input.keys_down[key] = false;
                }
            } break;
            case ButtonPress:
            case ButtonRelease: {
                HgKey key = HG_KEY_NONE;
                switch (event.xbutton.button) {
                    case Button1:
                        key = HG_KEY_MOUSE1;
                        break;
                    case Button2:
                        key = HG_KEY_MOUSE2;
                        break;
                    case Button3:
                        key = HG_KEY_MOUSE3;
                        break;
                    case Button4:
                        key = HG_KEY_MOUSE4;
                        break;
                    case Button5:
                        key = HG_KEY_MOUSE5;
                        break;
                }
                if (event.type == ButtonPress) {
                    window.internals->input.keys_pressed[key] = true;
                    window.internals->input.keys_down[key] = true;
                } else if (event.type == ButtonRelease) {
                    window.internals->input.keys_released[key] = true;
                    window.internals->input.keys_down[key] = false;
                }
            } break;
            case MotionNotify:
                window.internals->input.mouse_pos_x = (f64)event.xmotion.x / (f64)window.internals->input.height;
                window.internals->input.mouse_pos_y = (f64)event.xmotion.y / (f64)window.internals->input.height;
                break;
            default:
                break;
        }
    }

    if (window.internals->input.width != old_window_width || window.internals->input.height != old_window_height) {
        window.internals->input.was_resized = true;
    }

    window.internals->input.mouse_delta_x = window.internals->input.mouse_pos_x - old_mouse_pos_x;
    window.internals->input.mouse_delta_y = window.internals->input.mouse_pos_y - old_mouse_pos_y;
}

#elif defined(HG_PLATFORM_WINDOWS)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vulkan/vulkan_win32.h>

HINSTANCE hg_internal_win32_instance = nullptr;

void hg_internal_platform_init() {
    hg_internal_win32_instance = GetModuleHandle(nullptr);
}

void hg_internal_platform_exit() {
    hg_internal_win32_instance = nullptr;
}

struct HgWindow::Internals {
    HgWindowInput input;
    HWND hwnd;
};

static LRESULT CALLBACK hg_internal_window_callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    HgWindow::Internals *window = (HgWindow::Internals *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_NCCREATE:
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)(((CREATESTRUCTA *)lparam)->lpCreateParams));
            break;
        case WM_CLOSE:
            window->input.was_closed = true;
            break;
        case WM_SIZE:
            window->input.width = LOWORD(lparam);
            window->input.height = HIWORD(lparam);
            break;
        case WM_KILLFOCUS:
            std::memset(window->input.keys_down, 0, sizeof(window->input.keys_down));
                break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            HgKey key = HG_KEY_NONE;
            HgKey shift_key = HG_KEY_NONE;

            switch (wparam) {
                case '0':
                    key = HG_KEY_0;
                    shift_key = HG_KEY_RPAREN;
                    break;
                case '1':
                    key = HG_KEY_1;
                    shift_key = HG_KEY_EXCLAMATION;
                    break;
                case '2':
                    key = HG_KEY_2;
                    shift_key = HG_KEY_AT;
                    break;
                case '3':
                    key = HG_KEY_3;
                    shift_key = HG_KEY_HASH;
                    break;
                case '4':
                    key = HG_KEY_4;
                    shift_key = HG_KEY_DOLLAR;
                    break;
                case '5':
                    key = HG_KEY_5;
                    shift_key = HG_KEY_PERCENT;
                    break;
                case '6':
                    key = HG_KEY_6;
                    shift_key = HG_KEY_CAROT;
                    break;
                case '7':
                    key = HG_KEY_7;
                    shift_key = HG_KEY_AMPERSAND;
                    break;
                case '8':
                    key = HG_KEY_8;
                    shift_key = HG_KEY_ASTERISK;
                    break;
                case '9':
                    key = HG_KEY_9;
                    shift_key = HG_KEY_LPAREN;
                    break;

                case 'A':
                    key = HG_KEY_A;
                    break;
                case 'B':
                    key = HG_KEY_B;
                    break;
                case 'C':
                    key = HG_KEY_C;
                    break;
                case 'D':
                    key = HG_KEY_D;
                    break;
                case 'E':
                    key = HG_KEY_E;
                    break;
                case 'F':
                    key = HG_KEY_F;
                    break;
                case 'G':
                    key = HG_KEY_G;
                    break;
                case 'H':
                    key = HG_KEY_H;
                    break;
                case 'I':
                    key = HG_KEY_I;
                    break;
                case 'J':
                    key = HG_KEY_J;
                    break;
                case 'K':
                    key = HG_KEY_K;
                    break;
                case 'L':
                    key = HG_KEY_L;
                    break;
                case 'M':
                    key = HG_KEY_M;
                    break;
                case 'N':
                    key = HG_KEY_N;
                    break;
                case 'O':
                    key = HG_KEY_O;
                    break;
                case 'P':
                    key = HG_KEY_P;
                    break;
                case 'Q':
                    key = HG_KEY_Q;
                    break;
                case 'R':
                    key = HG_KEY_R;
                    break;
                case 'S':
                    key = HG_KEY_S;
                    break;
                case 'T':
                    key = HG_KEY_T;
                    break;
                case 'U':
                    key = HG_KEY_U;
                    break;
                case 'V':
                    key = HG_KEY_V;
                    break;
                case 'W':
                    key = HG_KEY_W;
                    break;
                case 'X':
                    key = HG_KEY_X;
                    break;
                case 'Y':
                    key = HG_KEY_Y;
                    break;
                case 'Z':
                    key = HG_KEY_Z;
                    break;

                case VK_OEM_1:
                    key = HG_KEY_SEMICOLON;
                    shift_key = HG_KEY_COLON;
                    break;
                case VK_OEM_7:
                    key = HG_KEY_APOSTROPHE;
                    shift_key = HG_KEY_QUOTATION;
                    break;
                case VK_OEM_COMMA:
                    key = HG_KEY_COMMA;
                    shift_key = HG_KEY_LESS;
                    break;
                case VK_OEM_PERIOD:
                    key = HG_KEY_PERIOD;
                    shift_key = HG_KEY_GREATER;
                    break;
                case VK_OEM_2:
                    key = HG_KEY_SLASH;
                    shift_key = HG_KEY_QUESTION;
                    break;
                case VK_OEM_3:
                    key = HG_KEY_GRAVE;
                    shift_key = HG_KEY_TILDE;
                    break;
                case VK_OEM_4:
                    key = HG_KEY_LBRACKET;
                    shift_key = HG_KEY_LBRACE;
                    break;
                case VK_OEM_6:
                    key = HG_KEY_RBRACKET;
                    shift_key = HG_KEY_RBRACE;
                    break;
                case VK_OEM_5:
                    key = HG_KEY_BACKSLASH;
                    shift_key = HG_KEY_BAR;
                    break;
                case VK_OEM_PLUS:
                    key = HG_KEY_EQUAL;
                    shift_key = HG_KEY_PLUS;
                    break;
                case VK_OEM_MINUS:
                    key = HG_KEY_MINUS;
                    shift_key = HG_KEY_UNDERSCORE;
                    break;

                case VK_UP:
                    key = HG_KEY_UP;
                    break;
                case VK_DOWN:
                    key = HG_KEY_DOWN;
                    break;
                case VK_LEFT:
                    key = HG_KEY_LEFT;
                    break;
                case VK_RIGHT:
                    key = HG_KEY_RIGHT;
                    break;
                case VK_ESCAPE:
                    key = HG_KEY_ESCAPE;
                    break;
                case VK_SPACE:
                    key = HG_KEY_SPACE;
                    break;
                case VK_RETURN:
                    key = HG_KEY_ENTER;
                    break;
                case VK_BACK:
                    key = HG_KEY_BACKSPACE;
                    break;
                case VK_DELETE:
                    key = HG_KEY_DELETE;
                    break;
                case VK_INSERT:
                    key = HG_KEY_INSERT;
                    break;
                case VK_TAB:
                    key = HG_KEY_TAB;
                    break;
                case VK_HOME:
                    key = HG_KEY_HOME;
                    break;
                case VK_END:
                    key = HG_KEY_END;
                    break;

                case VK_F1:
                    key = HG_KEY_F1;
                    break;
                case VK_F2:
                    key = HG_KEY_F2;
                    break;
                case VK_F3:
                    key = HG_KEY_F3;
                    break;
                case VK_F4:
                    key = HG_KEY_F4;
                    break;
                case VK_F5:
                    key = HG_KEY_F5;
                    break;
                case VK_F6:
                    key = HG_KEY_F6;
                    break;
                case VK_F7:
                    key = HG_KEY_F7;
                    break;
                case VK_F8:
                    key = HG_KEY_F8;
                    break;
                case VK_F9:
                    key = HG_KEY_F9;
                    break;
                case VK_F10:
                    key = HG_KEY_F10;
                    break;
                case VK_F11:
                    key = HG_KEY_F11;
                    break;
                case VK_F12:
                    key = HG_KEY_F12;
                    break;

                case VK_SHIFT: {
                    u32 scancode = (lparam >> 16) & 0xff;
                    if (scancode == 0x36)
                        key = HG_KEY_RSHIFT;
                    else if (scancode == 0x2A)
                        key = HG_KEY_LSHIFT;
                } break;
                case VK_MENU:
                    if (lparam & (1 << 24))
                        key = HG_KEY_RALT;
                    else
                        key = HG_KEY_LALT;
                    break;
                case VK_CONTROL:
                    if (lparam & (1 << 24))
                        key = HG_KEY_RCTRL;
                    else
                        key = HG_KEY_LCTRL;
                    break;
                case VK_LWIN:
                    key = HG_KEY_LSUPER;
                    break;
                case VK_RWIN:
                    key = HG_KEY_RSUPER;
                    break;
                case VK_CAPITAL:
                    key = HG_KEY_CAPSLOCK;
                    break;
            }
            if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
                if (shift_key != HG_KEY_NONE &&
                   (window->input.keys_down[HG_KEY_LSHIFT] ||
                    window->input.keys_down[HG_KEY_RSHIFT])
                ) {
                    window->input.keys_pressed[shift_key] = true;
                    window->input.keys_down[shift_key] = true;
                } else {
                    window->input.keys_pressed[key] = true;
                    window->input.keys_down[key] = true;
                }
            } else {
                window->input.keys_released[shift_key] = window->input.keys_down[shift_key];
                window->input.keys_down[shift_key] = false;
                window->input.keys_released[key] = window->input.keys_down[key];
                window->input.keys_down[key] = false;
            }
        } break;
        case WM_LBUTTONDOWN:
            window->input.keys_pressed[HG_KEY_LMOUSE] = true;
            window->input.keys_down[HG_KEY_LMOUSE] = true;
            break;
        case WM_RBUTTONDOWN:
            window->input.keys_pressed[HG_KEY_RMOUSE] = true;
            window->input.keys_down[HG_KEY_RMOUSE] = true;
            break;
        case WM_MBUTTONDOWN:
            window->input.keys_pressed[HG_KEY_MMOUSE] = true;
            window->input.keys_down[HG_KEY_MMOUSE] = true;
            break;
        case WM_LBUTTONUP:
            window->input.keys_released[HG_KEY_LMOUSE] = true;
            window->input.keys_down[HG_KEY_LMOUSE] = false;
            break;
        case WM_RBUTTONUP:
            window->input.keys_released[HG_KEY_RMOUSE] = true;
            window->input.keys_down[HG_KEY_RMOUSE] = false;
            break;
        case WM_MBUTTONUP:
            window->input.keys_released[HG_KEY_MMOUSE] = true;
            window->input.keys_down[HG_KEY_MMOUSE] = false;
            break;
        case WM_MOUSEMOVE:
            window->input.mouse_pos_x = (f64)LOWORD(lparam) / (f64)window->input.height;
            window->input.mouse_pos_y = (f64)HIWORD(lparam) / (f64)window->input.height;
            break;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

HgWindow HgWindow::create(const HgWindow::Config& config) {
    HgStdAllocator mem;

    const char *title = config.title != nullptr ? config.title : "Hurdy Gurdy";

    HgWindow window;
    window.internals = mem.alloc<HgWindow::Internals>();
    *window.internals = {};

    WNDCLASSA window_class{};
    window_class.hInstance = hg_internal_win32_instance;
    window_class.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    window_class.lpszClassName = title;
    window_class.lpfnWndProc = hg_internal_window_callback;
    if (!RegisterClassA(&window_class))
        hg_error("Win32 failed to register window class for window: %s\n", config.title);

    if (config.windowed) {
        window.internals->input.width = config.width;
        window.internals->input.height = config.height;
        window.internals->hwnd = CreateWindowExA(
            0,
            title,
            title,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            window.internals->input.width,
            window.internals->input.height,
            nullptr,
            nullptr,
            hg_internal_win32_instance,
            window.internals
        );
    } else {
        window.internals->input.width = GetSystemMetrics(SM_CXSCREEN);
        window.internals->input.height = GetSystemMetrics(SM_CYSCREEN);
        window.internals->hwnd = CreateWindowExA(
            0,
            title,
            title,
            WS_POPUP,
            0,
            0,
            window.internals->input.width,
            window.internals->input.height,
            nullptr,
            nullptr,
            hg_internal_win32_instance,
            window.internals
        );
    }
    if (window.internals->hwnd == nullptr)
        hg_error("Win32 window creation failed\n");

    ShowWindow(window.internals->hwnd, SW_SHOW);
    return window;
}

void HgWindow::destroy() {
    if (internals != nullptr) {
        DestroyWindow(internals->hwnd);

        HgStdAllocator mem;
        mem.free(internals);
    }
}

void HgWindow::set_icon(u32 *icon_data, u32 width, u32 height) {
    // window set_icon : TODO
    (void)icon_data;
    (void)width;
    (void)height;
}

bool HgWindow::is_fullscreen() {
    // window is_fullscreen : TODO
    return false;
}

void HgWindow::set_fullscreen(bool fullscreen) {
    // window set_fullscreen : TODO
    (void)fullscreen;
}

void HgWindow::set_cursor(HgWindow::Cursor cursor) {
    // window set_cursor : TODO
    (void)cursor;
}

void HgWindow::set_cursor_image(u32 *data, u32 width, u32 height) {
    // window set_cursor_image : TODO
    (void)data;
    (void)width;
    (void)height;
}

VkSurfaceKHR hg_vk_create_surface(VkInstance instance, HgWindow window) {
    hg_assert(instance != nullptr);
    hg_assert(window.internals != nullptr);

    PFN_vkCreateWin32SurfaceKHR pfn_vkCreateWin32SurfaceKHR
        = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
    if (pfn_vkCreateWin32SurfaceKHR == nullptr)
        hg_error("Could not load vkCreateWin32SurfaceKHR\n");

    VkWin32SurfaceCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.hinstance = hg_internal_win32_instance;
    info.hwnd = window.internals->hwnd;

    VkSurfaceKHR surface = nullptr;
    VkResult result = pfn_vkCreateWin32SurfaceKHR(instance, &info, nullptr, &surface);
    if (surface == nullptr)
        hg_error("Failed to create Vulkan surface: %s\n", hg_vk_result_string(result));

    hg_assert(surface != nullptr);
    return surface;
}

void hg_window_process_events(HgSpan<HgWindow const> windows) {
    hg_assert(windows != nullptr);

    for (usize i = 0; i < windows.count; ++i) {
        HgWindow::Internals *window = windows[i].internals;

        std::memset(window->input.keys_pressed, 0, sizeof(window->input.keys_pressed));
        std::memset(window->input.keys_released, 0, sizeof(window->input.keys_released));
        window->input.was_resized = false;

        u32 old_window_width = window->input.width;
        u32 old_window_height = window->input.height;
        f64 old_mouse_pos_x = window->input.mouse_pos_x;
        f64 old_mouse_pos_y = window->input.mouse_pos_y;

        MSG msg;
        while (PeekMessageA(&msg, window->hwnd, 0, 0, PM_REMOVE) != 0) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        if (window->input.width != old_window_width || window->input.height != old_window_height) {
            window->input.was_resized = true;
        }

        window->input.mouse_delta_x = window->input.mouse_pos_x - old_mouse_pos_x;
        window->input.mouse_delta_y = window->input.mouse_pos_y - old_mouse_pos_y;

        if (window->input.keys_down[HG_KEY_LSHIFT] && window->input.keys_down[HG_KEY_RSHIFT]) {
            bool lshift = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0;
            bool rshift = (GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0;
            if (!lshift) {
                window->input.keys_released[HG_KEY_LSHIFT] = true;
                window->input.keys_down[HG_KEY_LSHIFT] = false;
            }
            if (!rshift) {
                window->input.keys_released[HG_KEY_RSHIFT] = true;
                window->input.keys_down[HG_KEY_RSHIFT] = false;
            }
        }
    }
}

#else

#error "unsupported platform"

#endif

bool HgWindow::was_closed() {
    return internals->input.was_closed;
}

bool HgWindow::was_resized() {
    return internals->input.was_resized;
}

void HgWindow::get_size(u32 *width, u32 *height) {
    *width = internals->input.width;
    *height = internals->input.height;
}

void HgWindow::get_mouse_pos(f64& x, f64& y) {
    x = internals->input.mouse_pos_x;
    y = internals->input.mouse_pos_y;
}

void HgWindow::get_mouse_delta(f64& x, f64& y) {
    x = internals->input.mouse_delta_x;
    y = internals->input.mouse_delta_y;
}

bool HgWindow::is_key_down(HgKey key) {
    hg_assert(key >= 0 && key < HG_KEY_COUNT);
    return internals->input.keys_down[key];
}

bool HgWindow::was_key_pressed(HgKey key) {
    hg_assert(key >= 0 && key < HG_KEY_COUNT);
    return internals->input.keys_pressed[key];
}

bool HgWindow::was_key_released(HgKey key) {
    hg_assert(key >= 0 && key < HG_KEY_COUNT);
    return internals->input.keys_released[key];
}

#undef HG_MAKE_VULKAN_FUNC

static void *hg_internal_libvulkan = nullptr;

#define HG_LOAD_VULKAN_FUNC(name) \
    hg_internal_vulkan_funcs. name = (PFN_##name)hg_internal_vulkan_funcs.vkGetInstanceProcAddr(nullptr, #name); \
    if (hg_internal_vulkan_funcs. name == nullptr) { \
        hg_error("Could not load " #name "\n"); \
    }

void hg_vk_load(void) {

#if defined(HG_PLATFORM_LINUX)

    hg_internal_libvulkan = dlopen("libvulkan.so.1", RTLD_LAZY);
    if (hg_internal_libvulkan == nullptr)
        hg_error("Could not load vulkan dynamic lib: %s\n", dlerror());

    *(void **)&hg_internal_vulkan_funcs.vkGetInstanceProcAddr = dlsym(hg_internal_libvulkan, "vkGetInstanceProcAddr");
    if (hg_internal_vulkan_funcs.vkGetInstanceProcAddr == nullptr)
        hg_error("Could not load vkGetInstanceProcAddr: %s\n", dlerror());

#elif defined(HG_PLATFORM_WINDOWS)

    hg_internal_libvulkan = (void *)LoadLibraryA("vulkan-1.dll");
    if (hg_internal_libvulkan == nullptr)
        hg_error("Could not load vulkan dynamic lib\n");

    hg_internal_vulkan_funcs.vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)
        GetProcAddress((HMODULE)hg_internal_libvulkan, "vkGetInstanceProcAddr");
    if (hg_internal_vulkan_funcs.vkGetInstanceProcAddr == nullptr)
        hg_error("Could not load vkGetInstanceProcAddr\n");

#endif

    HG_LOAD_VULKAN_FUNC(vkCreateInstance);
}

void hg_vk_unload() {

    if (hg_internal_libvulkan != nullptr) {

#if defined(HG_PLATFORM_LINUX)
        dlclose(hg_internal_libvulkan);
#elif defined(HG_PLATFORM_WINDOWS)
        FreeLibrary((HMODULE)hg_internal_libvulkan);
#endif

        hg_internal_libvulkan = nullptr;
    }

}

#undef HG_LOAD_VULKAN_FUNC

