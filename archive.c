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
	HA archive handling
***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "error.h"
#include "ha.h"
#include "archive.h"
#include "haio.h"

#define STRING	32

int arcfile=-1;
char *arcname=NULL;
struct stat arcstat;
static unsigned arccnt=0;
static int dirty=0,addtries;
static U32B nextheader=4,thisheader,arcsize,bestpos,trypos;
static Fheader newhdr;

static U32B getvalue(int len) {

    unsigned char buf[4];
    U32B val;
    int i;
    
    if (read(arcfile,buf,len)!=len) error(1,ERR_READ,arcname);
    for (val=i=0;i<len;++i) val|=(U32B)buf[i]<<(i<<3);
    return val;
}

static void putvalue(U32B val, int len) {

    unsigned char buf[4];
    int i;
    
    for (i=0;i<len;++i,val>>=8) buf[i]=(unsigned char) val&0xff;
    if (write(arcfile,&buf,len)!=len) error(1,ERR_WRITE,arcname);
}

static char *getstring(void) {
    
    char *sptr;
    int offset;
    
    if ((sptr=malloc(STRING))==NULL) error(1,ERR_MEM,"getstring()");
    for (offset=0;;offset++) {
	if (read(arcfile,sptr+offset,1)!=1) error(1,ERR_READ,arcname);
	if (sptr[offset]==0) break;
	if ((offset&(STRING-1))==0) {
	    if ((sptr=realloc(sptr,STRING))==NULL) 
	      error(1,ERR_MEM,"getstring()");
	}
    }
    return sptr;
}

static void putstring(char *string) {
    
    int len;

    len=strlen(string)+1;
    if (write(arcfile,string,len)!=len) error(1,ERR_WRITE,arcname);
}

static Fheader *getheader(void) {

    static Fheader hd={0,0,0,0,0,0,NULL,NULL,0};

    if ((hd.ver=getvalue(1))!=0xff) {
	hd.type=hd.ver&0xf;
	hd.ver>>=4;
	if (hd.ver>MYVER) error(1,ERR_TOONEW);
	if (hd.ver<LOWVER) error(1,ERR_TOOOLD);
	if (hd.type!=M_SPECIAL && hd.type!=M_DIR && hd.type>=M_UNK) 
	  error(1,ERR_UNKMET,hd.type);
    }
    hd.clen=getvalue(4);
    hd.olen=getvalue(4);
    hd.crc=getvalue(4);
    hd.time=getvalue(4);
    if (hd.path!=NULL) free(hd.path);
    hd.path=getstring();
    if (hd.name!=NULL) free(hd.name);
    hd.name=getstring();
    hd.mdilen=(unsigned)getvalue(1);
    hd.mylen=hd.mdilen+20+strlen(hd.path)+strlen(hd.name);
    md_gethdr(hd.mdilen,hd.type);
    return &hd;
}

static void putheader(Fheader *hd) {

    putvalue((hd->ver<<4)|hd->type,1);
    putvalue(hd->clen,4); 
    putvalue(hd->olen,4); 
    putvalue(hd->crc,4); 
    putvalue(hd->time,4); 
    putstring(hd->path);
    putstring(hd->name);
    putvalue(hd->mdilen,1);
    md_puthdr();
}	

static void arc_clean(void) {

    U32B ipos,opos,cpylen;
    int len;
    unsigned cnt;
    Fheader *hd;
    
    ipos=opos=4;
    for (cnt=arccnt;cnt;--cnt) {
	if (lseek(arcfile,ipos,SEEK_SET)<0) error(1,ERR_SEEK,"arc_clean()");
	for (;;) {
	    hd=getheader();
	    if (hd->ver!=0xff) break;
	    ipos+=hd->clen+hd->mylen;
	    if (lseek(arcfile,ipos,SEEK_SET)<0) 
	      error(1,ERR_SEEK,"arc_clean()");
	} 
	if (ipos==opos) ipos=opos+=hd->clen+hd->mylen;
	else {
	    cpylen=hd->clen+hd->mylen;
	    while (cpylen) {
		if (lseek(arcfile,ipos,SEEK_SET)<0) 
		  error(1,ERR_SEEK,"arc_clean()");
		len=read(arcfile,ib,BLOCKLEN>cpylen?(unsigned)cpylen:BLOCKLEN);
		if (len<=0) error(1,ERR_READ,arcname);
		cpylen-=len;
		ipos+=len;
		if (lseek(arcfile,opos,SEEK_SET)<0) 
		  error(1,ERR_SEEK,"arc_clean()");
		if (write(arcfile,ib,len)!=len) error(1,ERR_WRITE,arcname);
		opos+=len;
	    }
	}
    }
    md_truncfile(arcfile,opos);
}

