#pragma once
#include "orzstr.h"
#ifdef	__cplusplus
extern "C" {
#endif
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
	p = re = (char *)calloc(1, re + 1);
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
	p = re = (char *)calloc(1, re + 1);
	iterate_str(s, (iterate_str_func)_addslash_write, &p);
	*(re + len) = 0;
	return re;
}

#ifdef	__cplusplus
}
#endif