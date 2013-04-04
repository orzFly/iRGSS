#pragma once
#include "orzstr.h"
#ifdef	__cplusplus
extern "C" {
#endif
	

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


void iterate_str(char *start, iterate_str_func f, void * opaque){
	while(*start)
		f(start++, opaque); 
}

void _addslashes_size(char *s, int *n){
	*n += strchr("\\\n\r\t\'\"", *s) ? 2 : 1;
}

void _addslashes_write(char *s, char **n){
	if(strchr("\\\n\r\t\'\"", *s)){
		*(*n)++ = '\\';
	}
	if (*s == '\n') *(*n)++ = 'n';
	else if (*s == '\r') *(*n)++ = 'r';
	else if (*s == '\t') *(*n)++ = 't';
	else *(*n)++ = *s;
}

char* addslashes(char *s){
	int len;
	char* re;
	char* p;
	iterate_str(s, (iterate_str_func)_addslashes_size, &len);
	p = re = (char *)calloc(1, len + 1);
	iterate_str(s, (iterate_str_func)_addslashes_write, &p);
	*(re + len) = 0;
	return re;
}

void _addslash_size(char *s, int *n){
	*n += strchr("\\\'", *s) ? 2 : 1;
}

void _addslash_write(char *s, char **n){
	if(strchr("\\\'", *s)){
		*(*n)++ = '\\';
	}
	*(*n)++ = *s;
}

char* addslash(char *s){
	int len;
	char* re;
	char* p;
	iterate_str(s, (iterate_str_func)_addslash_size, &len);
	p = re = (char *)calloc(1, len + 1);
	iterate_str(s, (iterate_str_func)_addslash_write, &p);
	*(re + len) = 0;
	return re;
}

#ifdef	__cplusplus
}
#endif