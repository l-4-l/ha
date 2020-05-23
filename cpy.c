/***********************************************************************
  This file is part of HA, a general purpose file archiver.
  Copyright (C) 1995 Harri Hirvola

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
************************************************************************
	HA CPY method
***********************************************************************/

#include <stdio.h>
#include <malloc.h>
#include "ha.h"
#include "cpy.h"
#include "haio.h"

void cpy(void) {
    
    register S16B c;
    register U32B cnt;
    
    if ((cnt=totalsize)==0) return;
    while ((c=getbyte())>=0) {
	putbyte(c);
	if (!--cnt) break;
    }
    flush();   
}





