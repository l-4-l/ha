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
	HA PC/bcc specific include file
***********************************************************************/

#include <dos.h>
#include <io.h>
#include <dir.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

#define BETA "á"
typedef int S16B;
typedef unsigned U16B;
typedef long S32B;
typedef unsigned long U32B;

#define EXAMPLE "\n examples : ha a21r foo \\bar\\*.* , ha l foo , ha xy foo" 
#define ALLFILES "*.*"
#define F_32B "ld"
#define FX_32B "lX"

#define AO_FLAGS (O_RDWR|O_BINARY)
#define AO_RDOFLAGS (O_RDONLY|O_BINARY)
#define AC_FLAGS O_RDWR|O_BINARY|O_CREAT,S_IREAD|S_IWRITE
#define DEF_FILEATTR S_IREAD|S_IWRITE
#define DEF_DIRATTR  DEF_FILEATTR

#define F_OK	0
#define STDOUT_FILENO fileno(stdout)

#define mkdir(a,b) mkdir(a)
#define md_curfilesize() filestat.st_size
#define md_curfiletime() filestat.st_mtime

#define md_special(a,b)       0   /* Not needed in msdos */

extern struct stat filestat;

void md_init(void);	
char *md_strcase(char *s);
char *md_arcname(char *name_req);
void md_gethdr(int len, int mode);
void md_puthdr(void);
int md_newfile(void);
int md_mkspecial(char *ofname,unsigned sdlen,unsigned char *sdata);
int md_filetype(char *path,char *name);
void md_listhdr(void);
void md_listdat(void);
char *md_timestring(unsigned long t);
void md_truncfile(int fh, U32B len);
char *md_tohapath(char *mdpath);
char *md_tomdpath(char *hapath);
char *md_strippath(char *mdfullpath);
char *md_stripname(char *mdfullpath);
char *md_pconcat(int delim, char *head, char *tail);
int md_namecmp(char *pat, char *cmp);
void md_setft(char *file,U32B time);
void md_setfattrs(char *file);
U32B md_systime(void);













