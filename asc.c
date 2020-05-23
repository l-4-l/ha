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
	HA ASC method
***********************************************************************/

#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include "ha.h"
#include "haio.h"
#include "asc.h"
#include "swdict.h"
#include "acoder.h"
#include "error.h"

#define POSCODES 31200
#define SLCODES	16
#define LLCODES 48
#define LLLEN	16
#define LLBITS	4
#define LLMASK	(LLLEN-1)
#define LENCODES (SLCODES+LLCODES*LLLEN)
#define LTCODES (SLCODES+LLCODES)
#define CTCODES 256
#define PTCODES 16
#define LTSTEP 8
#define MAXLT (750*LTSTEP)
#define CTSTEP 1
#define MAXCT (1000*CTSTEP)
#define PTSTEP 24
#define MAXPT (250*PTSTEP) 
#define TTSTEP 40
#define MAXTT (150*TTSTEP)
#define TTORD 4
#define TTOMASK (TTORD-1);
#define LCUTOFF (3*LTSTEP)
#define CCUTOFF (3*CTSTEP)
#define CPLEN 8
#define LPLEN 4
#define MINLENLIM 4096

static U16B ltab[2*LTCODES];
static U16B eltab[2*LTCODES];
static U16B ptab[2*PTCODES];
static U16B ctab[2*CTCODES];
static U16B ectab[2*CTCODES];
static U16B ttab[TTORD][2];
static U16B ccnt,pmax,npt;
static U16B ces;
static U16B les;
static U16B ttcon;

void asc_cleanup(void) {
    
    swd_cleanup();
}

static void tabinit(U16B t[], U16B tl, U16B ival) {

    register U16B i,j;
    
    for (i=tl;i<2*tl;++i) t[i]=ival;
    for (i=tl-1,j=(tl<<1)-2;i;--i,j-=2) {
	t[i]=t[j]+t[j+1];
    }
}

static void tscale(U16B t[], U16B tl) {

    register U16B i,j;
    
    for (i=(tl<<1)-1;i>=tl;--i) {
	if (t[i]>1) t[i]>>=1;
    }
    for (i=tl-1,j=(tl<<1)-2;i;--i,j-=2) {
	t[i]=t[j]+t[j+1];
    }
}

static void tupd(U16B t[], U16B tl, U16B maxt, U16B step, U16B p) {

    register S16B i;
    
    for (i=p+tl;i;i>>=1) t[i]+=step;
    if (t[1]>=maxt) tscale(t,tl);
}

static void tzero(U16B t[], U16B tl, U16B p) {

    register S16B i,step;
    
    for (i=p+tl,step=t[i];i;i>>=1) t[i]-=step;
}

static void model_init(void) {

    register S16B i;
    
    ces=CTSTEP;
    les=LTSTEP;
    ccnt=0;
    ttcon=0;
    npt=pmax=1;
    for (i=0;i<TTORD;++i) ttab[i][0]=ttab[i][1]=TTSTEP;
    tabinit(ltab,LTCODES,0);
    tabinit(eltab,LTCODES,1);
    tabinit(ctab,CTCODES,0);
    tabinit(ectab,CTCODES,1);
    tabinit(ptab,PTCODES,0);	
    tupd(ptab,PTCODES,MAXPT,PTSTEP,0);
}

static void pack_init(void) {
	
    model_init();
    ac_init_encode();
}

static void unpack_init(void) {

    model_init();
    ac_init_decode();
}

static void ttscale(U16B con) {

    ttab[con][0]>>=1;
    if (ttab[con][0]==0) ttab[con][0]=1;
    ttab[con][1]>>=1;
    if (ttab[con][1]==0) ttab[con][1]=1;
}

