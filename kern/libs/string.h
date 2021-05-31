#ifndef __KERN_LIBS_STRING_H__
#define __KERN_LIBS_STRING_H__

#include "libs/types.h"

int             memcmp(const void*, const void*, uint);
void*           memmove(void*, const void*, uint);
void*           memset(void*, int, uint);
void*           memcpy(void *dst,const void *src, uint);
char*           safestrcpy(char*, const char*, int);
int             strlen(const char*);
int             strncmp(const char*, const char*, uint);
char*           strncpy(char*, const char*, int);
int             strncasecmp(const char * s1, const char * s2, size_t n);
char*           strchr(const char *s, char c);
char*           strrchr(const char *s, char c);
void            wnstr(wchar *dst, char const *src, int len);
void            snstr(char *dst, wchar const *src, int len);
int             wcsncmp(wchar const *s1, wchar const *s2, int len);


#endif