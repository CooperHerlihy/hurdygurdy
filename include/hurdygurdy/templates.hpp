#pragma once

template<typename... Ts> requires (sizeof...(Ts) > 0)
void setError(StringView errorFmt, Ts... args)
{
    char fmt[4096];
    u64 fmtLen = errorFmt.length < sizeof(fmt) - 1
        ? errorFmt.length
        : sizeof(fmt) - 1;
    memcpy(fmt, errorFmt.chars, fmtLen);
    fmt[fmtLen] = 0;

    char buf[4096];
    snprintf(buf, sizeof(buf), fmt, args...);

    setError(buf);
}

template<typename T>
T Maybe<T>::orElse(const T& defaultVal)
{
    if (has)
    {
        T tmp = std::move(val);
        val.~T();
        has = false;
        return tmp;
    }
    else
    {
        return defaultVal;
    }
}

template<typename T>
T Maybe<T>::orElse(T&& defaultVal)
{
    if (has)
    {
        T tmp = std::move(val);
        val.~T();
        has = false;
        return tmp;
    }
    else
    {
        return std::move(defaultVal);
    }
}

template<typename T>
T Maybe<T>::expect([[maybe_unused]] StringView errMsg)
{
    if (has)
    {
        T tmp = std::move(val);
        val.~T();
        has = false;
        return tmp;
    }
    else
    {
        HG_PANIC("%.*s", (int)errMsg.length, errMsg.chars);
    }
}

template<typename T>
Maybe<T>::Maybe(const Maybe& other)
{
    if (other.has)
        new (&val) T{other.val};
    has = other.has;
}

template<typename T>
Maybe<T>& Maybe<T>::operator=(const Maybe<T>& other)
{
    if (this != &other)
    {
        if (has)
            val.~T();

        if (other.has)
            new (&val) T{other.val};
        has = other.has;
    }
    return *this;
}

template<typename T>
Maybe<T>::Maybe(Maybe&& other) noexcept
{
    if (other.has)
        new (&val) T{std::move(other.val)};
    has = other.has;

    if (other.has)
        other.val.~T();
    other.has = false;
}

template<typename T>
Maybe<T>& Maybe<T>::operator=(Maybe&& other) noexcept
{
    if (this != &other)
    {
        this->~Maybe();
        new (this) Maybe{std::move(other)};
    }
    return *this;
}

template<typename F> requires std::is_invocable_r_v<void, F, u64>
void forPar(u64 begin, u64 end, F fn)
{

    forPar(begin, end, &fn, [](void* pfn, u64 idx)
    {
        (*static_cast<F*>(pfn))(idx);
    });
}

template<typename T>
UniquePtr<T>::~UniquePtr() noexcept
{
    if (ptr != nullptr)
    {
        ptr->~T();
        heapFree(ptr, 1);
    }
}

template<typename T, typename... Args>
UniquePtr<T> makeUnique(Args&&... args)
{
    UniquePtr<T> ptr{};
    ptr.ptr = new (heapAlloc(sizeof(T), alignof(T))) T{std::forward<Args>(args)...};
    return ptr;
}

template<typename T>
SharedPtr<T>::~SharedPtr() noexcept
{
    if (ptr != nullptr && --ptr->refCount == 0)
    {
        ptr->val.~T();
        heapFree(ptr, sizeof(Block));
    }
}

template<typename T>
SharedPtr<T> SharedPtr<T>::clone() const
{
    SharedPtr other{};
    other.ptr = ptr;
    ++ptr->refCount;
    return other;
}

template<typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args)
{
    using Block = SharedPtr<T>::Block;

    SharedPtr<T> ptr{};
    ptr.ptr = new (heapAlloc(sizeof(Block), alignof(Block)))
        Block{1, std::forward<Args>(args)...};
    return ptr;
}

template<typename T>
Array<T>::Array(u64 countVal, u64 capacityVal)
    : vals{heapAlloc<T>(capacityVal)}
    , count{countVal}
    , capacity{capacityVal}
{
    HG_ASSERT(capacity >= count);
    for (u64 i = 0; i < count; ++i)
    {
        new (vals + i) T{};
    }
}

template<typename T>
Array<T>::~Array() noexcept
{
    for (u64 i = 0; i < count; ++i)
    {
        vals[i].~T();
    }
    heapFree(vals, capacity);
}

template<typename T>
void Array<T>::reset()
{
    for (u64 i = 0; i < count; ++i)
    {
        vals[i].~T();
    }
    count = 0;
}

template<typename T>
void Array<T>::resize(u64 newCount)
{
    if (newCount > capacity)
        reserve(newCount * 2);

    for (u64 i = count; i < newCount; ++i)
    {
        new (vals + i) T{};
    }
    count = newCount;
}

template<typename T>
void Array<T>::reserve(u64 newCapacity)
{
    if (newCapacity > capacity)
    {
        T* newVals = heapAlloc<T>(newCapacity);
        for (u64 i = 0; i < count; ++i)
        {
            new (newVals + i) T{std::move(vals[i])};
            vals[i].~T();
        }
        heapFree(vals, capacity);
        vals = newVals;
        capacity = newCapacity;
    }
}

template<typename T>
T* Array<T>::push()
{
    if (count == capacity)
        reserve(capacity == 0 ? 64 : capacity * 2);

    new (vals + count) T{};
    return vals + count++;
}

template<typename T>
T* Array<T>::push(const T& val)
{
    if (count == capacity)
        reserve(capacity == 0 ? 64 : capacity * 2);

    new (vals + count) T{val};
    return vals + count++;
}

template<typename T>
T* Array<T>::push(T&& val)
{
    if (count == capacity)
        reserve(capacity == 0 ? 64 : capacity * 2);

    new (vals + count) T{std::move(val)};
    return vals + count++;
}

template<typename T>
T Array<T>::pop()
{
    HG_ASSERT(count > 0);

    --count;
    T ret = std::move(vals[count]);
    vals[count].~T();
    return ret;
}

