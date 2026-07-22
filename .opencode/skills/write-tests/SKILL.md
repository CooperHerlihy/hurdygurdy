---
name: write-tests
description: Add/modify tests in src/test.cpp. Monolithic file, TEST() macro, scoped blocks, edge-case coverage.
---

Test like TDD: Write tests for the ideal behavior, not the current buggy behavior. Failing tests is good. Templates may not even compile.

Plan out all use cases, and edge cases in advance. Finish with end-to-end integration tests.

Tests are documentation. Comment every scenario and edge case.

Write tests as scoped blocks matching header declaration order.

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

