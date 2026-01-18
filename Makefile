SRC_DIR := .
BUILD_DIR := $(SRC_DIR)/build
TEST_DIR := $(SRC_DIR)/hg_test_dir

CXX := c++
AR := ar

STD := -std=c++17 -MMD -MP
WARNINGS := -Werror -Wall -Wextra -Wconversion -Wshadow -pedantic

DEBUG_CONFIG := -g -O0 -fsanitize=undefined -fno-exceptions -fno-rtti
RELEASE_CONFIG := -O3 -DNDEBUG -fno-exceptions -fno-rtti
CONFIG := $(DEBUG_CONFIG)

INCLUDES := \
	-I$(BUILD_DIR) \
	-I$(SRC_DIR)/include \
	-I$(SRC_DIR)/vendor/libX11/include

SHADERS := \
	$(SRC_DIR)/src/sprite.vert \
	$(SRC_DIR)/src/sprite.frag

SPV := $(patsubst $(SRC_DIR)/src/%, $(BUILD_DIR)/%.spv, $(SHADERS))
SPV_H := $(addsuffix .h, $(SPV))

.PHONY: all debug release clean

all: $(BUILD_DIR)/test $(TEST_DIR)

debug:
	$(MAKE) CONFIG="$(DEBUG_CONFIG)"

release:
	$(MAKE) CONFIG="$(RELEASE_CONFIG)"

$(BUILD_DIR):
	mkdir -p $@

$(TEST_DIR):
	mkdir -p $@

$(BUILD_DIR)/embed_file: $(SRC_DIR)/src/embed_file.cpp | $(BUILD_DIR)
	$(CXX) $(STD) $(CONFIG) $(WARNINGS) $(INCLUDES) $< -o $@

$(BUILD_DIR)/%.spv: $(SRC_DIR)/src/% | $(BUILD_DIR)
	glslc -o $@ $<

$(BUILD_DIR)/%.spv.h: $(BUILD_DIR)/%.spv $(BUILD_DIR)/embed_file
	$(BUILD_DIR)/embed_file $< $(notdir $<) > $@

$(BUILD_DIR)/vk_mem_alloc.o: $(SRC_DIR)/src/vk_mem_alloc.cpp | $(BUILD_DIR)
	$(CXX) $(STD) $(CONFIG) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/stb.o: $(SRC_DIR)/src/stb.c | $(BUILD_DIR)
	$(CXX) $(STD) $(CONFIG) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/hurdygurdy.o: $(SRC_DIR)/src/hurdygurdy.cpp $(SPV_H) | $(BUILD_DIR)
	$(CXX) $(STD) $(CONFIG) $(WARNINGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test.o: $(SRC_DIR)/src/test.cpp | $(BUILD_DIR)
	$(CXX) $(STD) $(CONFIG) $(WARNINGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/libhurdygurdy.a: $(BUILD_DIR)/hurdygurdy.o $(BUILD_DIR)/vk_mem_alloc.o $(BUILD_DIR)/stb.o
	$(AR) rcs $@ $^

$(BUILD_DIR)/test: $(BUILD_DIR)/test.o $(BUILD_DIR)/libhurdygurdy.a
	$(CXX) $(STD) $(CONFIG) $(WARNINGS) -o $@ $(BUILD_DIR)/test.o -L$(BUILD_DIR) -lhurdygurdy

clean:
	rm -rf $(BUILD_DIR) $(TEST_DIR)

-include $(BUILD_DIR)/*.d