template<typename T>
T* Array<T>::insertShift(u64 idx, const T& val)
{
    HG_ASSERT(idx <= count);

    if (count == capacity)
        reserve(capacity == 0 ? 64 : capacity * 2);

    if (idx < count)
    {
        new (vals + count) T{std::move(vals[count - 1])};
        for (u64 i = count - 1; i >= idx + 1; --i)
        {
            vals[i] = std::move(vals[i - 1]);
        }
        vals[idx] = val;
    }
    else
    {
        new (vals + count) T{val};
    }
    return vals + count++;
}

template<typename T>
T* Array<T>::insertShift(u64 idx, T&& val)
{
    HG_ASSERT(idx <= count);

    if (count == capacity)
        reserve(capacity == 0 ? 64 : capacity * 2);

    if (idx < count)
    {
        new (vals + count) T{std::move(vals[count - 1])};
        for (u64 i = count - 1; i >= idx + 1; --i)
        {
            vals[i] = std::move(vals[i - 1]);
        }
        vals[idx] = std::move(val);
    }
    else
    {
        new (vals + count) T{std::move(val)};
    }
    return vals + count++;
}

template<typename T>
T Array<T>::removeShift(u64 idx)
{
    HG_ASSERT(idx < count);

    --count;
    T ret = std::move(vals[idx]);
    for (u64 i = idx; i < count; ++i)
    {
        vals[i] = std::move(vals[i + 1]);
    }
    vals[count].~T();
    return ret;
}

template<typename T>
T* Array<T>::insertSwap(u64 idx, const T& val)
{
    HG_ASSERT(idx <= count);

    if (count == capacity)
        reserve(capacity == 0 ? 64 : capacity * 2);

    if (idx < count)
    {
        new (vals + count) T{std::move(vals[idx])};
        vals[idx] = val;
    }
    else
    {
        new (vals + count) T{val};
    }
    return vals + count++;
}

template<typename T>
T* Array<T>::insertSwap(u64 idx, T&& val)
{
    HG_ASSERT(idx <= count);

    if (count == capacity)
        reserve(capacity == 0 ? 64 : capacity * 2);

    if (idx < count)
    {
        new (vals + count) T{std::move(vals[idx])};
        vals[idx] = std::move(val);
    }
    else
    {
        new (vals + count) T{std::move(val)};
    }
    return vals + count++;
}

template<typename T>
T Array<T>::removeSwap(u64 idx)
{
    HG_ASSERT(idx < count);

    --count;
    T ret = std::move(vals[idx]);
    if (idx < count)
    {
        vals[idx] = std::move(vals[count]);
    }
    vals[count].~T();
    return ret;
}

template<typename T>
ArrayTemp<T>::ArrayTemp(Arena* arenaVal, u64 countVal, u64 capacityVal)
    : arena{arenaVal}
    , vals{arenaVal->alloc<T>(capacityVal)}
    , count{countVal}
    , capacity{capacityVal}
{
    HG_ASSERT(capacity >= count);
    for (u64 i = 0; i < count; ++i)
    {
        new (vals + i) T{};
    }
}

template<typename T>
ArrayTemp<T>::~ArrayTemp() noexcept
{
    for (u64 i = 0; i < count; ++i)
    {
        vals[i].~T();
    }
}

template<typename T>
void ArrayTemp<T>::reset()
{
    for (u64 i = 0; i < count; ++i)
    {
        vals[i].~T();
    }
    count = 0;
}

template<typename T>
void ArrayTemp<T>::resize(u64 newCount)
{
    if (newCount > capacity)
        reserve(newCount * 2);

    for (u64 i = count; i < newCount; ++i)
    {
        new (vals + i) T{};
    }
    count = newCount;
}

template<typename T>
void ArrayTemp<T>::reserve(u64 newCapacity)
{
    if (newCapacity > capacity)
    {
        if (!arena->extend(vals, capacity, newCapacity))
        {
            T* newVals = arena->alloc<T>(newCapacity);
            for (u64 i = 0; i < count; ++i)
            {
                new (newVals + i) T{std::move(vals[i])};
                vals[i].~T();
            }
            vals = newVals;
        }
        capacity = newCapacity;
    }
}

template<typename T>
T* ArrayTemp<T>::push()
{
    if (count == capacity)
        reserve(capacity == 0 ? 64 : capacity * 2);

    new (vals + count) T{};
    return vals + count++;
}

template<typename T>
T* ArrayTemp<T>::push(const T& val)
{
    if (count == capacity)
        reserve(capacity == 0 ? 64 : capacity * 2);

    new (vals + count) T{val};
    return vals + count++;
}

template<typename T>
T* ArrayTemp<T>::push(T&& val)
{
    if (count == capacity)
        reserve(capacity == 0 ? 64 : capacity * 2);

    new (vals + count) T{std::move(val)};
    return vals + count++;
}

template<typename T>
T ArrayTemp<T>::pop()
{
    HG_ASSERT(count > 0);

    --count;
    T ret = std::move(vals[count]);
    vals[count].~T();
    return ret;
}

template<typename T>
T* ArrayTemp<T>::insertShift(u64 idx, const T& val)
{
    HG_ASSERT(idx <= count);

    if (count == capacity)
        reserve(capacity == 0 ? 64 : capacity * 2);

    if (idx < count)
    {
        new (vals + count) T{std::move(vals[count - 1])};
        for (u64 i = count - 1; i >= idx + 1; --i)
        {
            vals[i] = std::move(vals[i - 1]);
        }
        vals[idx] = val;
    }
    else
    {
        new (vals + count) T{val};
    }
    return vals + count++;
}

