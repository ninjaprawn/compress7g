CC= gcc
OPTS= -Wall -g

ifeq ($(shell uname),WindowsNT)
CCACHE :=
else
CCACHE := $(shell which ccache)
endif

all: compress7g

extract2g: compress7g.c compress7g.h
	$(CCACHE) $(CC) $(OPTS) $< -o $@

.PHONY: clean
clean:
	rm -f compress7g
