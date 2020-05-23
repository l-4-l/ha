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
	HA arithmetic coder
***********************************************************************/

/***********************************************************************
This file contains some small changes made by Nico de Vries (AIP-NL)
allowing it to be compiled with Borland C++ 3.1.
***********************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include "ha.h"
#include "haio.h"
#include "acoder.h"

static U16B h,l,v;
static S16B s;
static S16B gpat,ppat;

/***********************************************************************
  Bit I/O
***********************************************************************/

#define putbit(b) 	{ ppat<<=1;				\
			  if (b) ppat|=1;			\
			  if (ppat&0x100) {			\
				putbyte(ppat&0xff);		\
				ppat=1;				\
			  }					\
			}


#define getbit(b) 	{ gpat<<=1;				\
			  if (!(gpat&0xff)) {			\
				gpat=getbyte();			\
				if (gpat&0x100) gpat=0x100;	\
				else {				\
					gpat<<=1;		\
					gpat|=1;		\
				}				\
			  }					\
			  b|=(gpat&0x100)>>8;			\
			}


/***********************************************************************
  Arithmetic encoding
***********************************************************************/

#ifndef __BORLANDC__

void ac_out(U16B low, U16B high, U16B tot) {
    
    register U32B r;
    
    r=(U32B)(h-l)+1;
    h=(U16B)(r*high/tot-1)+l;
    l+=(U16B)(r*low/tot);
    if (!((h^l)&0x8000)) {
	putbit(l&0x8000);
	while(s) {
	    --s;
	    putbit(~l&0x8000);
	}
	l<<=1;
	h<<=1;
	h|=1;
	while (!((h^l)&0x8000)) {
	    putbit(l&0x8000);
	    l<<=1;
	    h<<=1;
	    h|=1;
	}
    }
    while ((l&0x4000)&&!(h&0x4000)) {
	++s;
	l<<=1;
	l&=0x7fff;
	h<<=1;
	h|=0x8001;
    }
}

#else

void ac_out(U16B low, U16B high, U16B tot) {

    asm {
	mov ax,h
	mov bx,l
	sub ax,bx
	add ax,1
	mov cx,ax
        jc  aco0
	mul high
        jmp aco1
    }
aco0:
    asm {
        mov dx,high
    }
aco1:
    asm {
        cmp dx,tot
	je  aco2
        div tot
        sub ax,1
        jmp aco3
    }
aco2:
    asm {
	mov ax,0xffff
    }
aco3:
    asm {
        add ax,bx
	mov h,ax
	mov ax,cx
	cmp ax,0
        je  aco4
        mul low
        jmp aco5
    }
aco4:
    asm {
        mov dx,low
    }
aco5:
    asm {
	div tot
	add l,ax
    }
    if (!((h^l)&0x8000)) {
	putbit(l&0x8000);
	while(s) {
	    --s;
	    putbit(~l&0x8000);
	}
	l<<=1;
	h<<=1;
	h|=1;
	while (!((h^l)&0x8000)) {
	    putbit(l&0x8000);
	    l<<=1;
	    h<<=1;
	    h|=1;
	}
    }
    while ((l&0x4000)&&!(h&0x4000)) {
	++s;
	l<<=1;
	l&=0x7fff;
	h<<=1;
	h|=0x8001;
    }
}

#endif

void ac_init_encode(void) {

    h=0xffff;
    l=s=0;
    ppat=1;
}

void ac_end_encode(void) {

    ++s;
    putbit(l&0x4000);
    while (s--) {
	putbit(~l&0x4000);
    }
    if (ppat==1) {
	flush();
	return;
    }
    while(!(ppat&0x100)) ppat<<=1;
    putbyte(ppat&0xff);
    flush();
}


/***********************************************************************
  Arithmetic decoding
***********************************************************************/

#ifndef __BORLANDC__

void ac_in(U16B low, U16B high, U16B tot) {

    register U32B r;

    r=(U32B)(h-l)+1;
    h=(U16B)(r*high/tot-1)+l;
    l+=(U16B)(r*low/tot);
    while (!((h^l)&0x8000)) {
	l<<=1;
	h<<=1;
	h|=1;
	v<<=1;
	getbit(v);
    }
    while ((l&0x4000)&&!(h&0x4000)) {
	l<<=1;
	l&=0x7fff;
	h<<=1;
	h|=0x8001;
	v<<=1;
	v^=0x8000;
	getbit(v);
    }
}

U16B ac_threshold_val(U16B tot) {
	
    register U32B r;
    
    r=(U32B)(h-l)+1;
    return (U16B)((((U32B)(v-l)+1)*tot-1)/r);
}

#else

void ac_in(U16B low, U16B high, U16B tot) {

    asm {
	mov ax,h
	mov bx,l
	sub ax,bx
	add ax,1
	mov cx,ax
        jc  aci0
	mul high
        jmp aci1
    }
aci0:
    asm {
        mov dx,high
    }
aci1:
    asm {
        cmp dx,tot
	je  aci2
        div tot
        sub ax,1
        jmp aci3
    }
aci2:
    asm {
	mov ax,0xffff
    }
aci3:
    asm {
        add ax,bx
	mov h,ax
	mov ax,cx
	cmp ax,0
        je  aci4
        mul low
        jmp aci5
    }
aci4:
    asm {
        mov dx,low
    }
aci5:
    asm {
	div tot
	add l,ax
    }
    while (!((h^l)&0x8000)) {
	l<<=1;
	h<<=1;
	h|=1;
	v<<=1;
	getbit(v);
    }
    while ((l&0x4000)&&!(h&0x4000)) {
	l<<=1;
	l&=0x7fff;
	h<<=1;
	h|=0x8001;
	v<<=1;
	v^=0x8000;
	getbit(v);
    }
}

U16B ac_threshold_val(U16B tot) {

    U16B rv;

    asm {
	mov ax,h
	sub ax,l
	add ax,1
	jc  atv1
	mov cx,ax
	mov ax,v
	sub ax,l
	inc ax
	mul [tot]
	sub ax,1
	sbb dx,0
	div cx
	mov rv,ax
	jmp atv3
    }
atv1:
    asm {
	mov ax,v
	sub ax,l
	add ax,1
	jc  atv2
	mul [tot]
	sub ax,1
	sbb dx,0
	mov rv,dx
	jmp atv3
    }
atv2:
    asm {
	mov ax,tot
	dec ax
	mov rv,dx
    }
atv3:
    return rv;
}

#endif

void ac_init_decode(void) {

    h=0xffff;
    l=0;
    gpat=0;
    v=getbyte()<<8;
    v|=0xff&getbyte();
}

