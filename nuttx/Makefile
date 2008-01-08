############################################################
# Makefile
#
#   Copyright (C) 2007 Gregory Nutt. All rights reserved.
#   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name Gregory Nutt nor the names of its contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
############################################################

TOPDIR		:= ${shell pwd | sed -e 's/ /\\ /g'}
-include ${TOPDIR}/.config
-include ${TOPDIR}/Make.defs

# Process architecture and board-specific directories

ARCH_DIR	= arch/$(CONFIG_ARCH)
ARCH_SRC	= $(ARCH_DIR)/src
ARCH_INC	= $(ARCH_DIR)/include
BOARD_DIR	= configs/$(CONFIG_ARCH_BOARD)

# Add-on directories.  These may or may not be in place in the
# NuttX source tree (they must be specifically installed)

PCODE_DIR	:= ${shell if [ -r pcode/Makefile ]; then echo "pcode"; fi}

# FSDIRS depend on file descriptor support; NONFSDIRS do not
#   (except for parts of FSDIRS).  We will exclude FSDIRS
#   from the build if file descriptor support is disabled

NONFSDIRS	= sched lib $(ARCH_SRC) mm examples/$(CONFIG_EXAMPLE) $(PCODE_DIR)
FSDIRS		= fs drivers

ifeq ($(CONFIG_NET),y)
NONFSDIRS	+= net netutils
endif

# CLEANDIRS are the directories that will clean in.  These are
#   all directories that we know about.
# MAKEDIRS are the directories in which we will build targets

CLEANDIRS	= $(NONFSDIRS) $(FSDIRS)
MAKEDIRS	= $(NONFSDIRS)

ifeq ($(CONFIG_NFILE_DESCRIPTORS),0)
ifneq ($(CONFIG_NSOCKET_DESCRIPTORS),0)
MAKEDIRS	+= fs
endif
ifeq ($(CONFIG_NET),y)
MAKEDIRS	+= drivers
endif
else
MAKEDIRS	+= $(FSDIRS)
endif

# LINKLIBS is the list of NuttX libraries that is passed to the
#   processor-specific Makefile to build the final target.
#   Libraries in FSDIRS are excluded if file descriptor support
#   is disabled.

LINKLIBS	= sched/libsched$(LIBEXT) $(ARCH_SRC)/libarch$(LIBEXT) mm/libmm$(LIBEXT) \
		  lib/liblib$(LIBEXT) examples/$(CONFIG_EXAMPLE)/lib$(CONFIG_EXAMPLE)$(LIBEXT)

ifeq ($(CONFIG_NET),y)
LINKLIBS	+= net/libnet$(LIBEXT) netutils/libnetutils$(LIBEXT) 
endif

ifeq ($(CONFIG_NFILE_DESCRIPTORS),0)
ifneq ($(CONFIG_NSOCKET_DESCRIPTORS),0)
LINKLIBS	+= fs/libfs$(LIBEXT)
endif
ifeq ($(CONFIG_NET),y)
LINKLIBS	+= drivers/libdrivers$(LIBEXT)
endif
else
LINKLIBS	+= fs/libfs$(LIBEXT) drivers/libdrivers$(LIBEXT)
endif

ifneq ($(PCODE_DIR),)
LINKLIBS	+= pcode/libpcode$(LIBEXT)
endif

# This is the name of the final target
BIN		= nuttx$(EXEEXT)

all: $(BIN)
.PHONY: clean context clean_context distclean

# Build the mkconfig tool used to create include/nuttx/config.h
tools/mkconfig:
	$(MAKE) -C tools -f Makefile.mkconfig TOPDIR=$(TOPDIR)  mkconfig

# Create the include/nuttx/config.h file
include/nuttx/config.h: $(TOPDIR)/.config tools/mkconfig
	tools/mkconfig $(TOPDIR) > include/nuttx/config.h

# link the arch/<arch-name>/include dir to include/arch
include/arch: Make.defs
	@if [ -h include/arch ]; then \
		rm -f include/arch ; \
	else \
		if [ -e include/arch ]; then \
			echo "include/arch exists but is not a symbolic link" ; \
			exit 1 ; \
		fi ; \
	fi
	@ln -s $(TOPDIR)/$(ARCH_DIR)/include include/arch

# Link the configs/<board-name>/include dir to include/arch/board
include/arch/board: Make.defs include/arch
	@if [ -h include/arch/board ]; then \
			rm -f include/arch/board ; \
	else \
		if [ -e include/arch/board ]; then \
			echo "include/arch/board exists but is not a symbolic link" ; \
			exit 1 ; \
		fi ; \
	fi
	@ln -s $(TOPDIR)/$(BOARD_DIR)/include include/arch/board

# Link the configs/<board-name>/src dir to arch/<arch-name>/src/board
$(ARCH_SRC)/board: Make.defs
	@if [ -h $(ARCH_SRC)/board ]; then \
		rm -f $(ARCH_SRC)/board ; \
	else \
		if [ -e $(ARCH_SRC)/board ]; then \
			echo "$(ARCH_SRC)/board exists but is not a symbolic link" ; \
			exit 1 ; \
		fi ; \
	fi
	@ln -s $(TOPDIR)/$(BOARD_DIR)/src $(ARCH_SRC)/board