void arc_close(void) {
    
    if (arcfile>=0) {
	if (dirty) arc_clean();
	close(arcfile);
	if (!arccnt) {
	    if (remove(arcname)) error(1,ERR_REMOVE,arcname);
	}
    }
}

static U32B arc_scan(void) {
    
    U32B pos;
    unsigned i;
    Fheader *hd;
    
    pos=4;
    for (i=0;i<arccnt;++i) {
	if (pos>=arcsize) {
	    error(0,ERR_CORRUPTED);
	    arccnt=i;
	    return pos;
	}
	if (lseek(arcfile,pos,SEEK_SET)<0) error(1,ERR_SEEK,"arc_seek()");
	hd=getheader();
	pos+=hd->clen+hd->mylen;
	if (hd->ver==0xff) {
	    dirty=1;
	    --i;
	}
    }
    if (pos!=arcsize) dirty=1;
    return pos;
}

void arc_open(char *aname,int mode) {
    
    char id[2];
    
    dirty=0;
    arcname=md_arcname(aname);
    if ((arcfile=open(arcname,(mode&ARC_RDO)?AO_RDOFLAGS:AO_FLAGS))>=0) {
	if (fstat(arcfile,&arcstat)!=0) error(1,ERR_STAT,arcname);
	arcsize=arcstat.st_size;
	if (read(arcfile,id,2)!=2 || id[0]!='H' || id[1]!='A') {
	    error(1,ERR_NOHA,arcname);
	}
	arccnt=(unsigned)getvalue(2);
	arcsize=arc_scan();
	if (!quiet) printf("\nArchive : %s (%d files)\n",arcname,arccnt);
    }
    else if ((mode&ARC_NEW) && (arcfile=open(arcname,AC_FLAGS))>=0) {
	if (fstat(arcfile,&arcstat)!=0) error(1,ERR_STAT,arcname);
	if (!quiet) printf("\nNew archive : %s\n",arcname);
	if (write(arcfile,"HA\000",4)!=4) error(1,ERR_WRITE,arcname);
	arccnt=0;
	arcsize=4;
    }
    else error(1,ERR_ARCOPEN,arcname);
    cu_add(CU_FUNC,arc_close);
}

void arc_reset(void) {			

    nextheader=4;
}

Fheader *arc_seek(void) {

    static Fheader *hd;
    
    for (;;) {
	if (nextheader>=arcsize) return NULL;
	if (lseek(arcfile,nextheader,SEEK_SET)<0) 
	  error(1,ERR_SEEK,"arc_seek()");
	hd=getheader();
	thisheader=nextheader;
	nextheader+=hd->clen+hd->mylen;
	if (hd->ver==0xff) dirty=1;
	else if (match(hd->path,hd->name)) return hd;
    }
}

void arc_delete(void) {
    
    if (lseek(arcfile,thisheader,SEEK_SET)<0) 
      error(1,ERR_SEEK,"arc_delete()");	
    if (write(arcfile,"\xff",1)!=1) error(1,ERR_WRITE,arcname);
    if (lseek(arcfile,2,SEEK_SET)<0) error(1,ERR_SEEK,"arc_delete()");	
    putvalue(--arccnt,2);	
    dirty=1;
}

void arc_newfile(char *mdpath, char *name) {
    
    newhdr.ver=MYVER;
    newhdr.olen=md_curfilesize();
    newhdr.time=md_curfiletime();
    newhdr.path=md_tohapath(mdpath);
    newhdr.name=name;
    newhdr.mdilen=md_newfile();
    newhdr.mylen=newhdr.mdilen+20+strlen(newhdr.path)+strlen(newhdr.name);
    bestpos=trypos=arcsize+newhdr.mylen;
    addtries=0;
    dirty|=2;
}

void arc_accept(int method) {

    bestpos=trypos;
    newhdr.type=method;
    trypos+=newhdr.clen=ocnt;
    newhdr.crc=getcrc();
}

