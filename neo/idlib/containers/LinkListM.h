#ifndef LINKLISTM_H_
#define LINKLISTM_H_

#define LL_ADD( dst, src )\
	src->next = dst;\
	dst = src;

#endif
