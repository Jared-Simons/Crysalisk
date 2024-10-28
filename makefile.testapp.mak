DIR := $(subst /,\,${CURDIR})
BUILD_DIR := bin
OBJ_DIR := bin-obj

ASSEMBLY := testapp
EXTENSION := .exe

COMPILER_FLAGS := -g -fdeclspec
INCLUDE_DIRS := -Iengine\src 
LINKER_FLAGS := -g -L$(OBJ_DIR)\engine -l$(BUILD_DIR)\engine.lib
DEFINES := -D_DEBUG -DPLAT_WIN32 -D_CRT_SECURE_NO_WARNINGS

# Source: (https://stackoverflow.com/questions/3774568/makefile-issue-smart-way-to-scan-directory-tree-for-c-files)
# Make does not offer a recursive wildcard function, so here's one:
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

C_SRC_FILES := $(call rwildcard,$(ASSEMBLY)/,*.c)
OBJ_FILES := $(C_SRC_FILES:%=$(OBJ_DIR)/%.o)
DIRECTORIES := \$(ASSEMBLY)\src $(subst $(DIR),,$(shell dir $(ASSEMBLY)\src /S /AD /B | findstr /i src))

.PHONY: all
all: scaffold compile link

.PHONY: scaffold
scaffold:
	@echo Scaffolding folder structure...
	@setlocal enableextensions enabledelayedexpansion && mkdir $(addprefix $(OBJ_DIR), $(DIRECTORIES)) 2>NUL || cd .
	@setlocal enableextensions enabledelayedexpansion && mkdir $(BUILD_DIR) 2>NUL || cd .

.PHONY: link
link: scaffold $(OBJ_FILES)
	@clang $(OBJ_FILES) -o $(BUILD_DIR)\$(ASSEMBLY)$(EXTENSION) $(LINKER_FLAGS)

.PHONY: compile
compile:
	@echo Compiling...

$(OBJ_DIR)/%.c.o: %.c 
	@echo 	$<...
	@clang $< $(COMPILER_FLAGS) -c -o $@ $(DEFINES) $(INCLUDE_DIRS)

.PHONY: gen_compile_flags 
gen_compile_flags:
	$(shell powershell \"$(INCLUDE_FLAGS) $(DEFINES) -ferror-limit=0\".replace('-I', '-I..\').replace(' ', \"`n\").replace('-I..\C:', '-IC:') > $(ASSEMBLY)/compile_flags.txt)

.PHONY: clean
clean:
	@echo Cleaning "$(ASSEMBLY)"
	@if exist $(BUILD_DIR)\$(ASSEMBLY)$(EXTENSION) del $(BUILD_DIR)\$(ASSEMBLY)$(EXTENSION)
	@if exist $(BUILD_DIR)\$(ASSEMBLY).* del $(BUILD_DIR)\$(ASSEMBLY).*
	@if exist $(OBJ_DIR)\$(ASSEMBLY) rmdir /s /q $(OBJ_DIR)\$(ASSEMBLY)
