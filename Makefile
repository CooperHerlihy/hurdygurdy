SRC_DIR := .
BUILD_DIR := $(SRC_DIR)/build
TEST_DIR := $(SRC_DIR)/hg_test_dir

STD := -std=c++17 -MMD -MP
WARNINGS := -Werror -Wall -Wextra -Wconversion -Wshadow -pedantic

DEBUG_CONFIG := -g -O0 -fsanitize=undefined -fno-exceptions -fno-rtti
RELEASE_CONFIG := -O3 -DNDEBUG -fno-exceptions -fno-rtti
CONFIG := $(DEBUG_CONFIG)

INCLUDES := \
	-I$(BUILD_DIR) \
	-I$(SRC_DIR)/include \
	-I$(SRC_DIR)/vendor/libX11/include

SRC := \
	init.cpp \
	test_utils.cpp \
	math.cpp \
	memory.cpp \
	string.cpp \
	time.cpp \
	thread.cpp \
	resources.cpp \
	ecs.cpp \
	pipeline2d.cpp \
	x11.cpp \
	vulkan.cpp

SHADERS := \
	sprite.vert \
	sprite.frag

.PHONY: all debug release clean

all: $(BUILD_DIR)/tests $(TEST_DIR)

debug:
	$(MAKE) CONFIG="$(DEBUG_CONFIG)"

release:
	$(MAKE) CONFIG="$(RELEASE_CONFIG)"

$(BUILD_DIR):
	mkdir -p $@

$(TEST_DIR):
	mkdir -p $@

$(BUILD_DIR)/embed_file: $(SRC_DIR)/src/embed_file.cpp | $(BUILD_DIR)
	c++ $(STD) $(CONFIG) $(WARNINGS) $(INCLUDES) $< -o $@

$(BUILD_DIR)/%.spv: $(SRC_DIR)/src/% | $(BUILD_DIR)
	glslc -o $@ $<

$(BUILD_DIR)/%.spv.h: $(BUILD_DIR)/%.spv $(BUILD_DIR)/embed_file
	$(BUILD_DIR)/embed_file $< $(notdir $<) > $@

$(BUILD_DIR)/vk_mem_alloc.o: $(SRC_DIR)/src/vk_mem_alloc.cpp | $(BUILD_DIR)
	c++ $(STD) $(CONFIG) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/stb.o: $(SRC_DIR)/src/stb.c | $(BUILD_DIR)
	c++ $(STD) $(CONFIG) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/src/%.cpp $(patsubst %, $(BUILD_DIR)/%.spv.h, $(SHADERS)) | $(BUILD_DIR)
	c++ $(STD) $(CONFIG) $(WARNINGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/libhurdygurdy.a: $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(SRC)) $(BUILD_DIR)/vk_mem_alloc.o $(BUILD_DIR)/stb.o
	ar rcs $@ $^

$(BUILD_DIR)/tests: $(BUILD_DIR)/tests.o $(BUILD_DIR)/libhurdygurdy.a
	c++ $(STD) $(CONFIG) $(WARNINGS) -o $@ $(BUILD_DIR)/tests.o -L$(BUILD_DIR) -lhurdygurdy

clean:
	rm -rf $(BUILD_DIR) $(TEST_DIR)

-include $(BUILD_DIR)/*.d