template<typename T>
T* ArrayTemp<T>::insertShift(u64 idx, T&& val)
{
    HG_ASSERT(idx <= count);

    if (count == capacity)
        reserve(capacity == 0 ? 64 : capacity * 2);

    if (idx < count)
    {
        new (vals + count) T{std::move(vals[count - 1])};
        for (u64 i = count - 1; i >= idx + 1; --i)
        {
            vals[i] = std::move(vals[i - 1]);
        }
        vals[idx] = std::move(val);
    }
    else
    {
        new (vals + count) T{std::move(val)};
    }
    return vals + count++;
}

template<typename T>
T ArrayTemp<T>::removeShift(u64 idx)
{
    HG_ASSERT(idx < count);

    --count;
    T ret = std::move(vals[idx]);
    for (u64 i = idx; i < count; ++i)
    {
        vals[i] = std::move(vals[i + 1]);
    }
    vals[count].~T();
    return ret;
}

template<typename T>
T* ArrayTemp<T>::insertSwap(u64 idx, const T& val)
{
    HG_ASSERT(idx <= count);

    if (count == capacity)
        reserve(capacity == 0 ? 64 : capacity * 2);

    if (idx < count)
    {
        new (vals + count) T{std::move(vals[idx])};
        vals[idx] = val;
    }
    else
    {
        new (vals + count) T{val};
    }
    return vals + count++;
}

template<typename T>
T* ArrayTemp<T>::insertSwap(u64 idx, T&& val)
{
    HG_ASSERT(idx <= count);

    if (count == capacity)
        reserve(capacity == 0 ? 64 : capacity * 2);

    if (idx < count)
    {
        new (vals + count) T{std::move(vals[idx])};
        vals[idx] = std::move(val);
    }
    else
    {
        new (vals + count) T{std::move(val)};
    }
    return vals + count++;
}

template<typename T>
T ArrayTemp<T>::removeSwap(u64 idx)
{
    HG_ASSERT(idx < count);

    --count;
    T ret = std::move(vals[idx]);
    if (idx < count)
    {
        vals[idx] = std::move(vals[count]);
    }
    vals[count].~T();
    return ret;
}

template<typename T>
Queue<T>::Queue(u64 capacityVal)
    : vals{heapAlloc<T>(capacityVal)}
    , front{0}
    , back{0}
    , count{0}
    , capacity{capacityVal}
{}

template<typename T>
Queue<T>::~Queue() noexcept
{
    if (back != front)
        HG_WARN("Non-empty queue destroyed\n");

    for (u64 i = front; i != back; i = (i + 1) % capacity)
    {
        vals[i].~T();
    }
    heapFree(vals, capacity);
}

template<typename T>
void Queue<T>::reserve(u64 newCapacity)
{
    if (newCapacity > capacity)
    {
        T* newVals = heapAlloc<T>(newCapacity);

        T* nextVal = newVals;
        for (u64 i = front; i != back; i = (i + 1) % capacity)
        {
            new (nextVal++) T{std::move(vals[i])};
            vals[i].~T();
        }

        heapFree(vals, capacity);
        vals = newVals;
        capacity = newCapacity;
        front = 0;
        back = count;
    }
}

template<typename T>
void Queue<T>::pushFront(const T& val)
{
    if (++count >= capacity)
        reserve(capacity == 0 ? 128 : capacity * 2);

    front = (front == 0 ? capacity : front) - 1;
    new (vals + front) T{val};
}

template<typename T>
void Queue<T>::pushFront(T&& val)
{
    if (++count >= capacity)
        reserve(capacity == 0 ? 128 : capacity * 2);

    front = (front == 0 ? capacity : front) - 1;
    new (vals + front) T{std::move(val)};
}

template<typename T>
void QueueTemp<T>::pushFront(const T& val)
{
    if (++count >= capacity)
        reserve(capacity == 0 ? 128 : capacity * 2);

    front = (front == 0 ? capacity : front) - 1;
    new (vals + front) T{val};
}

template<typename T>
void QueueTemp<T>::pushFront(T&& val)
{
    if (++count >= capacity)
        reserve(capacity == 0 ? 128 : capacity * 2);

    front = (front == 0 ? capacity : front) - 1;
    new (vals + front) T{std::move(val)};
}

template<typename T>
void Queue<T>::pushBack(const T& val)
{
    if (++count >= capacity)
        reserve(capacity == 0 ? 128 : capacity * 2);

    new (vals + back) T{val};
    back = (back + 1) % capacity;
}

template<typename T>
void Queue<T>::pushBack(T&& val)
{
    if (++count >= capacity)
        reserve(capacity == 0 ? 128 : capacity * 2);

    new (vals + back) T{std::move(val)};
    back = (back + 1) % capacity;
}

template<typename T>
void QueueTemp<T>::pushBack(const T& val)
{
    if (++count >= capacity)
        reserve(capacity == 0 ? 128 : capacity * 2);

    new (vals + back) T{val};
    back = (back + 1) % capacity;
}

template<typename T>
void QueueTemp<T>::pushBack(T&& val)
{
    if (++count >= capacity)
        reserve(capacity == 0 ? 128 : capacity * 2);

    new (vals + back) T{std::move(val)};
    back = (back + 1) % capacity;
}

template<typename T>
T Queue<T>::popFront()
{
    HG_ASSERT(count > 0);
    --count;

    T ret = std::move(vals[front]);
    vals[front].~T();
    front = (front + 1) % capacity;
    return ret;
}

template<typename T>
T Queue<T>::popBack()
{
    HG_ASSERT(count > 0);
    --count;

    back = (back == 0 ? capacity : back) - 1;
    T ret = std::move(vals[back]);
    vals[back].~T();
    return ret;
}

template<typename T>
QueueTemp<T>::QueueTemp(Arena* arenaVal, u64 capacityVal)
    : arena{arenaVal}
    , vals{arenaVal->alloc<T>(capacityVal)}
    , front{0}
    , back{0}
    , count{0}
    , capacity{capacityVal}
{}

template<typename T>
QueueTemp<T>::~QueueTemp() noexcept
{
    if (back != front)
        HG_WARN("Non-empty queue destroyed\n");

    for (u64 i = front; i != back; i = (i + 1) % capacity)
    {
        vals[i].~T();
    }
}

