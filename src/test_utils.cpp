#include "hurdygurdy.hpp"

struct HgTestArray {
    HgTest* items;
    usize capacity;
    usize count;

    static HgTestArray create(usize init_count) {
        HgTestArray arr;
        arr.items = (HgTest*)std::malloc(init_count);
        arr.capacity = init_count;
        arr.count = 0;
        return arr;
    }

    void destroy() const {
        std::free(items);
    }

    HgTest& push() {
        if (capacity == count) {
            usize new_capacity = capacity == 0 ? 1 : capacity * 2;
            items = (HgTest*)std::realloc(items, new_capacity);
            capacity = new_capacity;
        }
        return items[count++];
    }
};

static HgTestArray& hg_internal_get_tests() {
    static HgTestArray tests = tests.create(1024);
    return tests;
}

HgTest::HgTest(const char* test_name, bool (*test_function)()) : name(test_name), function(test_function) {
    hg_internal_get_tests().push() = *this;
}

bool hg_run_tests() {
    std::printf("HurdyGurdy: Tests Begun\n");

    HgTestArray& tests = hg_internal_get_tests();
    bool all_succeeded = true;

    HgClock timer{};
    for (usize i = 0; i < tests.count; ++i) {
        std::printf("%s...\n", tests.items[i].name);
        if (tests.items[i].function()) {
            std::printf("\x1b[32mSuccess\n\x1b[0m");
        } else {
            all_succeeded = false;
            std::printf("\x1b[31mFailure\n\x1b[0m");
        }
    }
    f64 ms = timer.tick() * 1000.0f;

    if (all_succeeded) {
        std::printf("HurdyGurdy: Tests Complete in %fms \x1b[32m[Success]\x1b[0m\n", ms);
    } else {
        std::printf("HurdyGurdy: Tests Complete in %fms \x1b[31m[Failure]\x1b[0m\n", ms);
    }

    return all_succeeded;
}

