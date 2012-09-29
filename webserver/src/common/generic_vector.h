#ifndef WS_GENERIC_VECTOR_H
#define WS_GENERIC_VECTOR_H


#include "config.h"
#include <stdlib.h>


#define WS_GEN_VECTOR(name, element_type) \
	element_type * name ## _begin; \
	element_type * name ## _end; \
	element_type * name ## _allocated

#define WS_GEN_VECTOR_CREATE(reference) do { \
		reference ## _begin = \
		reference ## _end = \
		reference ## _allocated = 0; \
	} while (0)

#define WS_GEN_VECTOR_DESTROY(reference) do { \
		free( reference ## _begin ); \
	} while (0)

#define WS_GEN_VECTOR_RESERVE(reference, capacity) do { \
		void *new_begin; \
		const size_t old_size = WS_GEN_VECTOR_SIZE(reference); \
		if (WS_GEN_VECTOR_CAPACITY(reference) >= (capacity)) break; \
		new_begin = realloc( WS_GEN_VECTOR_DATA(reference), ((capacity) * sizeof(*WS_GEN_VECTOR_DATA(reference)))); \
		if (!new_begin) break; \
		WS_GEN_VECTOR_BEGIN(reference) = new_begin; \
		WS_GEN_VECTOR_END(reference) = WS_GEN_VECTOR_BEGIN(reference) + old_size; \
		(reference ## _allocated) = (WS_GEN_VECTOR_BEGIN(reference) + (capacity)); \
	} while(0)

#define WS_GEN_VECTOR_GROW(reference, min_capacity) do { \
		const size_t min_cap_copy = (min_capacity); \
		WS_GEN_VECTOR_RESERVE(reference, \
			(min_cap_copy > WS_GEN_VECTOR_CAPACITY(reference)) ? (min_cap_copy * 2) : min_cap_copy); \
	} while(0)

#define WS_GEN_VECTOR_PUSH_BACK(reference, element) do { \
		WS_GEN_VECTOR_GROW(reference, (WS_GEN_VECTOR_SIZE(reference) + 1)); \
		*WS_GEN_VECTOR_END(reference) = element; \
		++(WS_GEN_VECTOR_END(reference)); \
	} while(0)

#define WS_GEN_VECTOR_SIZE(reference) (size_t)(WS_GEN_VECTOR_END(reference) - WS_GEN_VECTOR_BEGIN(reference))

#define WS_GEN_VECTOR_CAPACITY(reference) (size_t)((reference ## _allocated) - WS_GEN_VECTOR_BEGIN(reference))

#define WS_GEN_VECTOR_DATA(reference) (reference ## _begin)

#define WS_GEN_VECTOR_BEGIN(reference) (reference ## _begin)

#define WS_GEN_VECTOR_END(reference) (reference ## _end)


#endif