template<typename T>
void QueueTemp<T>::reserve(u64 newCapacity)
{
    HG_ASSERT(arena != nullptr);

    if (newCapacity > capacity)
    {
        if (arena->extend(vals, capacity, newCapacity))
        {
            if (front > back)
            {
                for (u64 i = 0; i < front - back; ++i)
                {
                    new (vals + capacity + i) T{std::move(vals + back + i)};
                    vals[back + i].~T();
                }
            }
            back += capacity;
        }
        else
        {
            T* newVals = arena->alloc<T>(newCapacity);

            T* nextVal = newVals;
            for (u64 i = front; i != back; i = (i + 1) % capacity)
            {
                new (nextVal++) T{std::move(vals[i])};
                vals[i].~T();
            }

            vals = newVals;
            front = 0;
            back = count;
        }
        capacity = newCapacity;
    }
}



template<typename T>
T QueueTemp<T>::popFront()
{
    HG_ASSERT(count > 0);
    --count;

    T ret = std::move(vals[front]);
    vals[front].~T();
    front = (front + 1) % capacity;
    return ret;
}

template<typename T>
T QueueTemp<T>::popBack()
{
    HG_ASSERT(count > 0);
    --count;

    back = (back == 0 ? capacity : back) - 1;
    T ret = std::move(vals[back]);
    vals[back].~T();
    return ret;
}

template<typename V>
Set<V>::Set(u64 initCapacity)
    : hasVal{heapAlloc<bool>(initCapacity)}
    , vals{heapAlloc<V>(initCapacity)}
    , capacity{initCapacity}
    , count{0}
{
    memset(hasVal, 0, capacity);
}

template<typename V>
Set<V>::~Set() noexcept
{
    forEach([&](V* val)
    {
        val->~V();
    });
    heapFree(hasVal, capacity);
    heapFree(vals, capacity);
}

template<typename V>
void Set<V>::resize(u64 newSize)
{
    HG_ASSERT(newSize > count);
    if (newSize == capacity)
        return;

    Set<V> newSet{newSize};

    for (u64 i = 0; i < capacity; ++i)
    {
        if (hasVal[i])
            newSet.add(std::move(vals[i]));
    }

    *this = std::move(newSet);
}

template<typename V>
void Set<V>::reset()
{
    for (u64 i = 0; i < capacity; ++i)
    {
        if (hasVal[i])
        {
            vals[i].~V();
            hasVal[i] = false;
        }
    }
    count = 0;
}

template<typename V>
void Set<V>::add(const V& val)
{
    V v = val;
    if (count >= capacity / 2)
        resize(capacity == 0 ? 128 : capacity * 2);

    u64 idx = static_cast<u64>(hash(v) % capacity);
    for (u64 dist = 0; hasVal[idx] && !(vals[idx] == v); ++dist)
    {
        u64 otherDist = static_cast<u64>(hash(vals[idx]) % capacity) - idx;
        if (otherDist > capacity)
            otherDist += capacity;

        if (otherDist < dist)
        {
            std::swap(v, vals[idx]);
            dist = otherDist;
        }

        idx = (idx + 1) % capacity;
    }

    new (vals + idx) V{std::move(v)};
    hasVal[idx] = true;
    ++count;
}

template<typename V>
void Set<V>::add(V&& val)
{
    if (count >= capacity / 2)
        resize(capacity == 0 ? 128 : capacity * 2);

    u64 idx = static_cast<u64>(hash(val) % capacity);
    for (u64 dist = 0; hasVal[idx] && !(vals[idx] == val); ++dist)
    {
        u64 otherDist = static_cast<u64>(hash(vals[idx]) % capacity) - idx;
        if (otherDist > capacity)
            otherDist += capacity;

        if (otherDist < dist)
        {
            std::swap(val, vals[idx]);
            dist = otherDist;
        }

        idx = (idx + 1) % capacity;
    }

    new (vals + idx) V{std::move(val)};
    hasVal[idx] = true;
    ++count;
}

template<typename V>
void Set<V>::remove(const V& val)
{
    u64 idx = static_cast<u64>(hash(val) % capacity);
    while (hasVal[idx])
    {
        if (vals[idx] == val)
            break;
        idx = (idx + 1) % capacity;
    }
    if (!hasVal[idx])
        return;

    u64 next = (idx + 1) % capacity;
    while (hasVal[next])
    {
        if (hash(vals[next]) % capacity != next)
        {
            vals[idx] = std::move(vals[next]);
            idx = next;
        }
        next = (next + 1) % capacity;
    }

    vals[idx].~V();
    hasVal[idx] = false;
    --count;
}

template<typename V>
bool Set<V>::has(const V& val)
{
    for (u64 idx = static_cast<u64>(hash(val) % capacity); hasVal[idx]; idx = (idx + 1) % capacity)
    {
        if (vals[idx] == val)
            return true;
    }
    return false;
}

template<typename V>
template<typename F> requires std::is_invocable_r_v<void, F, V*>
void Set<V>::forEach(F fn)
{
    for (u64 i = 0; i < capacity; ++i)
    {
        if (hasVal[i])
            fn(vals + i);
    }
}

template<typename V>
SetTemp<V>::SetTemp(Arena* arenaVal, u64 initCapacity)
    : arena{arenaVal}
    , hasVal{arenaVal->alloc<bool>(initCapacity)}
    , vals{arenaVal->alloc<V>(initCapacity)}
    , capacity{initCapacity}
    , count{0}
{
    reset();
}

template<typename V>
SetTemp<V>::~SetTemp() noexcept
{
    forEach([&](V* val)
    {
        val->~V();
    });
}

