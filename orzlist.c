#pragma once
#include "orzlist.h"
#ifdef	__cplusplus
extern "C" {
#endif

orzlist* orzlist_create()
{
	orzlist* l = (orzlist*)calloc(sizeof(orzlist), 1);
	l->k = 0;
	l->v = 0;
	l->next = l;
	return l;
}

orzlist* orzlist_prepend(orzlist* o, void* k, void* v)
{
	orzlist* l = (orzlist*)calloc(sizeof(orzlist), 1);
	l->k = k;
	l->v = v;
	l->next = o->next;
	o->next = l;
	return o;
}

void* orzlist_get(orzlist* o, void* k)
{
	orzlist* l = o;
	if (l->next != o)
		while (l = l->next)
		{
			if (l->k == k)
				return l->v;
			
			if (l->next == o)
				break;
		}
	return 0;
}

void orzlist_remove(orzlist* o, void* k)
{
	orzlist* l = o->next;
	
	do
	{
		if (l->next == o)
			break;
		
		if (l->next->k == k)
		{
			orzlist* j = l->next;
			l->next = j->next;
			free(j);
		}
	}
	while (l = l->next);
}

void orzlist_clean(orzlist* o)
{
	orzlist* l = o->next;
	
	do
	{
		if (l->next == o)
			break;
		
		{
			orzlist* j = l->next;
			l->next = j->next;
			free(j);
		}
	}
	while (l = l->next);

	return;
}

orzlist* orzlist_reverse(orzlist* o)
{
	orzlist* n = orzlist_create();
	orzlist* l = o;
	if (l->next != o)
		while (l = l->next)
		{
			orzlist_prepend(n, l->k, l->v);
			
			if (l->next == o)
				break;
		}
	
	return n;
}

#ifdef	__cplusplus
}
#endif