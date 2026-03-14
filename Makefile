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

IMGUI_BACKEND := \
	imgui_impl_glfw.cpp \
	imgui_impl_vulkan.cpp

SRC := \
	utils.cpp \
	thread.cpp \
	resources.cpp \
	pipeline2d.cpp \
	pipeline3d.cpp \
	window_glfw.cpp \
	vulkan.cpp \
	test.cpp

SHADERS := \
	sprite.vert \
	sprite.frag \
	model.vert \
	model.frag

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
	c++ $(STD) $(CONFIG) $(WARNINGS) $(INCLUDES) $< -o $@

$(BUILD_DIR)/%.spv: $(SRC_DIR)/src/% | $(BUILD_DIR)
	glslc -o $@ $<

$(BUILD_DIR)/%.spv.h: $(BUILD_DIR)/%.spv $(BUILD_DIR)/embed_file
	$(BUILD_DIR)/embed_file $< $(notdir $<) > $@

$(BUILD_DIR)/vk_mem_alloc.o: $(SRC_DIR)/src/vk_mem_alloc.cpp | $(BUILD_DIR)
	c++ $(STD) $(CONFIG) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/stb.o: $(SRC_DIR)/src/stb.c | $(BUILD_DIR)
	c++ $(STD) $(CONFIG) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/vendor/imgui/backends/%.cpp | $(BUILD_DIR)
	c++ $(STD) $(CONFIG) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/vendor/imgui/%.cpp | $(BUILD_DIR)
	c++ $(STD) $(CONFIG) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/src/%.cpp $(patsubst %, $(BUILD_DIR)/%.spv.h, $(SHADERS)) | $(BUILD_DIR)
	c++ $(STD) $(CONFIG) $(WARNINGS) $(INCLUDES) -c $< -o $@

LIB_FILES := \
	$(patsubst %.cpp, $(BUILD_DIR)/%.o, $(SRC)) \
	$(patsubst %.cpp, $(BUILD_DIR)/%.o, $(IMGUI_BACKEND)) \
	$(patsubst $(SRC_DIR)/vendor/imgui/%.cpp, $(BUILD_DIR)/%.o, $(wildcard $(SRC_DIR)/vendor/imgui/*.cpp)) \

$(BUILD_DIR)/libhurdygurdy.a: $(LIB_FILES) $(BUILD_DIR)/vk_mem_alloc.o $(BUILD_DIR)/stb.o
	ar rcs $@ $^

$(BUILD_DIR)/minimal: $(BUILD_DIR)/minimal.o $(BUILD_DIR)/libhurdygurdy.a
	c++ $(STD) $(CONFIG) $(WARNINGS) -o $@ $< -L$(BUILD_DIR) -lhurdygurdy -lglfw

$(BUILD_DIR)/editor: $(BUILD_DIR)/editor.o $(BUILD_DIR)/libhurdygurdy.a
	c++ $(STD) $(CONFIG) $(WARNINGS) -o $@ $< -L$(BUILD_DIR) -lhurdygurdy -lglfw

clean:
	rm -rf $(BUILD_DIR) $(TEST_DIR)

-include $(BUILD_DIR)/*.d

