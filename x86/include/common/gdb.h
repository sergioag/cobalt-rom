/*
 * $Id: gdb.h,v 1.1.1.1 2003/06/10 22:42:01 iceblink Exp $
 */
#ifndef COMMON_GDB_H
#define COMMON_GDB_H

#ifdef REMOTE_GDB
void init_debug(void);
void breakpoint(void);
#endif

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
