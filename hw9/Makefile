CC ?= gcc
CFLAGS := ${CFLAGS}
CFLAGS += -Wall -Wextra -pedantic  -Werror=implicit-function-declaration -std=gnu17 
BUILDTYPE ?= debug
SOURCE_HEAD ?= $(shell pwd)/src
SOURCE_DIRS := .   # папки с исходными файлами, относительно SRCDIR
BINDIR = bin/${BUILDTYPE}
PROG_NAME := mydaemon
CONFIG_FILE := $(SOURCE_HEAD)/$(PROG_NAME).ini
#DEFINES:= __STDC_WANT_LIB_EXT1__=1

ifeq ($(BUILDTYPE), debug)
 CFLAGS += -O0 -ggdb -fdiagnostics-color=always
endif

ifeq ($(BUILDTYPE), release)
 CFLAGS += -O3
endif

this := $(BINDIR)/$(PROG_NAME)

SOURCE_DIRS := $(addprefix $(SOURCE_HEAD)/,$(SOURCE_DIRS))
src_files := $(wildcard $(addsuffix /*.c, $(SOURCE_DIRS)) )
obj_files := $(patsubst $(SOURCE_HEAD)%.c,$(BINDIR)%.o, $(src_files))
include_flags := $(addprefix -I, $(SOURCE_DIRS)) 

all: $(this)


# add static lib
LIB =ciniparser
lib_src_dir := ccan/$(LIB)
lib_bin_dir := bin/ciniparser/${BUILDTYPE}
lib_target :=  $(lib_bin_dir)/lib$(LIB).a
lib_src_files := $(wildcard $(addsuffix /*.c, $(lib_src_dir))) 
lib_obj_files := $(patsubst  $(lib_src_dir)%.c,$(lib_bin_dir)%.o, $(lib_src_files)) 
$(lib_target) : $(lib_obj_files)
	ar cr $@ $^
$(lib_bin_dir)%.o: $(lib_src_dir)%.c
	@mkdir -p $(dir $@)
	$(CC) $< -c $(CFLAGS) -I$(lib_src_dir) -I. -pipe -o $@

include_flags += -I$(lib_src_dir)
link_flags += -L$(lib_bin_dir) -l$(LIB)


$(this): $(obj_files) $(lib_target) $(CONFIG_FILE)
	$(CC)  $(obj_files) -o $@ $(link_flags) -pipe 
	cp $(CONFIG_FILE) $(BINDIR)
	@echo Build done!

$(BINDIR)%.o: $(SOURCE_DIRS)%.c
	@mkdir -p $(dir $@)
	$(CC) $< -c $(CFLAGS) $(include_flags) $(addprefix -D, $(DEFINES)) -MMD -pipe -o $@

include $(BINDIR/*.d) 

run: $(this)
	$(this) -d

# valgrind:
# 	valgrind --leak-check=full --show-leak-kinds=all -- $(this) Moscow

clean:
	rm -f $(obj_files) $(patsubst %.o, %.d, $(obj_files))
	rm -f $(this)
	rm -f $(lib_obj_files)
	rm -f $(lib_target)
	rm -f $(patsubst $(SOURCE_HEAD)%.ini,$(BINDIR)%.ini, $(CONFIG_FILE))

info:
	@echo PROG_NAME: $(PROG_NAME)
	@echo BUILDTYPE: $(BUILDTYPE)
	@echo BINDIR: $(BINDIR)
	@echo SOURCE_HEAD: $(SOURCE_HEAD)
	@echo SOURCE_DIRS: $(SOURCE_DIRS)
	@echo CFLAGS: $(CFLAGS)
	@echo DEFINES: $(DEFINES)
	@echo include_flags: $(include_flags)
	@echo src_files: $(src_files)
	@echo obj_files: $(obj_files)


.PHONY: all clean info run json valgrind
