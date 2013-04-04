#pragma once
#include "orzlist.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef	__cplusplus
extern "C" {
#endif
	

/*

	{
		char* ou;
		orzsb* sb = orzsb_create();
		orzsb_puts(sb, "this is orzsb");
		orzsb_puts(sb, "lalalalalalaa");
		orzsb_puts(sb, "233");
		ou = orzsb_build(sb);
		orzsb_clean(sb);
		free(sb);
		printf(ou);
		free(ou);
		return 0;
	}
	
*/
typedef struct orzsb orzsb;
struct orzsb {
	orzlist* list;
	int length;
};

orzsb* orzsb_create();
void orzsb_write(orzsb* o, void* s, int length);
void orzsb_print(orzsb* o, char* s);
void orzsb_puts(orzsb* o, char* s);
void orzsb_printf(orzsb* o, const char * format, ...);
char* orzsb_build(orzsb* o);
void orzsb_clean(orzsb* o);

typedef void (*iterate_str_func)(char *, void *);
void iterate_str(char *start, iterate_str_func f, void * opaque);
char* addslashes(char *s);
char* addslash(char *s);

#ifdef	__cplusplus
}
#endif