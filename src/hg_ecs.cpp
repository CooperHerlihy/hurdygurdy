#include "hg_core.hpp"
#include "hg_ecs.hpp"
#include "hg_templates.hpp"

namespace hg {

Ecs ecsCreate()
{
    Ecs ecs{};
    ecs.entities = handlePoolCreate();
    ecs.components = mapCreate<u64, Component>(128);
    ecsReset(&ecs);
    return ecs;
}

void ecsDestroy(Ecs* ecs)
{
    ecsReset(ecs);

    mapForEach(&ecs->components, [&](u64*, Component* system)
    {
        arrayAnyDestroy(&system->components);
        arrayDestroy(&system->entities);
        arrayDestroy(&system->indices);
        stringDestroy(&system->name);
    });

    mapDestroy(&ecs->components);
    handlePoolDestroy(&ecs->entities);
}

void ecsReset(Ecs* ecs)
{
    HG_ASSERT(ecs != nullptr);

    mapForEach(&ecs->components, [&](u64*, Component* system)
    {
        for (u32 c = 1; c < system->components.count; ++c)
        {
            system->dtor(system->components[c]);
        }
        system->entities.count = 1;
        system->components.count = 1;
        memClear(system->indices.vals, system->indices.count * sizeof(*system->indices.vals));
    });
    handlePoolReset(&ecs->entities);
}

void ecsRegisterComponent(Ecs* ecs, EcsRegisterComponent* config)
{
    HG_ASSERT(ecs != nullptr);

    u64 id = hash(config->name);
    HG_ASSERT(mapGet(&ecs->components, id) == nullptr);

    if (ecs->components.count * 2 > ecs->components.capacity)
        mapResize(&ecs->components, ecs->components.capacity * 2);
    Component* system = mapAdd(&ecs->components, id, {});

    system->name = stringCreate(config->name);
    system->indices = arrayCreate<u32>();
    system->entities = arrayCreate<Entity>();
    system->components = arrayAnyCreate(config->width, config->align);
    system->dtor = config->dtor;
    system->serialize = config->serialize;

    arrayPush(&system->entities);
    arrayAnyPush(&system->components);
}

void ecsUnregisterComponent(Ecs* ecs, u64 componentId)
{
    Component* system = mapGet(&ecs->components, componentId);

    for (u32 c = 1; c < system->components.count; ++c)
    {
        system->dtor(system->components[c]);
    }
    arrayAnyDestroy(&system->components);
    arrayDestroy(&system->entities);
    arrayDestroy(&system->indices);
    stringDestroy(&system->name);

    mapRemove(&ecs->components, componentId);
}

String ecsComponentName(Ecs* ecs, u64 componentId)
{
    HG_ASSERT(ecs != nullptr);
    Component* system = mapGet(&ecs->components, componentId);
    HG_ASSERT(system != nullptr);
    return system->name;
}

Entity ecsSpawn(Ecs* ecs)
{
    HG_ASSERT(ecs != nullptr);
    return {handlePoolAlloc(&ecs->entities)};
}

void ecsDespawn(Ecs* ecs, Entity e)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(ecsAlive(ecs, e));

    mapForEach(&ecs->components, [&](u64* id, Component*)
    {
        if (ecsHas(ecs, e, *id))
            ecsRemove(ecs, e, *id);
    });
    handlePoolFree(&ecs->entities, e.handle);
}

bool ecsAlive(Ecs* ecs, Entity e)
{
    HG_ASSERT(ecs != nullptr);
    return handlePoolAlive(&ecs->entities, e.handle);
}

void* ecsAdd(Ecs* ecs, Entity e, u64 componentId)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(ecsAlive(ecs, e));
    HG_ASSERT(!ecsHas(ecs, e, componentId));

    Component* system = mapGet(&ecs->components, componentId);
    HG_ASSERT(system != nullptr);

    u32 idx = handleIdx(e.handle);
    if (idx >= system->indices.count)
    {
        u32 oldCount = system->indices.count;
        u32 newCount = idx * 2;
        arrayResize(&system->indices, newCount);
        for (u32 i = oldCount; i < newCount; ++i)
            system->indices[i] = 0;
    }
    system->indices[idx] = system->entities.count;
    *arrayPush(&system->entities) = e;
    return arrayAnyPush(&system->components);
}