# Link arch/<arch-name>/include/<chip-name> to arch/<arch-name>/include/chip
$(ARCH_SRC)/chip: Make.defs
ifneq ($(CONFIG_ARCH_CHIP),)
	@if [ -h $(ARCH_SRC)/chip ]; then \
		rm -f $(ARCH_SRC)/chip ; \
	else \
		if [ -e $(ARCH_SRC)/chip ]; then \
			echo "$(ARCH_SRC)/chip exists but is not a symbolic link" ; \
			exit 1 ; \
		fi ; \
	fi
	@ln -s $(CONFIG_ARCH_CHIP) $(ARCH_SRC)/chip
endif

# Link arch/<arch-name>/src/<chip-name> to arch/<arch-name>/src/chip
$(ARCH_INC)/chip: Make.defs
ifneq ($(CONFIG_ARCH_CHIP),)
	@if [ -e $(ARCH_INC)/chip ]; then \
		if [ -h $(ARCH_INC)/chip ]; then \
			rm -f $(ARCH_INC)/chip ; \
		else \
			echo "$(ARCH_INC)/chip exists but is not a symbolic link" ; \
			exit 1 ; \
		fi ; \
	fi
	@ln -s $(CONFIG_ARCH_CHIP) $(ARCH_INC)/chip
endif

dirlinks: include/arch include/arch/board $(ARCH_SRC)/board $(ARCH_SRC)/chip $(ARCH_INC)/chip

context: check_context include/nuttx/config.h dirlinks

clean_context:
	@rm -f include/nuttx/config.h include/arch
	@if [ -h include/arch ]; then rm -f include/arch ; fi
	@if [ -h $(ARCH_INC)/board ]; then rm -f $(ARCH_INC)/board ; fi
	@if [ -h $(ARCH_SRC)/board ]; then rm -f $(ARCH_SRC)/board ; fi
	@if [ -h $(ARCH_INC)/chip ]; then rm -f $(ARCH_INC)/chip ; fi
	@if [ -h $(ARCH_SRC)/chip ]; then rm -f $(ARCH_SRC)/chip ; fi

check_context:
	@if [ ! -e ${TOPDIR}/.config -o ! -e ${TOPDIR}/Make.defs ]; then \
		echo "" ; echo "Nuttx has not been configured:" ; \
		echo "  cd tools; ./configure.sh <target>\n" ; echo "" ;\
		exit 1 ; \
	fi

sched/libsched$(LIBEXT): context
	$(MAKE) -C sched TOPDIR=$(TOPDIR) libsched$(LIBEXT)

lib/liblib$(LIBEXT): context
	$(MAKE) -C lib TOPDIR=$(TOPDIR) liblib$(LIBEXT)

$(ARCH_SRC)/libarch$(LIBEXT): context
	$(MAKE) -C $(ARCH_SRC) TOPDIR=$(TOPDIR) libarch$(LIBEXT)

mm/libmm$(LIBEXT): context
	$(MAKE) -C mm TOPDIR=$(TOPDIR) libmm$(LIBEXT)

net/libnet$(LIBEXT): context
	$(MAKE) -C net TOPDIR=$(TOPDIR) libnet$(LIBEXT)

netutils/libnetutils$(LIBEXT): context
	$(MAKE) -C netutils TOPDIR=$(TOPDIR) libnetutils$(LIBEXT)

fs/libfs$(LIBEXT): context
	$(MAKE) -C fs TOPDIR=$(TOPDIR) libfs$(LIBEXT)

drivers/libdrivers$(LIBEXT): context
	$(MAKE) -C drivers TOPDIR=$(TOPDIR) libdrivers$(LIBEXT)

pcode/libpcode$(LIBEXT): context
	$(MAKE) -C pcode TOPDIR=$(TOPDIR) libpcode$(LIBEXT)

examples/$(CONFIG_EXAMPLE)/lib$(CONFIG_EXAMPLE)$(LIBEXT): context
	$(MAKE) -C examples/$(CONFIG_EXAMPLE) TOPDIR=$(TOPDIR) lib$(CONFIG_EXAMPLE)$(LIBEXT)

$(BIN):	context depend $(LINKLIBS)
	$(MAKE) -C $(ARCH_SRC) TOPDIR=$(TOPDIR) LINKLIBS="$(LINKLIBS)" $(BIN)

depend:
	@for dir in $(MAKEDIRS) ; do \
		$(MAKE) -C $$dir TOPDIR=$(TOPDIR) depend ; \
	done

subdir_clean:
	@for dir in $(CLEANDIRS) ; do \
		if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir TOPDIR=$(TOPDIR) clean ; \
		fi \
	done
	$(MAKE) -C tools -f Makefile.mkconfig TOPDIR=$(TOPDIR) clean
	$(MAKE) -C mm -f Makefile.test TOPDIR=$(TOPDIR) clean

clean: subdir_clean
	rm -f $(BIN) $(BIN).* mm_test *.map *~

subdir_distclean:
	@for dir in $(CLEANDIRS) ; do \
		if [ -e $$dir/Makefile ]; then \
			$(MAKE) -C $$dir TOPDIR=$(TOPDIR) distclean ; \
		fi \
	done

distclean: clean subdir_distclean clean_context
	rm -f Make.defs setenv.sh .config


