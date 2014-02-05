/*
 Copyright (C) Free Mind Foundation 2013
 www.fmf-base.org
 xorentor

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "../idlib/precompiled.h"
#pragma hdrstop

// list of pools
static pool_t *head_pool = NULL;
static void *pool_create_block( pool_t *p, int size );

/*
 * public
 * release node
 */
void pool_freenode( pool_t *p, pe_t *n )
{
#ifdef POOL_DEBUG
	assert( p != NULL );
        assert( n != NULL );

        idLib::common->Printf( "releasing a node..\n" );
#endif
        pthread_spin_lock( &p->pool_lock );
        n->next = p->freenode;
        p->freenode = n;
        pthread_spin_unlock( &p->pool_lock );
}

/*
 * public
 * get free node
 */
pe_t *pool_getnode( pool_t *p )
{
        pe_t *e;
        char m = 0;

#ifdef POOL_DEBUG
        assert( p != NULL );
#endif
        pthread_spin_lock( &p->pool_lock );

        // no block exists
        if( p->head_block == NULL ) {
#ifdef POOL_DEBUG
                idLib::common->Printf( "no block exists\n" );
#endif
                p->freenode = (pe_t *)pool_create_block( p, p->blocks_number );
        } else {
                // block exists, but it might not have free nodes
                if( p->freenode->next == NULL ) {
                        // use it, allocate a new one, but return used one ( m = 1 )
#ifdef POOL_DEBUG
                idLib::common->Printf( "new block must be created\n" );
#endif
                        m = 1;
                }
        }

        e = p->freenode;
	memset( p->freenode->data, 0, p->offset );

        // post
        if( m == 1 )
                p->freenode = (pe_t *)pool_create_block( p, p->blocks_number );
        else
                p->freenode = p->freenode->next;

        pthread_spin_unlock( &p->pool_lock );

        return e;
}

/*
 * Create block
 */
static void *pool_create_block( pool_t *p, int size )
{
        pe_t *e, *he, *block_elements;
        block_t *b;
        void *elements_mem;

#ifdef POOL_DEBUG
        ASSERT( p != NULL );
        idLib::common->Printf( "new block to be created, number of elements within a block: [%d]\n", size );
#endif

        he = NULL;
        elements_mem = NULL;
		
        e = (pe_t *)Mem_Alloc( sizeof( pe_t ) * size );
        elements_mem = Mem_Alloc( size * p->offset );

        b = (block_t *)Mem_Alloc( sizeof( block_t ) );
        b->size = size;
        b->next = p->head_block;

        while( 0 < size-- ) {
                e->data = elements_mem;
                e->next_logical = he;
                e->next = he;
                he = e;
#ifdef POOL_DEBUG
        idLib::common->Printf( "element_memory: [%x] element_s: [%x] index: [%d] offset %d\n", (ptr)elements_mem, (ptr)e, size, p->offset );
#endif
                elements_mem = (void *)( (ptr)elements_mem + p->offset );
                e++;
        }

        b->head_element = he;

        p->head_block = b;

        block_elements = he;

        return block_elements;
}

/*
 * public
 * Create pool
 */
pool_t *pool_create( const int offset, const int blocks )
{
        pool_t *p;

        if( offset < 1 )
                return NULL;
        p = pool_get( offset );
        if(p != NULL) {
                return p;
        }

#ifdef POOL_DEBUG
        idLib::common->Printf( "creating pool: [%d]\n", offset );
#endif

        p = (pool_t *)Mem_Alloc( sizeof( pool_t ) );
        p->offset = offset;
        p->freenode = NULL;
	// TODO: add atomic here for sanity
        p->next = head_pool;
        p->head_block = NULL;
        if( blocks == 0 )
                p->blocks_number = BLOCK_SIZE;
        else
                p->blocks_number = blocks;

        head_pool = p;

	// TODO: replace by atomics
	pthread_spin_init( &p->pool_lock, PTHREAD_PROCESS_PRIVATE );

        return p;
}

