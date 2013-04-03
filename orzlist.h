#pragma once
#include "stdarg.h"
#ifdef	__cplusplus
extern "C" {
#endif

typedef struct orzlist orzlist;
struct orzlist {
	void* k;
	void* v;
	orzlist* next;
};

orzlist* orzlist_create();
orzlist* orzlist_prepend(orzlist* o, void* k, void* v);
void* orzlist_get(orzlist* o, void* k);
void orzlist_remove(orzlist* o, void* k);
void orzlist_clean(orzlist* o);
orzlist* orzlist_reverse(orzlist* o);

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

#ifdef	__cplusplus
}
#endif