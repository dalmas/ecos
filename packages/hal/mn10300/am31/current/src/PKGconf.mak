#==============================================================================
#
#    makefile
#
#    hal/mn10300/am31/src
#
#==============================================================================
#####COPYRIGHTBEGIN####
#
# -------------------------------------------
# The contents of this file are subject to the Cygnus eCos Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License.  You may obtain a copy of the License at
# http://sourceware.cygnus.com/ecos
# 
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the
# License for the specific language governing rights and limitations under
# the License.
# 
# The Original Code is eCos - Embedded Cygnus Operating System, released
# September 30, 1998.
# 
# The Initial Developer of the Original Code is Cygnus.  Portions created
# by Cygnus are Copyright (C) 1998,1999 Cygnus Solutions.  All Rights Reserved.
# -------------------------------------------
#
#####COPYRIGHTEND####
#==============================================================================

PACKAGE       := hal_mn10300_am31
include ../../../../../pkgconf/pkgconf.mak

LIBRARY       := libtarget.a
COMPILE       := var_misc.c
OTHER_OBJS    :=
OTHER_TARGETS := ldscript.stamp
OTHER_CLEAN   := ldscript.clean
OTHER_DEPS    := ldscript.d

include $(COMPONENT_REPOSITORY)/pkgconf/makrules.src

.PHONY: ldscript.clean

EXTRAS = $(wildcard $(PREFIX)/lib/libextras.a)

ldscript.stamp: mn10300_am31.ld $(EXTRAS)
ifneq ($(EXTRAS),)
	$(LD) -EL --whole-archive $(PREFIX)/lib/libextras.a -r -o $(PREFIX)/lib/extras.o
	$(CC) -E -P -Wp,-MD,ldscript.tmp -DEXTRAS=$(EXTRAS) -xc $(INCLUDE_PATH) $(CFLAGS) -o $(PREFIX)/lib/target.ld $<
else
	$(CC) -E -P -Wp,-MD,ldscript.tmp -xc $(INCLUDE_PATH) $(CFLAGS) -o $(PREFIX)/lib/target.ld $<
endif
	@echo > ldscript.d
	@echo $@ ':' $< '\' >> ldscript.d
	@tail -n +2 ldscript.tmp >> ldscript.d
	@rm ldscript.tmp
	$(TOUCH) $@

ldscript.clean:
	$(RM) -f $(PREFIX)/lib/target.ld
	$(RM) -f ldscript.stamp
