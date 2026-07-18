---
name: write-tests
description: >
  Use when the user says "write tests", "add tests", "test this", "cover with tests",
  "test coverage", "write a test for", or asks you to create/modify test code in
  src/test.cpp. This skill guides you in writing tests according to the project's
  conventions: monolithic test file at src/test.cpp, TEST() macro, scoped blocks,
  thorough edge-case coverage with documentation-style comments.
---

# Writing Tests

## File

All tests live in `src/test.cpp`. It is a monolithic file with a single `main()`.

## Structure

Tests are organized as **one scoped block per type/subsystem**. Each block covers every function of that type under every possible use case and edge case. Blocks may be further subdivided with nested scopes for organization.

```
// SubsystemName
{
    // Specific function or scenario
    {
        ...
    }

    // Edge case
    {
        ...
    }
}
```

## Test Macro

```cpp
#define TEST(cond) do { \
    if (!(cond)) \
        HG_PANIC("Test failed in " __FILE__ ":%d %s() \"" #cond "\"\n", __LINE__, __func__); \
} while(0)
```

Use `TEST(cond)` for every assertion.

## Comments as Documentation

Every test should also serve as **living documentation** of how a function is
supposed to be used and what edge cases exist. Use comments liberally:

- A block-level comment at the top of each subsystem section explaining what
  the type/subsystem does
- A comment before each sub-scope describing the scenario or edge case being
  tested (e.g., `// Point exactly at the origin`, `// Negative radius`,
  `// Empty rect`, `// Collision at exact boundary`)
- Comments inline to explain non-obvious values or relationships
- Use the comment style: `// Description of what this tests and why`

Tests should be **long and thorough**. Do not skip edge cases. Every function
parameter boundary, every error path, every corner case should have a test.

## Patterns & Conventions

### Arena usage

```cpp
ArenaScope arena = getScratch();
```

For arena-allocated types. Always use `ArenaScope` (restores arena head on scope
exit) rather than raw `Arena` unless you need manual control.

### RAII / cleanup

```cpp
HG_DEFER(functionCall());
```

Use `HG_DEFER` for cleanup of heap-allocated resources.

### Math comparisons

Use `FLT_EPSILON` for floating-point comparisons:

```cpp
TEST(std::abs(value - expected) <= FLT_EPSILON);
```

Use `vecEq2`, `vecEq3`, `vecEq4` for vector epsilon comparisons.

### Iterating patterns

For testing many values in sequence, enumerate them explicitly rather than using
loops unless the loop is part of what is being tested. The test file uses explicit
sequential TEST() calls to document each case.

For repeated test sequences (e.g., running the same test 3 times to verify
reusability), wrap the repeated block in a `for` loop with a brief comment.

### Concurrency tests

- Use `Fence* fence = fenceCreate()` + `HG_DEFER(fenceDestroy(fence))`
- Use `helpThreads(fence, timeout)` to wait while helping
- Use `forPar(begin, end, data, fn)` for parallel for loops

### StringView comparison

StringView supports `==` and `!=` for comparison. Use directly:

```cpp
TEST(str == "expected");
```

### Test ordering

Organize test blocks in the same order as their declarations appear in
`include/hurdygurdy.hpp`. This makes it easy to cross-reference the header
while reading the test file. For example, if the header declares types/functions
in this order:

1. StringView
2. cString
3. StringBuilder
4. String
5. isWhitespace / isNumeral
6. isInteger / isFloat
7. stringToInteger / stringToFloat
8. integerToString / floatToString

Then the test blocks should follow the same sequence. Within each block,
test basic construction/default state first, then normal operation, then
edge cases, then error cases.

## What NOT to do

- Do not use STL containers (`std::vector`, `std::string`, etc.)
- Do not use exceptions or RTTI
- Do not use `int`, `unsigned`, `size_t` — use `u32`, `i32`, `u64`, etc.
- Do not add new `#include` beyond what is already in the file
- Do not uncomment commented-out blocks unless asked
- Do not reformat existing code

## Complete Template

```cpp
// ============================================================================
// TypeName
// ============================================================================
//
// Block comment explaining the type and its role.
//
// Functions covered:
// - func1: what it does
// - func2: what it does
// ...

{
    // Basic construction and default state
    {
        // ...
    }

    // Normal operation: [describe scenario]
    {
        // ...
    }

    // Edge case: [describe edge case]
    {
        // ...
    }

    // Error case: [describe error case]
    {
        // ...
    }
}
```

## Checklist Before Writing

1. Read the API in `include/hurdygurdy.hpp` (or `src/internal.hpp` for internals)
   to understand every function signature and parameter
2. Identify all edge cases: null/empty inputs, boundary values, max/min values,
   invalid inputs, repeated operations, concurrent access
3. Write tests that would catch a regression in any of these cases
4. Use `cmake --workflow --preset debug` to build and `./build/test` to run
   after writing tests
