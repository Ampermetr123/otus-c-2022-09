CC ?= gcc
CFLAGS := ${CFLAGS}
CFLAGS += -Wall -Wextra -pedantic  -Werror=implicit-function-declaration -std=gnu11 
BUILDTYPE ?= debug
SOURCE_HEAD ?= $(shell pwd)/src
SOURCE_DIRS := .   # папки с исходными файлами, относительно SRCDIR
BINDIR = bin/${BUILDTYPE}
PROG_NAME := weather
DEFINES:= PROG_NAME=\"$(PROG_NAME)\"

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

# shared lib libcurl
link_flags := $(shell pkg-config --libs libcurl)
include_flags += $(shell pkg-config --cflags libcurl)

#static lib lib ccan/json
jsonlib_src_dir := libs/json
jsonlib_bin_dir := libs/bin/json/${BUILDTYPE}
jsonlib := $(jsonlib_bin_dir)/libjson.a
jsonlib_src_files := $(wildcard $(addsuffix /*.c, $(jsonlib_src_dir))) 
jsonlib_obj_files := $(patsubst  $(jsonlib_src_dir)%.c,$(jsonlib_bin_dir)%.o, $(jsonlib_src_files)) 
json: $(jsonlib)
$(jsonlib) : $(jsonlib_obj_files)
	ar cr $@ $^
$(jsonlib_bin_dir)%.o: $(jsonlib_src_dir)%.c
	@mkdir -p $(dir $@)
	$(CC) $< -c $(CFLAGS) -I$(jsonlib_src_dir) -pipe -o $@

include_flags += -I$(jsonlib_src_dir)
link_flags += -L$(jsonlib_bin_dir) -ljson



$(this): $(obj_files) $(jsonlib)
	$(CC)  $(obj_files) -o $@ $(link_flags) -pipe 
	@echo Build done!

$(BINDIR)%.o: $(SOURCE_DIRS)%.c
	@mkdir -p $(dir $@)
	$(CC) $< -c $(CFLAGS) $(include_flags) $(addprefix -D, $(DEFINES)) -MMD -pipe -o $@

include $(wildcard *.d) 

run: $(this)
	$(this) Moscow

valgrind:
	valgrind --leak-check=full --show-leak-kinds=all -- $(this) Moscow

clean:
	rm -f $(obj_files) $(patsubst %.o, %.d, $(obj_files))
	rm -f $(this)
	rm -f $(jsonlib)

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
