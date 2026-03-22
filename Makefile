SRC_DIR := .
BUILD_DIR := $(SRC_DIR)/build
TEST_DIR := $(SRC_DIR)/hg_test_dir

STD := -std=c++17 -MMD -MP
WARNINGS := -Werror -Wall -Wextra -pedantic

DEBUG_CONFIG := -g -O0 -fsanitize=undefined -fno-exceptions -fno-rtti
RELEASE_CONFIG := -O3 -DNDEBUG -fno-exceptions -fno-rtti
CONFIG := $(DEBUG_CONFIG)

INCLUDES := \
	-I$(BUILD_DIR) \
	-I$(SRC_DIR)/include \
	-I$(SRC_DIR)/vendor/libX11/include \
	-I$(SRC_DIR)/vendor/imgui \
	-I$(SRC_DIR)/vendor/imgui/backends

SHADERS := \
	noise.comp \
	sprite.vert \
	sprite.frag \
	model.vert \
	model.frag

IMGUI_BACKEND := \
	imgui_impl_glfw.cpp \
	imgui_impl_vulkan.cpp

SRC := \
	utils.cpp \
	concurrency.cpp \
	resources.cpp \
	pipeline2d.cpp \
	pipeline3d.cpp \
	window_glfw.cpp \
	vulkan.cpp \
	test.cpp

TARGETS := \
	editor \
	minimal

.PHONY: all debug release clean

all: $(patsubst %, $(BUILD_DIR)/%, $(TARGETS))

debug:
	$(MAKE) CONFIG="$(DEBUG_CONFIG)"

release:
	$(MAKE) CONFIG="$(RELEASE_CONFIG)"

$(BUILD_DIR):
	mkdir -p $@

$(TEST_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.vert.spv: $(SRC_DIR)/src/%.vert | $(BUILD_DIR)
	glslc -o $@ $< -I$(SRC_DIR)/include

$(BUILD_DIR)/%.frag.spv: $(SRC_DIR)/src/%.frag | $(BUILD_DIR)
	glslc -o $@ $< -I$(SRC_DIR)/include

$(BUILD_DIR)/%.comp.spv: $(SRC_DIR)/src/%.comp | $(BUILD_DIR)
	glslc -o $@ $< -I$(SRC_DIR)/include

$(BUILD_DIR)/vk_mem_alloc.o: $(SRC_DIR)/src/vk_mem_alloc.cpp | $(BUILD_DIR)
	c++ $(STD) $(CONFIG) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/stb.o: $(SRC_DIR)/src/stb.c | $(BUILD_DIR)
	c++ $(STD) $(CONFIG) $(INCLUDES) -c $< -o $@

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

SHADERS_SPV := $(patsubst %, $(BUILD_DIR)/%.spv, $(SHADERS))

$(BUILD_DIR)/%: $(BUILD_DIR)/%.o $(BUILD_DIR)/libhurdygurdy.a | $(SHADERS_SPV) $(TEST_DIR)
	c++ $(STD) $(CONFIG) $(WARNINGS) -o $@ $< -L$(BUILD_DIR) -lhurdygurdy -lglfw

clean:
	rm -rf $(BUILD_DIR) $(TEST_DIR)

-include $(BUILD_DIR)/*.d

