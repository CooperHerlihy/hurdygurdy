---
name: write-tests
description: Add/modify tests in src/test.cpp. Monolithic file, TEST() macro, scoped blocks, edge-case coverage.
---

**Test the header, not the implementation**: Write tests against the *correct* behavior, not against any specific buggy behavior you happen to observe. If the implementation does anything unexpected, it should be documented or fixed.

Tests are **living documentation**. Comment every scenario and edge case — the comments are as important as the assertions.

Write tests as scoped blocks matching header declaration order. Cover every function, parameter boundary, error path, and edge case.

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
- **Memory**: `ArenaScope scratch = getScratch();`
- **Floats:** `TEST(std::abs(a - b) <= FLT_EPSILON)` or `vecEq2/3/4`.
- **Lifetimes:** Use Lifecycle type defined at top of test.cpp

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