/*
 * public
 * Get pool by name
 */
pool_t *pool_get( const int name )
{
        pool_t *p;

        p = head_pool;

#ifdef POOL_DEBUG
        //ASSERT( name != 0 );
        idLib::common->Printf( "get pool by name: [%d]\n", name );
#endif
        while( p ) {
                if( p->offset == name )
                        return p;
                p = p->next;
        }

        return NULL;
}

/*
 * public
 * Dump memory manager, only when POOL_DEBUG defined
 */
#ifdef POOL_DEBUG
void mm_dump()
{
        pool_t *p;
        block_t *b;
        pe_t *e;
        char *data, *data1;
        int32 i, j, k, l;

        p = head_pool;

        j = 0;
        while( p ) {
                idLib::common->Printf( "- pool[%xh] address: [%xh]  offset: [%xh]\n", j, (int32)p, p->offset);
                b = p->head_block;
                k = 0;
                while( b ) {
                        e = b->head_element;
                        idLib::common->Printf( "   - block[%xh] address: [%xh]\n", k, (int32)b);
                        l = 0;
                        while( e ) {
                                idLib::common->Printf( "           - Element[%xh] structure address:[%xh] element data address: [%xh] \n", l, (int32)e, (int32)e->data );
                                data = (e->data);
                                data1 = (e->data);
                                idLib::common->Printf( "             data hex:");
                                for( i = 0; i < p->offset; i++ ) {
                                        idLib::common->Printf( "%x ", *data++ );
                                }
                                idLib::common->Printf( "\n");
                                idLib::common->Printf( "             data char:");
                                for( i = 0; i < p->offset; i++ ) {
                                       idLib::common->Printf( "%c ", *data1++ );
                                }
                                idLib::common->Printf( "\n\n");

                                e = e->next_logical;
                                l++;
                        }
                        b = b->next;
                        k++;
                }
                p = p->next;
                j++;
        }
}
#endif

/*
 * public
 * Free all memory back to heap
 */
void mm_free()
{
        pool_t *p, *pdel;
        block_t *b, *bdel;
        p = head_pool;

#ifdef POOL_DEBUG
                idLib::common->Printf( "freeing all memory\n" );
#endif
        while( p ) {
                b = p->head_block;
                while( b ) {
                        free( (void *)( (ptr)b->head_element->data - ( b->size - 1 ) * p->offset ) );

                        free( (void *)( (int)b->head_element - sizeof( pe_t ) * ( b->size - 1 ) ) );
                        bdel = b;
                        b = b->next;
                        free( (void *)bdel );
                }

                pdel = p;
                p = p->next;
                free( (void *)pdel );
        }

        head_pool = NULL;

#ifdef POOL_DEBUG
                idLib::common->Printf( "memory free'd successfuly\n" );
#endif
}


/*
 * public
 * destroy pool by pointer
 */
void destroy_pool( pool_t *dp )
{
        pool_t *p, *prev = NULL;
        block_t *b, *bdel;
        p = head_pool;

#ifdef POOL_DEBUG
        ASSERT( dp != NULL );
#endif

        while( p ) {
                if( p == dp ) {
                        if( p == head_pool )
                                head_pool = p->next;
                        else
                                prev->next = p->next;

#ifdef POOL_DEBUG
                idLib::common->Printf( "pool about to be destroyed %p %d \n", dp, dp->offset );
#endif
                        b = p->head_block;
                        while( b ) {
                                free( (void *)( (ptr)b->head_element->data - ( b->size - 1 ) * p->offset ) );
                                free( (void *)( (ptr)b->head_element - sizeof( pe_t ) * ( b->size - 1 ) ) );
                                bdel = b;
                                b = b->next;
                                free( (void *)bdel );
                        }

                        free( (void *)p );
                        return;
                }
                prev = p;
                p = p->next;
        }
}
                 
