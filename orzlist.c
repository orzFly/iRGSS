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

orzsb* orzsb_create()
{
	orzsb* l = (orzsb*)calloc(sizeof(orzsb), 1);
	l->list = orzlist_create();
	l->length = 0;
	return l;
}

void orzsb_write(orzsb* o, void* s, int length)
{
	char* ss = (char*)calloc(1, length);
	memcpy(ss, s, length);
	orzlist_prepend(o->list, (void*)ss, (void*)length);
	o->length += length;
	return;
}

void orzsb_print(orzsb* o, char* s)
{
	orzsb_write(o, (void*)s, strlen(s));
}

void orzsb_puts(orzsb* o, char* s)
{
	orzsb_write(o, (void*)s, strlen(s));
	orzsb_write(o, (void*)"\n", 1);
}

char* orzsb_build(orzsb* o)
{
	char* re = (char*)calloc(1, o->length + 1);
	char* p = re;
	orzlist* rev = orzlist_reverse(o->list);
	orzlist* l = rev;
	if (l->next != rev)
		while (l = l->next)
		{
			memcpy(p, l->k, (int)l->v);
			p += (int)l->v;
			
			if (l->next == rev) break;
		}
	
	*p = (char)0;
	orzlist_clean(rev);
	free(rev);
	return re;
}

void orzsb_clean(orzsb* o)
{
	orzlist* l = o->list;
	if (l->next != l)
		while (l = l->next)
		{
			free(l->k);
			
			if (l->next == o->list) break;
		}
	orzlist_clean(o->list);
}

void orzsb_printf(orzsb* o, const char * format, ...)
{
	int len;
	char* buffer;
	va_list args;
	va_start(args, format);
	len = vsnprintf(0, 0, format, args) + 1;
	buffer = (char*)calloc(1, len);
	vsnprintf(buffer, len, format, args);
	orzsb_print(o, buffer);
	free(buffer);
	va_end(args);
}

#ifdef	__cplusplus
}
#endif