---
name: write-tests
description: Add/modify tests in src/test.cpp. Monolithic file, TEST() macro, scoped blocks, edge-case coverage.
---

Write tests as scoped blocks matching header declaration order. Cover every function, parameter boundary, error path, and edge case.

Tests are **living documentation**. Comment every scenario and edge case — the comments are as important as the assertions.

```cpp
// SubsystemName
{
    // Scenario description
    {
        TEST(cond);
    }
}
```

## Patterns

- **Cleanup**: Destructor or `HG_DEFER(functionCall());`
- **Arena**: `ArenaScope scratch = getScratch();`
- **Concurrency**: `Fence* f = fenceCreate(); HG_DEFER(fenceDestroy(f)); helpThreads(f, timeout);`
- **Floats:** `TEST(std::abs(a - b) <= FLT_EPSILON)` or `vecEq2/3/4`.

## Template

```cpp
// ============================================================================
// TypeName
// ============================================================================
//
// Block comment explaining the type and its role.

{
    // Basic construction and default state
    {
        // ...
    }

    // Normal operation
    {
        // ...
    }

    // Edge case: [describe]
    {
        // ...
    }

    // Error case: [describe]
    {
        // ...
    }
}
```

