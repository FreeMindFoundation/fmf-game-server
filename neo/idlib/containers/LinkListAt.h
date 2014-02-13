/*
 * Copyright (C) Free Mind Foundation 2013
 * www.fmf-base.org xorentor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * */
#ifndef LINKLISTAT_H_
#define LINKLISTAT_H_

// int32
#define cmpxchg( ptr, _old, _new, fail_label ) { \
        volatile unsigned int *__ptr = (volatile unsigned int *)(ptr);   \
        asm goto( "lock; cmpxchg %1,%0 \t\n"           \
        "jnz %l[" #fail_label "] \t\n"               \
        : : "m" (*__ptr), "r" (_new), "a" (_old)       \
        : "memory", "cc"                             \
        : fail_label );                              \
}

/*
 *   node must have 'next' on offset 0
 */
static inline void LL_ADD_AT( void **list, void *node ) {
        volatile void *oldHead;
again:
        oldHead = *(void **)list;
        *((ptr *)node) = (ptr)oldHead;
        cmpxchg( list, oldHead, node, again );
}

#endif
