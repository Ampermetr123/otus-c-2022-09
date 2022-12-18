CC ?= gcc  
BUILDTYPE ?= debug
BINDIR = bin/${BUILDTYPE}
CFLAGS += -Wall -Wextra -pedantic -Werror=implicit-function-declaration -std=gnu17
ifeq ($(BUILDTYPE), debug)
  CFLAGS += -O0 -ggdb -fdiagnostics-color=always
endif    
ifeq ($(BUILDTYPE), release)
  CFLAGS += -O3
endif  

PROG_NAME := huge32

this := $(BINDIR)/$(PROG_NAME)	

all: $(this)

sources := main.c crc32.c
sources := $(addprefix $(shell pwd)/, $(sources))  

$(this) :	$(sources)
	@mkdir -p $(dir $@) 
	$(CC) $^  $(CFLAGS) -o $(this)

clean:
	rm -f $(this)

.PHONY: all clean