void ecsRemove(Ecs* ecs, Entity e, u64 componentId)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(ecsAlive(ecs, e));
    HG_ASSERT(ecsHas(ecs, e, componentId));

    Component* system = mapGet(&ecs->components, componentId);
    HG_ASSERT(system != nullptr);

    u32 idx = system->indices[handleIdx(e.handle)];
    system->dtor(system->components[idx]);

    Entity last = arrayPop(&system->entities);
    if (e != last)
    {
        system->entities[idx] = last;
        system->indices[handleIdx(last.handle)] = idx;
        memCopy(
            system->components[idx],
            system->components[system->components.count - 1],
            system->components.width);
    }
    system->indices[handleIdx(e.handle)] = 0;
    --system->components.count;
}

bool ecsHas(Ecs* ecs, Entity e, u64 componentId)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(ecsAlive(ecs, e));

    Component* system = mapGet(&ecs->components, componentId);
    if (system == nullptr)
        return false;

    u32 idx = handleIdx(e.handle);
    return idx < system->indices.count && system->indices[idx] != 0;
}

void* ecsGet(Ecs* ecs, Entity e, u64 componentId)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(ecsAlive(ecs, e));

    Component* system = mapGet(&ecs->components, componentId);
    HG_ASSERT(system != nullptr);
    HG_ASSERT(system->indices[handleIdx(e.handle)] != 0);
    HG_ASSERT(system->indices[handleIdx(e.handle)] < system->entities.count);

    return system->components[system->indices[handleIdx(e.handle)]];
}

Entity ecsGetEntity(Ecs* ecs, const void* component, u64 componentId)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(component != nullptr);

    Component* system = mapGet(&ecs->components, componentId);
    HG_ASSERT(system != nullptr);

    return system->entities[(u32)((uptr)component - (uptr)system->components.vals) / system->components.width];
}

Entity* ecsEntities(Ecs* ecs, u64 componentId)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(mapGet(&ecs->components, componentId) != nullptr);
    HG_ASSERT(mapGet(&ecs->components, componentId)->entities.count != 0);
    return mapGet(&ecs->components, componentId)->entities.vals + 1;
}

void* ecsComponents(Ecs* ecs, u64 componentId)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(mapGet(&ecs->components, componentId) != nullptr);
    HG_ASSERT(mapGet(&ecs->components, componentId)->components.count != 0);
    Component* system = mapGet(&ecs->components, componentId);
    return (u8*)system->components.vals + system->components.width;
}

u32 ecsCount(Ecs* ecs, u64 componentId)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(mapGet(&ecs->components, componentId) != nullptr);
    HG_ASSERT(mapGet(&ecs->components, componentId)->entities.count != 0);
    return mapGet(&ecs->components, componentId)->entities.count - 1;
}

u64 ecsFindSmallest(Ecs* ecs, u64* ids, u32 idCount)
{
    HG_ASSERT(ecs != nullptr);

    u32 smallestCount = (u32)-1;
    u64 smallest = ids[0];

    for (u32 i = 1; i < idCount; ++i)
    {
        Component* system = mapGet(&ecs->components, ids[i]);
        HG_ASSERT(system != nullptr);

        if (system->entities.count < smallestCount)
        {
            smallestCount = system->entities.count;
            smallest = ids[i];
        }
    }
    return smallest;
}

