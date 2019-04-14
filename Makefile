export TOPDIR:=$(CURDIR)
export OUTDIR:=$(TOPDIR)/out
export INCDIR:=$(TOPDIR)

export BUILD_EXECUTABLE:=$(TOPDIR)/build_executable.mk
export CLEAR_VARS:=$(TOPDIR)/clear_vars.mk

MAKE:=make --no-print-directory

makefile_lists := $(shell find ./test -maxdepth 3 -name Makefile)

all:
	@for mk in $(makefile_lists); \
	do \
		$(MAKE) -C `dirname $${mk}` all; \
	done

clean:
	@for mk in $(makefile_lists); \
	do \
		$(MAKE) -C `dirname $${mk}` clean; \
	done
