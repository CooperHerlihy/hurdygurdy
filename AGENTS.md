# Hurdy Gurdy

Modern C++20 game development library

## Rules

- never introduce third-party code
- never use exceptions or RTTI
- prefer not to use auto
- prioritize both performance and readability
- prefer modern declarative patterns to legacy imperative patterns
- use `i8`, `u32`, `f64`, etc., not `int`, `float`, etc.
- handle programmer errors with `HG_ASSERT`
- handle external errors with `setError("msg")` and `std::optional`
- handle unrecoverable errors with `panic()`
- prefer a static create function to a failing constructor
- use `HG_DEFER` to defer C-style destroy functions
- use `StringTemp` and `VectorTemp`, or `getScratch()`, for temporary allocations
- use `forPar()` and `callPar()` for concurrency
- use `log()` for temporary log statements
- otherwise, prefer to use the standard library

- when writing tests, use the `writing-tests` skill

## Build

```
debug:
  cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug
  cmake --build debug
  ./build/tests

release:
  cmake -G Ninja -B build/release -DCMAKE_BUILD_TYPE=Release
  cmake --build build/release
  ./build/release/tests

san:
  cmake -G Ninja -B build/san -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
  cmake --build build/san
  LSAN_OPTIONS=detect_leaks=0 ./build/san/tests

tsan:
  cmake -G Ninja -B build/tsan -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer" \
    -DCMAKE_C_FLAGS="-fsanitize=thread -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
  cmake --build build/tsan
  TSAN_OPTIONS=suppressions=/dev/null ./build/tsan/tests

valgrind:
  valgrind --leak-check=full ./build/tests
```

Windows: open a **Visual Studio 2022 x64 developer shell** first, then cmake

