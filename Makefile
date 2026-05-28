SRC_DIR := .
BUILD_DIR := $(SRC_DIR)/build
TEST_DIR := $(SRC_DIR)/hg_test_dir

STD := -std=c++17 -MMD -MP
WARNINGS := -Werror -Wall -Wextra -pedantic

DEBUG_CONFIG := -g -Og -fsanitize=undefined -fno-exceptions -fno-rtti
RELEASE_CONFIG := -O3 -DNDEBUG -fno-exceptions -fno-rtti
CONFIG := $(DEBUG_CONFIG)

INCLUDES := \
	-I$(BUILD_DIR) \
	-I$(SRC_DIR)/include \
	-I$(SRC_DIR)/vendor/SDL/include \
	-I$(SRC_DIR)/vendor/Vulkan-Headers/include \
	-I$(SRC_DIR)/vendor/imgui \
	-I$(SRC_DIR)/vendor/imgui/backends

SHADERS := \
	noise.comp \
	skybox.vert \
	skybox.frag \
	sprite.vert \
	sprite.frag \
	model.vert \
	model.frag

IMGUI_BACKEND := \
	imgui_impl_sdl3.cpp \
	imgui_impl_vulkan.cpp

SRC := \
	hurdygurdy.cpp \
	vulkan.cpp \
	sdl.cpp \
	test.cpp

TARGETS := \
	editor \
	noise \
	minimal

.PHONY: all debug release clean

all: $(patsubst %, $(BUILD_DIR)/%, $(TARGETS)) $(patsubst %, $(BUILD_DIR)/%.spv, $(SHADERS))

debug:
	$(MAKE) CONFIG="$(DEBUG_CONFIG)"

release:
	$(MAKE) CONFIG="$(RELEASE_CONFIG)"

$(BUILD_DIR):
	mkdir -p $@

$(TEST_DIR):
	mkdir -p $@

$(BUILD_DIR)/vk_mem_alloc.o: $(SRC_DIR)/src/vk_mem_alloc.cpp | $(BUILD_DIR)
	c++ $(STD) $(CONFIG) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/stb.o: $(SRC_DIR)/src/stb.c | $(BUILD_DIR)
	c++ $(STD) $(CONFIG) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.vert.spv: $(SRC_DIR)/src/%.vert $(SRC_DIR)/include/hurdygurdy.glsl | $(BUILD_DIR)
	glslc -o $@ $< -I$(SRC_DIR)/include

$(BUILD_DIR)/%.frag.spv: $(SRC_DIR)/src/%.frag $(SRC_DIR)/include/hurdygurdy.glsl | $(BUILD_DIR)
	glslc -o $@ $< -I$(SRC_DIR)/include

$(BUILD_DIR)/%.comp.spv: $(SRC_DIR)/src/%.comp $(SRC_DIR)/include/hurdygurdy.glsl | $(BUILD_DIR)
	glslc -o $@ $< -I$(SRC_DIR)/include

$(BUILD_DIR)/%.o: $(SRC_DIR)/vendor/imgui/backends/%.cpp | $(BUILD_DIR)
	c++ $(STD) $(CONFIG) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/vendor/imgui/%.cpp | $(BUILD_DIR)
	c++ $(STD) $(CONFIG) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/src/%.cpp | $(BUILD_DIR)
	c++ $(STD) $(CONFIG) $(WARNINGS) $(INCLUDES) -c $< -o $@

LIB_FILES := \
	$(patsubst %.cpp, $(BUILD_DIR)/%.o, $(SRC)) \
	$(patsubst %.cpp, $(BUILD_DIR)/%.o, $(IMGUI_BACKEND)) \
	$(patsubst $(SRC_DIR)/vendor/imgui/%.cpp, $(BUILD_DIR)/%.o, $(wildcard $(SRC_DIR)/vendor/imgui/*.cpp)) \

$(BUILD_DIR)/libhurdygurdy.a: $(LIB_FILES) $(BUILD_DIR)/vk_mem_alloc.o $(BUILD_DIR)/stb.o
	ar rcs $@ $^

$(BUILD_DIR)/%: $(BUILD_DIR)/%.o $(BUILD_DIR)/libhurdygurdy.a | $(TEST_DIR)
	c++ $(STD) $(CONFIG) $(WARNINGS) -o $@ $< -L$(BUILD_DIR) -lhurdygurdy -lSDL3

clean:
	rm -rf $(BUILD_DIR) $(TEST_DIR)

-include $(BUILD_DIR)/*.d

