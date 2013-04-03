#pragma once
#include <stdio.h>
#ifdef	__cplusplus
extern "C" {
#endif
typedef void (*iterate_str_func)(char *, void *);
void iterate_str(char *start, iterate_str_func f, void * opaque);
char* addslashes(char *s);
char* addslash(char *s);

#ifdef	__cplusplus
}
#endif