static void codepair(S16B l, S16B p) {

    register U16B i,j,lt,k,cf,tot;
    
    i=ttab[ttcon][0]+ttab[ttcon][1];
    ac_out(ttab[ttcon][0],i,i+1);
    ttab[ttcon][1]+=TTSTEP;
    if (i>=MAXTT) ttscale(ttcon);
    ttcon=((ttcon<<1)|1)&TTOMASK;
    while (ccnt>pmax) {
	tupd(ptab,PTCODES,MAXPT,PTSTEP,npt++);
	pmax<<=1;	
    }

    for (i=p,j=0;i;++j,i>>=1); 
    cf=ptab[PTCODES+j];
    tot=ptab[1];
    for (lt=0,i=PTCODES+j;i;i>>=1) {
	if (i&1) lt+=ptab[i-1];
	ptab[i]+=PTSTEP;
    }
    if (ptab[1]>=MAXPT) tscale(ptab,PTCODES);
    ac_out(lt,lt+cf,tot);
    if (p>1) {
	for (i=0x8000U;!(p&i);i>>=1);
	j=p&~i;
	if (i!=(pmax>>1)) ac_out(j,j+1,i);
	else ac_out(j,j+1,ccnt-(pmax>>1));
    }
    i=l-MINLEN;
    if (i==LENCODES-1) i=SLCODES-1,j=0xffff;
    else if (i<SLCODES-1) j=0xffff; 
    else {
	j=(i-SLCODES+1)&LLMASK;
	i=((i-SLCODES+1)>>LLBITS)+SLCODES;	
    }
    if ((cf=ltab[LTCODES+i])==0) {
	ac_out(ltab[1],ltab[1]+les,ltab[1]+les);	
	for (lt=0,k=LTCODES+i;k;k>>=1) {
	    if (k&1) lt+=eltab[k-1];
	    ltab[k]+=LTSTEP;
	}
	if (ltab[1]>=MAXLT) tscale(ltab,LTCODES);
	ac_out(lt,lt+eltab[LTCODES+i],eltab[1]);
	tzero(eltab,LTCODES,i);
	if (eltab[1]!=0) les+=LTSTEP;
	else les=0;
	for (k=i<=LPLEN?0:i-LPLEN;
	     k<(i+LPLEN>=LTCODES-1?LTCODES-1:i+LPLEN);++k) {
	    if (eltab[LTCODES+k]) tupd(eltab,LTCODES,MAXLT,1,k);
	}
    }
    else {
	tot=ltab[1]+les;
	for (lt=0,k=LTCODES+i;k;k>>=1) {
	    if (k&1) lt+=ltab[k-1];
	    ltab[k]+=LTSTEP;
	}
	if (ltab[1]>=MAXLT) tscale(ltab,LTCODES);
	ac_out(lt,lt+cf,tot);
    }
    if (ltab[LTCODES+i]==LCUTOFF) les-=LTSTEP<les?LTSTEP:les-1; 
    if (j!=0xffff) ac_out(j,j+1,LLLEN); 
    if (ccnt<POSCODES) {
	ccnt+=l;
	if (ccnt>POSCODES) ccnt=POSCODES;
    }
}


static void codechar(S16B c) {

    register U16B i,lt,tot,cf;

    i=ttab[ttcon][0]+ttab[ttcon][1];
    ac_out(0,ttab[ttcon][0],i+1);
    ttab[ttcon][0]+=TTSTEP;
    if (i>=MAXTT) ttscale(ttcon);
    ttcon=(ttcon<<1)&TTOMASK;
    if ((cf=ctab[CTCODES+c])==0) {
	ac_out(ctab[1],ctab[1]+ces,ctab[1]+ces);
	for (lt=0,i=CTCODES+c;i;i>>=1) {
	    if (i&1) lt+=ectab[i-1];
	    ctab[i]+=CTSTEP;
	}
	if (ctab[1]>=MAXCT) tscale(ctab,CTCODES);
	ac_out(lt,lt+ectab[CTCODES+c],ectab[1]);
	tzero(ectab,CTCODES,c);
	if (ectab[1]!=0) ces+=CTSTEP;
	else ces=0;
	for (i=c<=CPLEN?0:c-CPLEN;
	     i<(c+CPLEN>=CTCODES-1?CTCODES-1:c+CPLEN);++i) {
	    if (ectab[CTCODES+i]) tupd(ectab,CTCODES,MAXCT,1,i);
	}
    }
    else {
	tot=ctab[1]+ces;
	for (lt=0,i=CTCODES+c;i;i>>=1) {
	    if (i&1) lt+=ctab[i-1];
	    ctab[i]+=CTSTEP;
	}
	if (ctab[1]>=MAXCT) tscale(ctab,CTCODES);
	ac_out(lt,lt+cf,tot);
    }
    if (ctab[CTCODES+c]==CCUTOFF) ces-=CTSTEP<ces?CTSTEP:ces-1; 
    if (ccnt<POSCODES) ++ccnt;
}


void asc_pack(void) {
    
    S16B oc;
    U16B omlf,obpos;
    
    swd_init(LENCODES+MINLEN-1,POSCODES);
    pack_init();
    for (swd_findbest();swd_char>=0;) {
	if (swd_mlf>MINLEN || (swd_mlf==MINLEN && swd_bpos<MINLENLIM)) {
	    omlf=swd_mlf;
	    obpos=swd_bpos;
	    oc=swd_char;
	    swd_findbest();
	    if (swd_mlf>omlf) codechar(oc);
	    else {
		swd_accept();
		codepair(omlf,obpos);
		swd_findbest();
	    }
	}
	else {
	    swd_mlf=MINLEN-1;
	    codechar(swd_char);
	    swd_findbest();
	}
    }
    ac_out(ttab[ttcon][0]+ttab[ttcon][1],
	   ttab[ttcon][0]+ttab[ttcon][1]+1,ttab[ttcon][0]+ttab[ttcon][1]+1);
    ac_end_encode();
    asc_cleanup();
}


