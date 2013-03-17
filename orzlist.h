#pragma once

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
orzlist* orzlist_remove(orzlist* o, void* k);

#ifdef	__cplusplus
}
#endif