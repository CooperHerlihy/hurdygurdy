SOURCES := $(wildcard shaders/*.vert) $(wildcard shaders/*.frag) $(wildcard shaders/*.comp)
SPV_FILES := $(SOURCES:.vert=.vert.spv)
SPV_FILES := $(SPV_FILES:.frag=.frag.spv)
SPV_FILES := $(SPV_FILES:.comp=.comp.spv)

shaders: $(SPV_FILES)

%.vert.spv: %.vert
	glslc $< -o $@

%.frag.spv: %.frag
	glslc $< -o $@

%.comp.spv: %.comp
	glslc $< -o $@