void asc_unpack(void) {
    
    register U16B l,p,tv,i,lt;
    
    swd_dinit(POSCODES);
    unpack_init();
    for (;;) {
	tv=ac_threshold_val(ttab[ttcon][0]+ttab[ttcon][1]+1);		
	i=ttab[ttcon][0]+ttab[ttcon][1];
	if (ttab[ttcon][0]>tv) {
	    ac_in(0,ttab[ttcon][0],i+1);
	    ttab[ttcon][0]+=TTSTEP;
	    if (i>=MAXTT) ttscale(ttcon);
	    ttcon=(ttcon<<1)&TTOMASK;
	    tv=ac_threshold_val(ctab[1]+ces);
	    if (tv>=ctab[1]) {
		ac_in(ctab[1],ctab[1]+ces,ctab[1]+ces);
		tv=ac_threshold_val(ectab[1]);
		for (l=2,lt=0;;) {
		    if (lt+ectab[l]<=tv) {
			lt+=ectab[l];
			++l;
		    }
		    if (l>=CTCODES) {
			l-=CTCODES;
			break;
		    }
		    l<<=1;
		}
		ac_in(lt,lt+ectab[CTCODES+l],ectab[1]);
		tzero(ectab,CTCODES,l);
		if (ectab[1]!=0) ces+=CTSTEP;
		else ces=0;
		for (i=l<CPLEN?0:l-CPLEN;
		     i<(l+CPLEN>=CTCODES-1?CTCODES-1:l+CPLEN);++i) {
		    if (ectab[CTCODES+i]) tupd(ectab,CTCODES,MAXCT,1,i);
		}
	    }
	    else {
		for (l=2,lt=0;;) {
		    if (lt+ctab[l]<=tv) {
			lt+=ctab[l];
			l++;
		    }
		    if (l>=CTCODES) {
			l-=CTCODES;
			break;
		    }
		    l<<=1;
		}
		ac_in(lt,lt+ctab[CTCODES+l],ctab[1]+ces);
	    }
	    tupd(ctab,CTCODES,MAXCT,CTSTEP,l);
	    if (ctab[CTCODES+l]==CCUTOFF) ces-=CTSTEP<ces?CTSTEP:ces-1; 
	    swd_dchar(l);
	    if (ccnt<POSCODES) ++ccnt;
	}
	else if (i>tv) {
	    ac_in(ttab[ttcon][0],i,i+1);
	    ttab[ttcon][1]+=TTSTEP;
	    if (i>=MAXTT) ttscale(ttcon);
	    ttcon=((ttcon<<1)|1)&TTOMASK;
	    while (ccnt>pmax) {
		tupd(ptab,PTCODES,MAXPT,PTSTEP,npt++);
		pmax<<=1;	
	    }
	    tv=ac_threshold_val(ptab[1]);
	    for (p=2,lt=0;;) {
		if (lt+ptab[p]<=tv) {
		    lt+=ptab[p];
		    p++;
		}
		if (p>=PTCODES) {
		    p-=PTCODES;
		    break;
		}
		p<<=1;
	    }
	    ac_in(lt,lt+ptab[PTCODES+p],ptab[1]);
	    tupd(ptab,PTCODES,MAXPT,PTSTEP,p);
	    if (p>1) {
		for (i=1;p;i<<=1,--p);
		i>>=1;
		if (i==(pmax>>1)) l=ccnt-(pmax>>1);
		else l=i;
		p=ac_threshold_val(l);
		ac_in(p,p+1,l);
		p+=i;
	    }
	    tv=ac_threshold_val(ltab[1]+les);
	    if (tv>=ltab[1]) {
		ac_in(ltab[1],ltab[1]+les,ltab[1]+les);
		tv=ac_threshold_val(eltab[1]);
		for (l=2,lt=0;;) {
		    if (lt+eltab[l]<=tv) {
			lt+=eltab[l];
			++l;
		    }
		    if (l>=LTCODES) {
			l-=LTCODES;
			break;
		    }
		    l<<=1;
		}
		ac_in(lt,lt+eltab[LTCODES+l],eltab[1]);
		tzero(eltab,LTCODES,l);
		if (eltab[1]!=0) les+=LTSTEP;
		else les=0;
		for (i=l<LPLEN?0:l-LPLEN;
		     i<(l+LPLEN>=LTCODES-1?LTCODES-1:l+LPLEN);++i) {
		    if (eltab[LTCODES+i]) tupd(eltab,LTCODES,MAXLT,1,i);
		}
	    }
	    else {
		for (l=2,lt=0;;) {
		    if (lt+ltab[l]<=tv) {
			lt+=ltab[l];
			++l;
		    }
		    if (l>=LTCODES) {
			l-=LTCODES;
			break;
		    }
		    l<<=1;
		}
		ac_in(lt,lt+ltab[LTCODES+l],ltab[1]+les);
	    }
	    tupd(ltab,LTCODES,MAXLT,LTSTEP,l);
	    if (ltab[LTCODES+l]==LCUTOFF) les-=LTSTEP<les?LTSTEP:les-1; 
	    if (l==SLCODES-1) l=LENCODES-1;
	    else if (l>=SLCODES) {
		i=ac_threshold_val(LLLEN);
		ac_in(i,i+1,LLLEN);
		l=((l-SLCODES)<<LLBITS)+i+SLCODES-1;
	    }
	    l+=3;
	    if (ccnt<POSCODES) {
		ccnt+=l;
		if (ccnt>POSCODES) ccnt=POSCODES;
	    }
	    swd_dpair(l,p);				
	}
	else {
	    ac_in(i,i+1,i+1);
	    flush();
	    asc_cleanup();
	    return;
	}
    }
}
 