template<typename V>
void SetTemp<V>::resize(u64 newSize)
{
    HG_ASSERT(newSize > count);
    if (newSize == capacity)
        return;

    SetTemp<V> newSet{newSize};

    for (u64 i = 0; i < capacity; ++i)
    {
        if (hasVal[i])
            newSet.add(std::move(vals[i]));
    }

    *this = std::move(newSet);
}

template<typename V>
void SetTemp<V>::reset()
{
    for (u64 i = 0; i < capacity; ++i)
    {
        if (hasVal[i])
        {
            vals[i].~V();
            hasVal[i] = false;
        }
    }
    count = 0;
}

template<typename V>
void SetTemp<V>::add(const V& val)
{
    V v = val;
    if (count >= capacity / 2)
        resize(capacity == 0 ? 128 : capacity * 2);

    u64 idx = static_cast<u64>(hash(v) % capacity);
    for (u64 dist = 0; hasVal[idx] && !(vals[idx] == v); ++dist)
    {
        u64 otherDist = static_cast<u64>(hash(vals[idx]) % capacity) - idx;
        if (otherDist > capacity)
            otherDist += capacity;

        if (otherDist < dist)
        {
            std::swap(v, vals[idx]);
            dist = otherDist;
        }

        idx = (idx + 1) % capacity;
    }

    new (vals + idx) V{std::move(v)};
    hasVal[idx] = true;
    ++count;
}

template<typename V>
void SetTemp<V>::add(V&& val)
{
    if (count >= capacity / 2)
        resize(capacity == 0 ? 128 : capacity * 2);

    u64 idx = static_cast<u64>(hash(val) % capacity);
    for (u64 dist = 0; hasVal[idx] && !(vals[idx] == val); ++dist)
    {
        u64 otherDist = static_cast<u64>(hash(vals[idx]) % capacity) - idx;
        if (otherDist > capacity)
            otherDist += capacity;

        if (otherDist < dist)
        {
            std::swap(val, vals[idx]);
            dist = otherDist;
        }

        idx = (idx + 1) % capacity;
    }

    new (vals + idx) V{std::move(val)};
    hasVal[idx] = true;
    ++count;
}

template<typename V>
void SetTemp<V>::remove(const V& val)
{
    u64 idx = static_cast<u64>(hash(val) % capacity);
    while (hasVal[idx])
    {
        if (vals[idx] == val)
            break;
        idx = (idx + 1) % capacity;
    }
    if (!hasVal[idx])
        return;

    u64 next = (idx + 1) % capacity;
    while (hasVal[next])
    {
        if (hash(vals[next]) % capacity != next)
        {
            vals[idx] = std::move(vals[next]);
            idx = next;
        }
        next = (next + 1) % capacity;
    }
    vals[idx].~V();
    hasVal[idx] = false;
    --count;
}

template<typename V>
bool SetTemp<V>::has(const V& val)
{
    for (u64 idx = static_cast<u64>(hash(val) % capacity); hasVal[idx]; idx = (idx + 1) % capacity)
    {
        if (vals[idx] == val)
            return true;
    }
    return false;
}

template<typename V>
template<typename F> requires std::is_invocable_r_v<void, F, V*>
void SetTemp<V>::forEach(F fn)
{
    for (u64 i = 0; i < capacity; ++i)
    {
        if (hasVal[i])
            fn(vals + i);
    }
}

template<typename K, typename V>
Map<K, V>::Map(u64 initCapacity)
    : hasVal{heapAlloc<bool>(initCapacity)}
    , keys{heapAlloc<K>(initCapacity)}
    , vals{heapAlloc<V>(initCapacity)}
    , capacity{initCapacity}
    , count{0}
{
    memset(hasVal, 0, capacity);
}

template<typename K, typename V>
Map<K, V>::~Map() noexcept
{
    forEach([&](K* key, V* val)
    {
        key->~K();
        val->~V();
    });
    heapFree(hasVal, capacity);
    heapFree(keys, capacity);
    heapFree(vals, capacity);
}

template<typename K, typename V>
void Map<K, V>::resize(u64 newSize)
{
    HG_ASSERT(newSize > count);
    if (newSize == capacity)
        return;

    Map<K, V> newMap{newSize};

    for (u64 i = 0; i < capacity; ++i)
    {
        if (hasVal[i])
            newMap.add(std::move(keys[i]), std::move(vals[i]));
    }

    *this = std::move(newMap);
}

template<typename K, typename V>
void Map<K, V>::reset()
{
    for (u64 i = 0; i < capacity; ++i)
    {
        if (hasVal[i])
        {
            keys[i].~K();
            vals[i].~V();
            hasVal[i] = false;
        }
    }
    count = 0;
}

template<typename K, typename V>
V* Map<K, V>::add(const K& key, const V& val)
{
    K k = key;
    V v = val;
    if (count >= capacity / 2)
        resize(capacity == 0 ? 128 : capacity * 2);

    u64 idx = static_cast<u64>(hash(k) % capacity);
    for (u64 dist = 0; hasVal[idx] && !(keys[idx] == k); ++dist)
    {
        u64 otherDist = static_cast<u64>(hash(keys[idx]) % capacity) - idx;
        if (otherDist > capacity)
            otherDist += capacity;

        if (otherDist < dist)
        {
            std::swap(k, keys[idx]);
            std::swap(v, vals[idx]);
            dist = otherDist;
        }

        idx = (idx + 1) % capacity;
    }

    hasVal[idx] = true;
    new (keys + idx) K{std::move(k)};
    new (vals + idx) V{std::move(v)};
    ++count;

    return vals + idx;
}

template<typename K, typename V>
V* Map<K, V>::add(K&& key, V&& val)
{
    if (count >= capacity / 2)
        resize(capacity == 0 ? 128 : capacity * 2);

    u64 idx = static_cast<u64>(hash(key) % capacity);
    for (u64 dist = 0; hasVal[idx] && !(keys[idx] == key); ++dist)
    {
        u64 otherDist = static_cast<u64>(hash(keys[idx]) % capacity) - idx;
        if (otherDist > capacity)
            otherDist += capacity;

        if (otherDist < dist)
        {
            std::swap(key, keys[idx]);
            std::swap(val, vals[idx]);
            dist = otherDist;
        }

        idx = (idx + 1) % capacity;
    }

    hasVal[idx] = true;
    new (keys + idx) K{std::move(key)};
    new (vals + idx) V{std::move(val)};
    ++count;

    return vals + idx;
}