static void swapIdxLocation(Ecs* ecs, u32 lhs, u32 rhs, u64 componentId)
{
    HG_ASSERT(ecs != nullptr);

    Component* system = mapGet(&ecs->components, componentId);
    HG_ASSERT(system != nullptr);

    HG_ASSERT(lhs != 0 && lhs < system->entities.count);
    HG_ASSERT(rhs != 0 && rhs < system->entities.count);

    Entity lhsEntity = system->entities[lhs];
    Entity rhsEntity = system->entities[rhs];

    HG_ASSERT(ecsAlive(ecs, lhsEntity));
    HG_ASSERT(ecsAlive(ecs, rhsEntity));
    HG_ASSERT(ecsHas(ecs, lhsEntity, componentId));
    HG_ASSERT(ecsHas(ecs, rhsEntity, componentId));

    Arena* sc = scratch();
    HG_ARENA_SCOPE(sc);

    system->entities[lhs] = rhsEntity;
    system->entities[rhs] = lhsEntity;
    system->indices[handleIdx(lhsEntity.handle)] = rhs;
    system->indices[handleIdx(rhsEntity.handle)] = lhs;

    void* temp = arenaAlloc(sc, system->components.width, 1);
    memCopy(temp, system->components[lhs], system->components.width);
    memCopy(system->components[lhs], system->components[rhs], system->components.width);
    memCopy(system->components[rhs], temp, system->components.width);
}

namespace {
    struct QuicksortData {
        Ecs* ecs;
        Component* system;
        u64 comp;
        void* data;
        bool (*compare)(void*, Ecs* ecs, Entity lhs, Entity rhs);

        u32 quicksortInter(u32 pivot, u32 inc, u32 dec)
        {
            while (inc != dec)
            {
                while (!compare(data, ecs, system->entities[dec], system->entities[pivot]))
                {
                    --dec;
                    if (dec == inc)
                        goto finish;
                }
                while (!compare(data, ecs, system->entities[pivot], system->entities[inc]))
                {
                    ++inc;
                    if (inc == dec)
                        goto finish;
                }
                swapIdxLocation(ecs, inc, dec, comp);
            }

        finish:
            if (compare(data, ecs, system->entities[inc], system->entities[pivot]))
                swapIdxLocation(ecs, pivot, inc, comp);

            return inc;
        }

        void quicksort(u32 begin, u32 end)
        {
            if (begin + 1 >= end)
                return;

            u32 middle = quicksortInter(begin, begin + 1, end - 1);
            quicksort(begin, middle);
            quicksort(middle, end);
        }
    };
}

void ecsSort(
    Ecs* ecs,
    u64 componentId,
    void* data,
    bool (*compare)(void*, Ecs* ecs, Entity lhs, Entity rhs))
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(compare != nullptr);

    Component* system = mapGet(&ecs->components, componentId);
    HG_ASSERT(system != nullptr);

    QuicksortData q{ecs, system, componentId, data, compare};
    q.quicksort(1, system->entities.count);
}

