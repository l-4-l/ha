############################################################################
#  This file is part of HA, a general purpose file archiver.
#  Copyright (C) 1995 Harri Hirvola
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
############################################################################
#	Makefile for HA/gcc
############################################################################

MCH = nix

CC = gcc
CPP = $(CC) -E
CFLAGS = -Wall -O2 -c -I../$(MCH) -I../include
LDFLAGS = -O2

MDEFINES = "CC=$(CC)" "CPP=$(CPP)" "CFLAGS=$(CFLAGS)" "DEFS=$(DEFS)"
SUBDIRS = c $(MCH) include
OBJS = c/*.o $(MCH)/*.o

ha : subdirs 
	$(CC) $(LDFLAGS) -o ha $(OBJS)

subdirs: 
	@for i in $(SUBDIRS); do (cd $$i && echo $$i && $(MAKE) $(MDEFINES)) || exit; done

depend dep:
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE) $(MDEFINES) dep) || exit; done

clean: 
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE) clean) || exit; done
	rm -f .depend ha *.out *~

#
# include a dependency file if one exists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif
