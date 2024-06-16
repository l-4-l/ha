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

OBJS = ha.o haio.o machine.o archive.o acoder.o swdict.o \
	cpy.o error.o misc.o hsc.o asc.o info.o

all: $(OBJS)
ha: all
	$(CC) $(LDFLAGS) -o ha $(OBJS)
clean:
	rm -f *.o ./ha
