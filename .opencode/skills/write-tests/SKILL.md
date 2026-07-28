# Write / Update Tests

Use this skill when adding new tests or modifying existing tests in `src/test/`.
Always match the established format and conventions.

## Test Harness

- `#include "tests.hpp"` provides the `TEST()` macro and `Lifecycle` struct.
- `TEST(cond)` panics on failure with file/line info.
- `Lifecycle` tracks ctor/copy/move/dtor counts via `Lifecycle::stats`.
- Always call `Lifecycle::stats.reset()` before a block that tests lifecycle.

## File Structure

Every test file lives in `src/test/xxx.cpp` and follows this pattern:

```cpp
#include "tests.hpp"

void testXxx()
{
    // ============================================================================
    // SectionName
    // ============================================================================
    //
    // One-line description of the type or API under test.

    // ------------------------------------------------------------------
    // SubsectionName
    // ------------------------------------------------------------------

    // Specific test case description
    {
        TEST(...);
    }

    // Another test case
    {
        TEST(...);
    }
}
```

## Registration

1. Add the function declaration to `src/test/tests.hpp`:
   ```cpp
   void testXxx();
   ```
2. Add the call to `src/test/tests.cpp` in `main()`, in alphabetical order.

## Comment Conventions

### Section banners

- Top-level: `// ============================================================================` (75 dashes)
- Subsection: `// ------------------------------------------------------------------` (66 dashes)
- Both lines have exactly one space after `//` before the dash.

### Test case comments

- Single `//` line, capitalized, no trailing period.
- Be concise -- describe what the test verifies, not how.
- One blank line before each comment. No blank line between the comment and its opening `{`.

### Description paragraphs

- After a section banner, a `//` paragraph can describe the type or API.
- Each line is `// ` followed by text. Hard-wrap at roughly 80 chars.

### Inline comments

- Use sparingly. Only for non-obvious assertions, e.g.:
  ```cpp
  TEST(!a.has); // moved-from is empty
  ```

## Test Patterns

### Basic value tests

```cpp
    // Default-constructed set is empty
    {
        Set<u32> set;
        TEST(set.hasVal == nullptr);
        TEST(set.capacity == 0);
        TEST(set.count == 0);
    }
```

### Lifecycle tracking

```cpp
    // push default-constructs in place (0 copies, 0 moves)
    {
        Lifecycle::stats.reset();
        {
            Array<Lifecycle> arr;
            arr.push();
            TEST(arr.count == 1);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.copies == 0);
        TEST(Lifecycle::stats.moves == 0);
    }
```

### Move semantics

```cpp
    // Move construct transfers ownership
    {
        Array<u32> a;
        a.push(1);
        u32* oldVals = a.vals;
        Array<u32> b = std::move(a);
        TEST(a.vals == nullptr);
        TEST(b.vals == oldVals);
    }
```

### Edge cases

```cpp
    // Remove non-existent element is safe
    {
        Set<u32> set;
        set.add(5);
        set.remove(99);
        TEST(set.count == 1);
    }
```

### TODO stubs

```cpp
void testXxx()
{
    HG_WARN("TODO: testXxx()\n");
}
```

## What NOT to do

- No `int`/`size_t`/`std::` containers. Use `u32`/`u64`/`hg::` types.
- No exceptions. Use `TEST()` and `Maybe<T>`.
- No docstrings or Doxygen comments.
- No inline prose or explanatory paragraphs after the section description.
- No trailing punctuation in test comments.
- No nested test blocks deeper than one level inside a subsection.
- No blank line between a test comment and its opening brace.