template<typename K, typename V>
bool Map<K, V>::remove(const K& key, V* val)
{
    u64 idx = static_cast<u64>(hash(key) % capacity);
    while (hasVal[idx])
    {
        if (keys[idx] == key)
            break;
        idx = (idx + 1) % capacity;
    }
    if (!hasVal[idx])
        return false;

    if (val != nullptr)
        *val = std::move(vals[idx]);

    u64 next = (idx + 1) % capacity;
    while (hasVal[next])
    {
        if (hash(keys[next]) % capacity != next)
        {
            keys[idx] = std::move(keys[next]);
            vals[idx] = std::move(vals[next]);
            idx = next;
        }
        next = (next + 1)  % capacity;
    }

    keys[idx].~K();
    vals[idx].~V();
    hasVal[idx] = false;
    --count;

    return true;
}

template<typename K, typename V>
V* Map<K, V>::get(const K& key)
{
    for (u64 idx = static_cast<u64>(hash(key) % capacity); hasVal[idx]; idx = (idx + 1) % capacity)
    {
        if (keys[idx] == key)
            return vals + idx;
    }
    return nullptr;
}

template<typename K, typename V>
template<typename F> requires std::is_invocable_r_v<void, F, K*, V*>
void Map<K, V>::forEach(F fn)
{
    for (u64 i = 0; i < capacity; ++i)
    {
        if (hasVal[i])
            fn(&keys[i], &vals[i]);
    }
}

template<typename K, typename V>
MapTemp<K, V>::MapTemp(Arena* arenaVal, u64 initCapacity)
    : arena{arenaVal}
    , hasVal{arenaVal->alloc<bool>(initCapacity)}
    , keys{arenaVal->alloc<K>(initCapacity)}
    , vals{arenaVal->alloc<V>(initCapacity)}
    , capacity{initCapacity}
    , count{0}
{
    memset(hasVal, 0, capacity);
}

template<typename K, typename V>
MapTemp<K, V>::~MapTemp() noexcept
{
    forEach([&](K* key, V* val)
    {
        key->~K();
        val->~V();
    });
}

template<typename K, typename V>
void MapTemp<K, V>::resize(u64 newSize)
{
    HG_ASSERT(newSize > count);
    if (newSize == capacity)
        return;

    MapTemp<K, V> newMapTemp{newSize};

    for (u64 i = 0; i < capacity; ++i)
    {
        if (hasVal[i])
            newMapTemp.add(std::move(keys[i]), std::move(vals[i]));
    }

    *this = std::move(newMapTemp);
}

template<typename K, typename V>
void MapTemp<K, V>::reset()
{
    for (u64 i = 0; i < capacity; ++i)
    {
        if (hasVal[i])
        {
            keys[i].~K();
            vals[i].~V();
            hasVal[i] = false;
        }
    }
    count = 0;
}

template<typename K, typename V>
V* MapTemp<K, V>::add(const K& key, const V& val)
{
    K k = key;
    V v = val;
    if (count >= capacity / 2)
        resize(capacity == 0 ? 128 : capacity * 2);

    u64 idx = static_cast<u64>(hash(k) % capacity);
    for (u64 dist = 0; hasVal[idx] && !(keys[idx] == k); ++dist)
    {
        u64 otherDist = static_cast<u64>(hash(keys[idx]) % capacity) - idx;
        if (otherDist > capacity)
            otherDist += capacity;

        if (otherDist < dist)
        {
            std::swap(k, keys[idx]);
            std::swap(v, vals[idx]);
            dist = otherDist;
        }

        idx = (idx + 1) % capacity;
    }

    hasVal[idx] = true;
    new (keys + idx) K{std::move(k)};
    new (vals + idx) V{std::move(v)};
    ++count;

    return vals + idx;
}

template<typename K, typename V>
V* MapTemp<K, V>::add(K&& key, V&& val)
{
    if (count >= capacity / 2)
        resize(capacity == 0 ? 128 : capacity * 2);

    u64 idx = static_cast<u64>(hash(key) % capacity);
    for (u64 dist = 0; hasVal[idx] && !(keys[idx] == key); ++dist)
    {
        u64 otherDist = static_cast<u64>(hash(keys[idx]) % capacity) - idx;
        if (otherDist > capacity)
            otherDist += capacity;

        if (otherDist < dist)
        {
            std::swap(key, keys[idx]);
            std::swap(val, vals[idx]);
            dist = otherDist;
        }

        idx = (idx + 1) % capacity;
    }

    hasVal[idx] = true;
    new (keys + idx) K{std::move(key)};
    new (vals + idx) V{std::move(val)};
    ++count;

    return vals + idx;
}

template<typename K, typename V>
bool MapTemp<K, V>::remove(const K& key, V* val)
{
    u64 idx = static_cast<u64>(hash(key) % capacity);
    while (hasVal[idx])
    {
        if (keys[idx] == key)
            break;
        idx = (idx + 1) % capacity;
    }
    if (!hasVal[idx])
        return false;

    if (val != nullptr)
        *val = std::move(vals[idx]);

    u64 next = (idx + 1) % capacity;
    while (hasVal[next])
    {
        if (hash(keys[next]) % capacity != next)
        {
            keys[idx] = std::move(keys[next]);
            vals[idx] = std::move(vals[next]);
            idx = next;
        }
        next = (next + 1)  % capacity;
    }

    keys[idx].~K();
    vals[idx].~V();
    hasVal[idx] = false;
    --count;

    return true;
}

