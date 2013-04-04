#pragma once
#include <stdlib.h>
#include <stdarg.h>
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

#ifdef	__cplusplus
}
#endif