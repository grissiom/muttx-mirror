############################################################################
# arch/rgmp/src/Makefile
#
#   Copyright (C) 2011 Gregory Nutt. All rights reserved.
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
# 3. Neither the name NuttX nor the names of its contributors may be
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
############################################################################

-include $(TOPDIR)/Make.defs
include $(CONFIG_RGMP_SUBARCH)/Make.defs

RGMP_ARCH_ASRCS := $(addprefix $(CONFIG_RGMP_SUBARCH)/,$(RGMP_ARCH_ASRCS))
RGMP_ARCH_CSRCS := $(addprefix $(CONFIG_RGMP_SUBARCH)/,$(RGMP_ARCH_CSRCS))

CFLAGS		+= -I$(TOPDIR)/sched -I$(TOPDIR)/fs

ASRCS		= $(RGMP_ARCH_ASRCS)
CSRCS		= nuttx.c rgmp.c bridge.c $(RGMP_ARCH_CSRCS)
AOBJS		= $(ASRCS:.S=$(OBJEXT))
COBJS		= $(CSRCS:.c=$(OBJEXT))

SRCS		= $(ASRCS) $(CSRCS)
OBJS		= $(AOBJS) $(COBJS)

LDFLAGS		+= -T$(RGMPLKSCPT)
LDPATHS		= $(addprefix -L$(TOPDIR)/,$(dir $(LINKLIBS)))
LDLIBS		= $(patsubst lib%,-l%,$(basename $(notdir $(LINKLIBS))))
LDPATHS		+= -L$(RGMPLIBDIR)
LDLIBS		+= -lrgmp -lm -ltest $(shell gcc -print-libgcc-file-name)

all: libarch$(LIBEXT)
.PHONY: clean distclean depend

$(AOBJS): %$(OBJEXT): %.S
	$(call ASSEMBLE, $<, $@)

$(COBJS) $(LINKOBJS): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

# The architecture-specific library
libarch$(LIBEXT): $(OBJS)
	@( for obj in $(OBJS) ; do \
		$(call ARCHIVE, $@, $${obj}); \
	done ; )

# Generate the final NuttX binary by linking the host-specific objects with the NuttX
# specific objects (with munged names)

nuttx$(EXEEXT):
	@echo "LD:  nuttx$(EXEEXT)"
	@$(LD) $(LDFLAGS) $(LDPATHS) --start-group $(LDLIBS) --end-group -o $(TOPDIR)/$@
	@$(OBJDUMP) -S $(TOPDIR)/$@ > $(TOPDIR)/nuttx.asm
	@$(NM) -n $(TOPDIR)/$@ > $(TOPDIR)/nuttx.sym
	@$(OBJCOPY) -S -O binary $(TOPDIR)/$@ nuttx.img
	@cp nuttx.img $(TOPDIR)/kernel.img

# This is part of the top-level export target

export_head:

# Dependencies

.depend: Makefile $(SRCS)
	@$(MKDEP) $(CC) -- $(CFLAGS) -- $(SRCS) >Make.dep
	@touch $@

depend: .depend

clean:
	@rm $(TOPDIR)/kernel.img nuttx.img 
	@rm -f libarch$(LIBEXT) *~ .*.swp
	$(call CLEAN)

distclean: clean
	@rm -f Make.dep .depend

-include Make.dep
