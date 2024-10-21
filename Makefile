BUILD_DIR := bin
OBJ_DIR := bin-obj

COMPILER_FLAGS := -g -shared -fdeclspec
LINKER_FLAGS := -luser32.lib
DEFINES := -DDLL_EXPORT -DPLAT_WIN32 -D_CRT_SECURE_NO_WARNINGS

# Source: (https://stackoverflow.com/questions/3774568/makefile-issue-smart-way-to-scan-directory-tree-for-c-files)
# Make does not offer a recursive wildcard function, so here's one:
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

C_SOURCE_FILES := $(call rwildcard,$(ASSEMBLY)/,*.c)

.PHONY: build
build:
	clang $(C_SOURCE_FILES) -I$(ASSEMBLY)\src $(ADDL_INCLUDE_DIRS) $(COMPILER_FLAGS) -o $(BUILD_DIR)\$(ASSEMBLY).$(EXTENSION) $(ADDL_LINKER_FLAGS) $(LINKER_FLAGS) $(DEFINES)