template<typename K, typename V>
V* MapTemp<K, V>::get(const K& key)
{
    for (u64 idx = static_cast<u64>(hash(key) % capacity); hasVal[idx]; idx = (idx + 1) % capacity)
    {
        if (keys[idx] == key)
            return vals + idx;
    }
    return nullptr;
}

template<typename K, typename V>
template<typename F> requires std::is_invocable_r_v<void, F, K*, V*>
void MapTemp<K, V>::forEach(F fn)
{
    for (u64 i = 0; i < capacity; ++i)
    {
        if (hasVal[i])
            fn(&keys[i], &vals[i]);
    }
}

template<typename T>
Pool<T>::~Pool() noexcept
{
#ifdef HG_DEBUG_MODE
    if (active.count > 0)
        HG_WARN("Memory leak, pool destroyed with active objects\n");
    for (u32 i = 0; i < active.count; ++i)
    {
        active[i]->~T();
    }
#endif

    for (u32 i = 0; i < prealloc.count; ++i)
    {
        heapFree(prealloc[i], blockCount);
    }
}

template<typename T>
template<typename... Args>
T* Pool<T>::alloc(Args&&... args)
{
    if (inactive.count == 0)
    {
        T* block = heapAlloc<T>(blockCount);
        for (u32 i = 0; i < blockCount; ++i)
        {
            inactive.push(block + i);
        }
        prealloc.push(block);
    }

    T* object = new (inactive.pop()) T{std::forward<Args>(args)...};
#ifdef HG_DEBUG_MODE
    active.push(object);
#endif
    return object;
}

template<typename T>
void Pool<T>::free(T* object)
{
    if (object == nullptr)
        return;

    object->~T();
#ifdef HG_DEBUG_MODE
    for (u32 i = 0; i < active.count; ++i)
    {
        if (active[i] == object)
        {
            active.removeSwap(i);
            goto found;
        }
    }
    HG_WARN("Invalid attempt to free to pool, object not in pool, possible double free\n");
    return;
found:
#endif
    inactive.push(object);
}

template<typename T>
void assetLoadImpl(AssetData<T>* data)
{
    static_cast<void>(data);
    static_assert(false, "Asset type cannot be loaded without template specialization");
}

template<typename T>
Asset<T>::Asset(AssetData<T>* dataVal)
    : data{dataVal}
{
    if (data != nullptr)
        ++data->refCount;
}

template<typename T>
Asset<T>::~Asset() noexcept
{
    if (data != nullptr && --data->refCount == 0)
    {
        if (data->path != "")
            assets<T>.map.remove(data->path);

        assets<T>.pool.free(data);
    }
}

template<typename T>
Asset<T> newAsset()
{
    return assets<T>.pool.alloc();
}

template<typename T>
Asset<T> load(StringView path)
{
    AssetData<T>** asset = assets<T>.map.get(path);
    if (asset != nullptr)
        return *asset;

    AssetData<T>* data = assets<T>.pool.alloc();

    data->path = String::create(path);
    assets<T>.map.add(data->path, data);

    assetLoadImpl(data);
    return data;
}

template<typename T>
void reload(const Asset<T>& asset)
{
    if (asset.data != nullptr)
    {
        *asset = {};
        assetLoadImpl(asset.data);
    }
}

template<typename T>
void serialize(Serializer* s, T* val)
{
    serializeVoid(s, {val, sizeof(*val)});
}

template<typename... Ts>
void serializeObject(Serializer* s, Ts*... vals)
{
    serializeBegin(s);
    (serialize(s, vals), ...);
    serializeEnd(s);
}

template<typename T, u64 N>
void serialize(Serializer* s, T (*arr)[N])
{
    serializeBegin(s);
    for (u64 i = 0; i < N; ++i)
    {
        serialize(s, &(*arr)[i]);
    }
    serializeEnd(s);
}

template<std::integral T>
void serialize(Serializer* s, T* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->data = static_cast<i64>(*val);
    }
    else
    {
        HG_ASSERT(s->current->data.is<i64>());
        *val = static_cast<T>(s->current->data.get<i64>());
    }
}

template<std::floating_point T>
void serialize(Serializer* s, T* val)
{
    serializeNodeStart(s);

    if (s->writing)
    {
        s->current->data = f64{*val};
    }
    else
    {
        HG_ASSERT(s->current->data.is<f64>());
        *val = static_cast<T>(s->current->data.get<f64>());
    }
}

template<typename... Ts>
void serialize(Serializer* s, Product<Ts...>* product)
{
    if constexpr (sizeof...(Ts) > 0)
    {
        [&]<u64... Is>(std::index_sequence<Is...>)
        {
            serializeObject(s, &product->template get<Is>()...);
        }(std::index_sequence_for<Ts...>{});
    }
}

template<typename... Ts>
void serialize(Serializer* s, Sum<Ts...>* sum)
{
    serializeBegin(s);
    u32 tag = sum->tag;
    serialize(s, &tag);
    if (tag < sum->count)
    {
        if (!s->writing && tag != sum->tag)
        {
            sum->tag = tag;
            sum->call([&](auto& val) { new (&val) std::remove_cvref_t<decltype(val)>{}; });
        }
        sum->call([&](auto& val) { serialize(s, &val); });
    }
    else
    {
        if (!s->writing)
            *sum = {};
    }
    serializeEnd(s);
}

template<typename T>
void serialize(Serializer* s, Maybe<T>* maybe)
{
    serializeBegin(s);
    bool has = maybe->has;
    serialize(s, &has);
    if (has)
    {
        if (!s->writing && !maybe->has)
            *maybe = some<T>();
        serialize(s, &maybe->val);
    }
    else
    {
        if (!s->writing)
            *maybe = {};
    }
    serializeEnd(s);
}

template<typename T>
void serialize(Serializer* s, UniquePtr<T>* ptr)
{
    serializeBegin(s);
    bool has = *ptr != nullptr;
    serialize(s, &has);
    if (has)
    {
        if (!s->writing)
            *ptr = makeUnique<T>();
        serialize(s, ptr->ptr);
    }
    serializeEnd(s);
}