template<>
void serialize(Serializer* s, Ecs* ecs)
{
    HG_ASSERT(s != nullptr);
    HG_ASSERT(ecs != nullptr);

    Arena* sc = scratch(&s->arena, 1);
    HG_ARENA_SCOPE(sc);

    serializeBegin(s);
    HG_DEFER(serializeEnd(s));

    EntitySerializer ecsSerial{};
    u32 entityCount = 0;
    if (s->writing)
    {
        ecsSerial.entityToIdx = arenaAlloc<u32>(sc, ecs->entities.handles.count);
        for (u32 i = 1; i < ecs->entities.handles.count; ++i)
        {
            if (ecs->entities.handles[i] != handleNull)
                ecsSerial.entityToIdx[handleIdx(ecs->entities.handles[i])] = entityCount++;
        }

        serialize(s, &entityCount);
    }
    else
    {
        serialize(s, &entityCount);

        ecsSerial.idxToEntity = arenaAlloc<Entity>(sc, entityCount);
        for (u32 i = 0; i < entityCount; ++i)
        {
            ecsSerial.idxToEntity[i] = ecsSpawn(ecs);
        }
    }

    u32 systemCount;
    if (s->writing)
        systemCount = ecs->components.count;
    serializeBegin(s, &systemCount);
    HG_DEFER(serializeEnd(s));

    u32 systemIdx = (u32)-1;
    for (u32 i = 0; i < systemCount; ++i)
    {
        serializeBegin(s);
        HG_DEFER(serializeEnd(s));

        u64 systemId = (u64)-1;
        Component* system;
        if (s->writing)
        {
            ++systemIdx;
            while (!ecs->components.hasVal[systemIdx])
            {
                ++systemIdx;
            }
            systemId = ecs->components.keys[systemIdx];
            system = &ecs->components.vals[systemIdx];
            serialize(s, &system->name);
        }
        else
        {
            String compName;
            serialize(s, &compName);
            systemId = hash(compName);
            system = mapGet(&ecs->components, systemId);
        }

        u32 compCount;
        if (s->writing)
            compCount = system->entities.count - 1;
        serializeBegin(s, &compCount);
        HG_DEFER(serializeEnd(s));

        for (u32 c = 0; c < compCount; ++c)
        {
            serializeBegin(s);
            HG_DEFER(serializeEnd(s));

            u32 entityIdx;
            if (s->writing)
                entityIdx = ecsSerial.entityToIdx[handleIdx(system->entities[c + 1].handle)];
            serialize(s, &entityIdx);

            void* compData;
            if (s->writing)
                compData = system->components[c + 1];
            else
                compData = ecsAdd(ecs, ecsSerial.idxToEntity[entityIdx], systemId);
            system->serialize(s, compData, &ecsSerial);
        }
    }
}

void entitySerialize(Serializer* s, Entity* val, EntitySerializer* ecs)
{
    if (s->writing)
    {
        u32 idx = *val != entityNull ? ecs->entityToIdx[handleIdx(val->handle)] : (u32)-1;
        serialize(s, (i32*)&idx);
    }
    else
    {
        u32 idx = (u32)-1;
        serialize(s, (i32*)&idx);
        *val = idx != (u32)-1 ? ecs->idxToEntity[idx] : entityNull;
    }
}

template<>
void ecsSerialize(Serializer* s, Node* node, EntitySerializer* ecs)
{
    serializeBegin(s);
    entitySerialize(s, &node->parent, ecs);
    entitySerialize(s, &node->nextSibling, ecs);
    entitySerialize(s, &node->prevSibling, ecs);
    entitySerialize(s, &node->firstChild, ecs);
    serializeEnd(s);
}

Node* nodeAdd(Ecs* ecs, Entity e)
{
    Node* node = ecsAdd<Node>(ecs, e);
    *node = {};
    return node;
}

void nodeDestroy(Ecs* ecs, Entity e)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(ecsAlive(ecs, e));

    Node* node = ecsGet<Node>(ecs, e);
    if (node->parent.handle != handleNull)
    {
        if (node->prevSibling.handle != handleNull)
            ecsGet<Node>(ecs, node->prevSibling)->nextSibling = node->nextSibling;
        else
            ecsGet<Node>(ecs, node->parent)->firstChild = node->nextSibling;

        if (node->nextSibling.handle != handleNull)
            ecsGet<Node>(ecs, node->nextSibling)->prevSibling = node->prevSibling;
    }

    Entity child = node->firstChild;
    while (child.handle != handleNull)
    {
        Node* childNode = ecsGet<Node>(ecs, child);
        Entity next = childNode->nextSibling;
        nodeDestroy(ecs, child);
        child = next;
    }

    ecsDespawn(ecs, e);
}

