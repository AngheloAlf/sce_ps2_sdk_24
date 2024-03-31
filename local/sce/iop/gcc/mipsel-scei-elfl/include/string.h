/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5
 */
/* $Id: string.h,v 1.4 2000/10/02 09:03:00 tei Exp $ */

/*
 *                     I/O Processor System Services
 *
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                         string.h
 *                         string functions pseudo definition header 
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       1.00           1999/10/12      tei
 *       1.01           2000/10/02      tei         ANSI compatible
 */

#ifndef	_STRING_H
#define	_STRING_H

#ifndef _TYPES_H
#include <sys/types.h>
#endif
#include <memory.h>

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif
extern char  *strcat (char *dest,     const char *src);
extern char  *strncat(char *dest,     const char *src, size_t n);
extern int    strcmp (const char *s1, const char *s2);
extern int    strncmp(const char *s1, const char *s2,  size_t n);
extern char  *strcpy (char *dest,     const char *src);
extern char  *strncpy(char *dest,     const char *src, size_t n);
extern size_t strlen (const char *s);
extern char  *index  (const char *s,  int c);
extern char  *rindex (const char *s,  int c);

extern char  *strchr (const char *s,  int c);
extern char  *strrchr(const char *s,  int c);
extern char  *strpbrk(const char *s1, const char *s2);
extern int    strspn (const char *s1, const char *s2);
extern int    strcspn(const char *s1, const char *s2);
extern char  *strtok (char *s1,       const char *s2);
extern char  *strstr (const char *s1, const char *s2);
#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif

#endif	/* _STRING_H */