template<typename T>
void serialize(Serializer* s, Array<T>* arr)
{
    serializeBegin(s);
    if (s->writing)
    {
        serialize(s, &arr->count);
        serialize(s, &arr->capacity);
    }
    else
    {
        u32 count;
        u32 capacity;
        serialize(s, &count);
        serialize(s, &capacity);
        *arr = Array<T>{count, capacity};
    }
    for (u32 i = 0; i < arr->count; ++i)
    {
        serialize(s, arr->vals + i);
    }
    serializeEnd(s);
}

template<typename V>
void serialize(Serializer* s, Set<V>* set)
{
    serializeBegin(s);

    if (s->writing)
    {
        serialize(s, &set->capacity);
        serialize(s, &set->count);

        set->forEach([&](V* val)
        {
            serialize(s, val);
        });
    }
    else
    {
        u32 capacity;
        u32 count;
        serialize(s, &capacity);
        serialize(s, &count);

        *set = Set<V>{capacity};
        for (u32 i = 0; i < count; ++i)
        {
            V val;
            serialize(s, &val);
            set->add(val);
        }
    }

    serializeEnd(s);
}

template<typename K, typename V>
void serialize(Serializer* s, Map<K, V>* map)
{
    serializeBegin(s);

    if (s->writing)
    {
        serialize(s, &map->capacity);
        serialize(s, &map->count);

        map->forEach([&](K* key, V* val)
        {
            serializeObject(s, key, val);
        });
    }
    else
    {
        u32 capacity;
        u32 count;
        serialize(s, &capacity);
        serialize(s, &count);

        *map = Map<K, V>{capacity};
        for (u32 i = 0; i < count; ++i)
        {
            K key;
            V val;
            serializeObject(s, &key, &val);
            map->add(std::move(key), std::move(val));
        }
    }

    serializeEnd(s);
}

template<typename T>
void serialize(Serializer* s, Asset<T>* asset)
{
    if (s->writing)
    {
        serialize(s, &asset->data->path);
    }
    else
    {
        String path;
        serialize(s, &path);
        if (path != "")
            *asset = load<T>(path);
        else
            *asset = {};
    }
}

// template<typename T, typename Fn> requires std::is_invocable_r_v<void, Fn, Entity, T*>
// void ecsForEachSingle(Ecs* ecs, Fn& fn)
// {
//     Entity* e = ecsEntities<T>(ecs);
//     Entity* end = e + ecsCount<T>(ecs);
//     T* c = ecsComponents<T>(ecs);
//     for (; e != end; ++e, ++c)
//     {
//         fn(*e, c);
//     }
// }
//
// template<typename... Ts, typename Fn> requires std::is_invocable_r_v<void, Fn, Entity, Ts*...>
// void ecsForEachMulti(Ecs* ecs, Fn& fn)
// {
//     u64 id = ecsFindSmallest<Ts...>(ecs);
//     Component* system = ecs->components.get(id);
//     HG_ASSERT(system != nullptr);
//
//     Entity* e = system->entities.vals + 1;
//     Entity* end = e + system->entities.count - 1;
//     for (; e != end; ++e)
//     {
//         if (ecsHasAll<Ts...>(ecs, *e))
//             fn(*e, ecsGet<Ts>(ecs, *e)...);
//     }
// }
//
// template<typename... Ts, typename Fn> requires (sizeof...(Ts) != 0) && std::is_invocable_r_v<void, Fn, Entity, Ts*...>
// void ecsForEach(Ecs* ecs, Fn fn)
// {
//     if constexpr (sizeof...(Ts) == 1)
//     {
//         ecsForEachSingle<Ts...>(ecs, fn);
//     } else {
//         ecsForEachMulti<Ts...>(ecs, fn);
//     }
// }
//
// template<typename T, typename Fn> requires std::is_invocable_r_v<void, Fn, Entity, T*>
// void ecsForParSingle(Ecs* ecs, Fn& fn)
// {
//     struct Capture {
//         Ecs* ecs;
//         Fn* fn;
//     };
//     Capture capture{ecs, &fn};
//
//     forPar(0, ecsCount<T>(ecs), &capture, [](void* pcapture, u64 idx)
//     {
//         Capture* capture = static_cast<Capture*>(pcapture);
//         (*capture->fn)(
//             ecsEntities<T>(capture->ecs)[idx],
//             &ecsComponents<T>(capture->ecs)[idx]);
//     });
// }
//
// template<typename... Ts, typename Fn> requires std::is_invocable_r_v<void, Fn, Entity, Ts*...>
// void ecsForParMulti(Ecs* ecs, Fn& fn)
// {
//     Component* system = ecs->components.get(ecsFindSmallest<Ts...>(ecs));
//     HG_ASSERT(system != nullptr);
//
//     struct Capture {
//         Ecs* ecs;
//         Component* system;
//         Fn* fn;
//     };
//     Capture capture{ecs, system, &fn};
//
//     forPar(1, system->entities.count, &capture, [](void* pcapture, u64 idx)
//     {
//         Capture* capture = static_cast<Capture*>(pcapture);
//         Entity e = capture->system->entities[idx];
//         if (ecsHasAll<Ts...>(capture->ecs, e))
//             (*capture->fn)(e, ecsGet<Ts>(capture->ecs, e)...);
//     });
// }
//
// template<typename... Ts, typename Fn> requires (sizeof...(Ts) > 0) && std::is_invocable_r_v<void, Fn, Entity, Ts*...>
// void ecsForPar(Ecs* ecs, Fn fn)
// {
//     if constexpr (sizeof...(Ts) == 1)
//     {
//         ecsForParSingle<Ts...>(ecs, fn);
//     } else {
//         ecsForParMulti<Ts...>(ecs, fn);
//     }
// }