void arc_trynext(void) {

    if (lseek(arcfile,trypos,SEEK_SET)<0) error(1,ERR_SEEK,"arc_trynext()");
    if (addtries++) dirty=1;
}

static void delold(void) {

    U32B pos,oldpos;
    unsigned i;
    Fheader *hd;
    
    pos=4;
    for (i=arccnt;i>0;--i) {
	if (pos>=arcsize) break;
	if (lseek(arcfile,pos,SEEK_SET)<0) error(1,ERR_SEEK,"delold()");
	hd=getheader();
	oldpos=pos;
	pos+=hd->clen+hd->mylen;
	if (hd->ver==0xff) {
	    dirty=1;
	    ++i;
	}
	else {
	    if (!strcmp(md_strcase(hd->path),newhdr.path) && 
		!strcmp(md_strcase(hd->name),newhdr.name)) {
		if (lseek(arcfile,oldpos,SEEK_SET)<0) 
		  error(1,ERR_SEEK,"delold()");
		if (write(arcfile,"\xff",1)!=1) error(1,ERR_WRITE,arcname);
		dirty=1;
		--arccnt;
	    }
	}
    }
}

int arc_addfile(void) {
    
    U32B basepos,len;
    int cplen;
    
    if ((basepos=arcsize+newhdr.mylen)!=bestpos) {
	if (lseek(arcfile,basepos,SEEK_SET)<0) 
	  error(1,ERR_SEEK,"arc_addfile()");
	len=newhdr.clen;
	while (len) {
	    if (lseek(arcfile,bestpos,SEEK_SET)<0) 
	      error(1,ERR_SEEK,"arc_addfile()");
	    cplen=BLOCKLEN>len?(int)len:BLOCKLEN;
	    if (read(arcfile,ib,cplen)!=cplen) error(1,ERR_READ,arcname);
	    len-=cplen;
	    bestpos+=cplen;
	    if (lseek(arcfile,basepos,SEEK_SET)<0) 
	      error(1,ERR_SEEK,"arc_addfile()");
	    if (write(arcfile,ib,cplen)!=cplen) error(1,ERR_WRITE,arcname);
	    basepos+=cplen;
	}
    }
    if (lseek(arcfile,arcsize,SEEK_SET)<0) error(1,ERR_SEEK,"arc_addfile()");
    putheader(&newhdr);
    dirty&=1;
    delold();
    ++arccnt;
    arcsize+=newhdr.mylen+newhdr.clen;
    if (lseek(arcfile,2,SEEK_SET)<0) error(1,ERR_SEEK,"arc_addfile()");
    putvalue(arccnt,2);
    return 1;
}

int arc_adddir(void) {
    
    newhdr.type=M_DIR;
    newhdr.olen=newhdr.clen=0;
    newhdr.crc=0;
    
    if (lseek(arcfile,arcsize,SEEK_SET)<0) error(1,ERR_SEEK,"arc_adddir()");
    putheader(&newhdr);
    dirty&=1;
    delold();
    ++arccnt;
    arcsize+=newhdr.mylen;
    if (lseek(arcfile,2,SEEK_SET)<0) error(1,ERR_SEEK,"arc_adddir()");
    putvalue(arccnt,2);
    return 1;
}

int arc_addspecial(char *fullname) {

    unsigned char *sdata;
    
    newhdr.type=M_SPECIAL;
    newhdr.olen=newhdr.clen=md_special(fullname, &sdata);
    newhdr.crc=0;
    
    if (lseek(arcfile,arcsize,SEEK_SET)<0) 
      error(1,ERR_SEEK,"arc_addspecial()");
    putheader(&newhdr);
    if (newhdr.clen!=0) {
	if (lseek(arcfile,arcsize+newhdr.mylen,SEEK_SET)<0) 
	  error(1,ERR_SEEK,"arc_addspecial()");
	if (write(arcfile,sdata,newhdr.clen)!=newhdr.clen) 
	  error(1,ERR_WRITE,arcname);
    }
    dirty&=1;
    delold();
    ++arccnt;
    arcsize+=newhdr.mylen+newhdr.clen;
    if (lseek(arcfile,2,SEEK_SET)<0) error(1,ERR_SEEK,"arc_addspecial()");
    putvalue(arccnt,2);
    return 1;
}
