void nodeDetach(Ecs* ecs, Entity e)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(ecsAlive(ecs, e));

    Node* node = ecsGet<Node>(ecs, e);
    if (node->parent.handle == handleNull)
    {
        HG_ASSERT(node->prevSibling.handle == handleNull);
        HG_ASSERT(node->nextSibling.handle == handleNull);

        Entity child = node->firstChild;
        while (child.handle != handleNull)
        {
            Node* childNode = ecsGet<Node>(ecs, child);
            Entity next = childNode->nextSibling;
            childNode->parent = Entity{};
            childNode->nextSibling = Entity{};
            childNode->prevSibling = Entity{};
            child = next;
        }
    } else {
        if (node->prevSibling.handle != handleNull)
            ecsGet<Node>(ecs, node->prevSibling)->nextSibling = node->nextSibling;
        else
            ecsGet<Node>(ecs, node->parent)->firstChild = node->nextSibling;

        if (node->nextSibling.handle != handleNull)
            ecsGet<Node>(ecs, node->nextSibling)->prevSibling = node->prevSibling;

        Entity child = node->firstChild;
        while (child.handle != handleNull)
        {
            Node* childNode = ecsGet<Node>(ecs, child);
            Entity next = childNode->nextSibling;
            childNode->parent = Entity{};
            childNode->nextSibling = Entity{};
            childNode->prevSibling = Entity{};
            nodeAddChild(ecs, node->parent, child);
            child = next;
        }
    }
    *node = {};
}

void nodeAddChild(Ecs* ecs, Entity parent, Entity child)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(ecsAlive(ecs, parent));
    HG_ASSERT(ecsAlive(ecs, child));

    Node* node = ecsGet<Node>(ecs, parent);
    Node* childNode = ecsGet<Node>(ecs, child);

    HG_ASSERT(childNode->parent.handle == handleNull);
    HG_ASSERT(childNode->prevSibling.handle == handleNull);
    HG_ASSERT(childNode->nextSibling.handle == handleNull);

    if (node->firstChild.handle != handleNull)
    {
        ecsGet<Node>(ecs, node->firstChild)->prevSibling = child;
        childNode->nextSibling = node->firstChild;
    }
    node->firstChild = child;
    childNode->parent = parent;
}

template<>
void serialize(Serializer* s, Transform* node)
{
    serializeObject(s,
        &node->position,
        &node->scale,
        &node->rotation);
}

Transform* transformAdd(Ecs* ecs, Entity e, Vec3 position, Vec3 scale, Quat rotation)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(ecsAlive(ecs, e));

    Transform* tf = ecsAdd<Transform>(ecs, e);
    *tf = {};
    tf->position = position;
    tf->scale = scale;
    tf->rotation = rotation;
    transformUpdate(ecs, e);

    return tf;
}

static void transformUpdateChild(Ecs* ecs, Entity e)
{
    HG_ASSERT(ecsHas<Transform>(ecs, e));

    Node* node = ecsGet<Node>(ecs, e);

    Transform* tf = ecsGet<Transform>(ecs, e);

    tf->mat = ecsGet<Transform>(ecs, node->parent)->mat
            * matModel3D(tf->position, tf->scale, tf->rotation);

    Entity child = node->firstChild;
    while (child.handle != handleNull)
    {
        transformUpdate(ecs, child);
        child = ecsGet<Node>(ecs, child)->nextSibling;
    }
}

void transformUpdate(Ecs* ecs, Entity e)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(ecsAlive(ecs, e));
    HG_ASSERT(ecsHas<Transform>(ecs, e));

    if (ecsHas<Node>(ecs, e))
    {
        Node* node = ecsGet<Node>(ecs, e);
        if (node->parent.handle != handleNull && ecsHas<Transform>(ecs, node->parent))
        {
            transformUpdateChild(ecs, e);
        }
        else
        {
            Transform* tf = ecsGet<Transform>(ecs, e);
            tf->mat = matModel3D(tf->position, tf->scale, tf->rotation);

            Entity child = node->firstChild;
            while (child.handle != handleNull)
            {
                transformUpdateChild(ecs, child);
                child = ecsGet<Node>(ecs, child)->nextSibling;
            }
        }
    }
    else
    {
        Transform* tf = ecsGet<Transform>(ecs, e);
        tf->mat = matModel3D(tf->position, tf->scale, tf->rotation);
    }
}

} // namespace hg

