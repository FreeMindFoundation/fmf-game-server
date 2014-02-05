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
