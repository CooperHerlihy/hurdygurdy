SRC_DIR := .
BUILD_DIR := $(SRC_DIR)/build
TEST_DIR := $(SRC_DIR)/hg_test_dir

ifeq ($(OS),Windows_NT)
    IS_WINDOWS := 1
else
    IS_WINDOWS := 0
endif

ifeq ($(IS_WINDOWS),1)
    CXX := cl
    AR := lib
    STD := /std:c++17
    WARNINGS := /W4 /WX /D _CRT_SECURE_NO_WARNINGS
    DEBUG_CONFIG := /nologo /Zi /Od
    RELEASE_CONFIG := /nologo /O2 /D NDEBUG
    OBJ_EXT := .obj
    LIB_EXT := .lib
    EXE_EXT := .exe
else
    CXX := c++
    AR := ar
    STD := -std=c++17 -MMD -MP
    WARNINGS := -Werror -Wall -Wextra -Wconversion -Wshadow -pedantic
    DEBUG_CONFIG := -g -O0 -fsanitize=undefined -fno-exceptions -fno-rtti
    RELEASE_CONFIG := -O3 -DNDEBUG -fno-exceptions -fno-rtti
    OBJ_EXT := .o
    LIB_EXT := .a
    EXE_EXT :=
endif

CONFIG := $(DEBUG_CONFIG)

ifeq ($(IS_WINDOWS),1)
    INCLUDES := /I"$(BUILD_DIR)" /I"$(SRC_DIR)/include" /I"$(SRC_DIR)/vendor/libX11/include"
else
    INCLUDES := -I$(BUILD_DIR) -I$(SRC_DIR)/include -I$(SRC_DIR)/vendor/libX11/include
endif

SHADERS := $(SRC_DIR)/src/sprite.vert $(SRC_DIR)/src/sprite.frag
SPV := $(patsubst $(SRC_DIR)/src/%, $(BUILD_DIR)/%.spv, $(SHADERS))
SPV_H := $(addsuffix .h, $(SPV))

.PHONY: all debug release clean shaders

all: $(BUILD_DIR)/test$(EXE_EXT) $(TEST_DIR)

debug:
	$(MAKE) CONFIG="$(DEBUG_CONFIG)"

release:
	$(MAKE) CONFIG="$(RELEASE_CONFIG)"

$(BUILD_DIR):
ifeq ($(IS_WINDOWS),1)
	mkdir "$(BUILD_DIR)"
else
	mkdir -p $@
endif

$(TEST_DIR):
ifeq ($(IS_WINDOWS),1)
	mkdir "$(TEST_DIR)"
else
	mkdir -p $@
endif

$(BUILD_DIR)/embed_file$(EXE_EXT): $(SRC_DIR)/src/embed_file.cpp | $(BUILD_DIR)
ifeq ($(IS_WINDOWS),1)
	$(CXX) $(STD) $(WARNINGS) $(CONFIG) $(INCLUDES) /Fd:"$(BUILD_DIR)/embed_file.pdb" /Fo:"$(BUILD_DIR)/embed_file$(OBJ_EXT)" /Fe:"$@" $<
else
	$(CXX) $(STD) $(CONFIG) $(WARNINGS) $(INCLUDES) $< -o $@
endif

$(BUILD_DIR)/%.spv: $(SRC_DIR)/src/% | $(BUILD_DIR)
ifeq ($(IS_WINDOWS),1)
	glslc -o "$@" "$<"
else
	glslc -o $@ $<
endif

$(BUILD_DIR)/%.spv.h: $(BUILD_DIR)/%.spv $(BUILD_DIR)/embed_file$(EXE_EXT)
	$(BUILD_DIR)/embed_file$(EXE_EXT) $< $(notdir $<) > $@

shaders: $(SPV_H)

$(BUILD_DIR)/vk_mem_alloc$(OBJ_EXT): $(SRC_DIR)/src/vk_mem_alloc.cpp | $(BUILD_DIR)
ifeq ($(IS_WINDOWS),1)
	$(CXX) $(STD) $(CONFIG) $(INCLUDES) /Fd:"$(@:.obj=.pdb)" /Fo:"$@" /c $<
else
	$(CXX) $(STD) $(CONFIG) $(INCLUDES) -c $< -o $@
endif

$(BUILD_DIR)/stb$(OBJ_EXT): $(SRC_DIR)/src/stb.c | $(BUILD_DIR)
ifeq ($(IS_WINDOWS),1)
	$(CXX) $(STD) $(CONFIG) $(INCLUDES) /Fd:"$(@:.obj=.pdb)" /Fo:"$@" /c $<
else
	$(CXX) $(STD) $(CONFIG) $(INCLUDES) -c $< -o $@
endif

$(BUILD_DIR)/hurdygurdy$(OBJ_EXT): $(SRC_DIR)/src/hurdygurdy.cpp $(SPV_H) | $(BUILD_DIR)
ifeq ($(IS_WINDOWS),1)
	$(CXX) $(STD) $(WARNINGS) $(CONFIG) $(INCLUDES) /Fd:"$(@:.obj=.pdb)" /Fo:"$@" /c $<
else
	$(CXX) $(STD) $(CONFIG) $(WARNINGS) $(INCLUDES) -c $< -o $@
endif

$(BUILD_DIR)/test$(OBJ_EXT): $(SRC_DIR)/src/test.cpp | $(BUILD_DIR)
ifeq ($(IS_WINDOWS),1)
	$(CXX) $(STD) $(WARNINGS) $(CONFIG) $(INCLUDES) /Fd:"$(@:.obj=.pdb)" /Fo:"$@" /c $<
else
	$(CXX) $(STD) $(CONFIG) $(WARNINGS) $(INCLUDES) -c $< -o $@
endif

$(BUILD_DIR)/libhurdygurdy$(LIB_EXT): $(BUILD_DIR)/hurdygurdy$(OBJ_EXT) $(BUILD_DIR)/vk_mem_alloc$(OBJ_EXT) $(BUILD_DIR)/stb$(OBJ_EXT)
ifeq ($(IS_WINDOWS),1)
	$(AR) /nologo /OUT:"$@" $^
else
	$(AR) rcs $@ $^
endif

$(BUILD_DIR)/test$(EXE_EXT): $(BUILD_DIR)/test$(OBJ_EXT) $(BUILD_DIR)/libhurdygurdy$(LIB_EXT) | $(TEST_DIR)
ifeq ($(IS_WINDOWS),1)
	$(CXX) $(STD) $(WARNINGS) $(CONFIG) /Fe:"$@" $< $(BUILD_DIR)/libhurdygurdy$(LIB_EXT) User32.lib
else
	$(CXX) $(STD) $(CONFIG) $(WARNINGS) -o $@ $< -L$(BUILD_DIR) -lhurdygurdy
endif

clean:
ifeq ($(IS_WINDOWS),1)
	rmdir /s /q "$(BUILD_DIR)"
	rmdir /s /q "$(TEST_DIR)"
else
	rm -rf $(BUILD_DIR)
	rm -rf $(TEST_DIR)
endif

-include $(BUILD_DIR)/*.d
