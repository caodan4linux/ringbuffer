export TOPDIR:=$(CURDIR)
export OUTDIR:=$(TOPDIR)/out
export INCDIR:=$(TOPDIR)/src
export BUILD_SCRIPTS_DIR:=$(TOPDIR)/build_scripts

export BUILD_EXECUTABLE:=$(BUILD_SCRIPTS_DIR)/build_executable.mk
export CLEAR_VARS:=$(BUILD_SCRIPTS_DIR)/clear_vars.mk

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
	@$(RM) -rf $(OUTDIR)
