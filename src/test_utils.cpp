#include "hurdygurdy.hpp"

namespace {
    struct Test {
        const char* name;
        bool (*function)();
    };

    struct TestArr {
        Test* items;
        usize capacity;
        usize count;

        static TestArr create(usize init_count) {
            TestArr arr;
            arr.items = (Test*)std::malloc(init_count);
            arr.capacity = init_count;
            arr.count = 0;
            return arr;
        }

        void destroy() const {
            std::free(items);
        }

        Test& push() {
            if (capacity == count) {
                usize new_capacity = capacity == 0 ? 1 : capacity * 2;
                items = (Test*)std::realloc(items, new_capacity);
                capacity = new_capacity;
            }
            return items[count++];
        }
    };
}

static TestArr& hg_internal_get_tests() {
    static TestArr tests = tests.create(1024);
    return tests;
}

bool hg_tests_register(const char* name, bool (*function)()) {
    hg_internal_get_tests().push() = {name, function};
    return true;
}

bool hg_tests_run() {
    std::printf("HurdyGurdy: Tests Begun\n");

    TestArr& tests = hg_internal_get_tests();
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

