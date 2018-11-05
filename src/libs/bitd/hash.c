/*****************************************************************************
 *
 * Original Author: Andrei Radulescu-Banu
 * Creation Date:   
 * Description:
 *
 * Copyright (C) 2018 Andrei Radulescu-Banu.  All Rights Reserved.
 * Unauthorized reproduction, modification, distribution, transmission,
 * republication, display or performance are strictly prohibited.
 ****************************************************************************/

/*****************************************************************************
 *                                INCLUDE FILES
 *****************************************************************************/
#include "bitd/hash.h"


/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS
 *****************************************************************************/



/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/

typedef struct hash_entry {
    bitd_uint32 hashed_key;
    bitd_hash_key key;
    bitd_hash_value value;
    struct hash_entry *next;    
} hash_entry;

struct bitd_hash_s {
    bitd_mutex lock;
    bitd_hash_func_t *hash_func;
    bitd_hash_compare_t *hash_cmp;
    bitd_hash_free_t *hash_free;
    hash_entry *e[256];
};


/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/



/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/



/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/


/*
 *============================================================================
 *                        bitd_hash_create
 *============================================================================
 * Description: Create a hash, with a given hashing function
 * Parameters:
 *     hash_func - pointer to hashing routine
 *     hash_comp - key comparison routine, returning 0 on match
 *     hash_free - possibly NULL pointer to routine called after a
 *              hash element has been freed
 * Returns:
 */
bitd_hash bitd_hash_create(bitd_hash_func_t *f, 
			   bitd_hash_compare_t *cmp,
			   bitd_hash_free_t *g) {
    bitd_hash h = NULL;
    int ret = -1;

    /* Paramter check */
    if (!f || !cmp) {
        goto end;
    }

    /* Allocate the hash control block */
    h = calloc(1, sizeof(*h));
    h->lock = bitd_mutex_create();
    if (!h->lock) {
        goto end;
    }
    
    /* Copy the function pointers */
    h->hash_func = f;
    h->hash_cmp = cmp;
    h->hash_free = g;

    /* Success */
    ret = 0;

end:
    if (ret < 0) {
        bitd_hash_destroy(h);
        return NULL;
    }

    return h;
}


/*
 *============================================================================
 *                        bitd_hash_destroy
 *============================================================================
 * Description: Remove all elements from the hash, and destroy the hash
 * Parameters:
 * Returns:
 */
void bitd_hash_destroy(bitd_hash h) {
    bitd_uint32 idx;
    hash_entry *e, *e_next = NULL;


    if (h) {
        for (idx = 0; idx < 255; idx++) {
            for (e = h->e[idx]; e; e = e_next) {
                e_next = e->next;
                
                if (h->hash_free) {
                    h->hash_free(e->key, e->value);
                }
                free(e);
            }
        }
        
        if (h->lock) {
            bitd_mutex_destroy(h->lock);
        }
        free(h);
    }
}


/*
 *============================================================================
 *                        bitd_hash_add
 *============================================================================
 * Description: Add a hash element. May fail in case of a key collision.
 * Parameters:
 * Returns:
 */
bitd_boolean bitd_hash_add(bitd_hash h, bitd_hash_key k, bitd_hash_value v) {
    bitd_uint32 hashed_key;
    bitd_uint32 idx;
    hash_entry *e;

    hashed_key = h->hash_func(k);
    idx = hashed_key & 0xff;

    bitd_mutex_lock(h->lock);

    /* Make sure there's no key collision */
    for (e = h->e[idx]; e; e = e->next) {
        if (e->hashed_key == hashed_key &&
            !h->hash_cmp(e->key, k)) {
            /* Key collision */
            bitd_mutex_unlock(h->lock);            
            return FALSE;            
        }
    }
     
    /* Insert at head of list */
    e = malloc(sizeof(hash_entry));
    e->hashed_key = hashed_key;
    e->key = k;
    e->value = v;
    e->next = h->e[idx];
    h->e[idx] = e;

    bitd_mutex_unlock(h->lock);

    return TRUE;
}


/*
 *============================================================================
 *                        bitd_hash_remove
 *============================================================================
 * Description: Remove a hash element. Return TRUE if hash element 
 *        was actually removed.
 * Parameters:
 * Returns:
 */
bitd_boolean bitd_hash_remove(bitd_hash h, bitd_hash_key k) {
    bitd_uint32 hashed_key;
    bitd_uint32 idx;
    hash_entry *e, *e_prev;

    hashed_key = h->hash_func(k);
    idx = hashed_key & 0xff;

    bitd_mutex_lock(h->lock);

    for (e = h->e[idx], e_prev = NULL; e; e_prev = e, e = e->next) {
        if (e->hashed_key == hashed_key &&
            !h->hash_cmp(e->key, k)) {
            /* Key match */
            if (!e_prev) {
                bitd_assert(e == h->e[idx]);
                h->e[idx] = e->next;
            } else {
                e_prev->next = e->next;
            }

            if (h->hash_free) {
                h->hash_free(e->key, e->value);
            }
            free(e);
            bitd_mutex_unlock(h->lock);
            return TRUE;            
        }
    }

    bitd_mutex_unlock(h->lock);

    return FALSE;
}


/*
 *============================================================================
 *                        bitd_hash_map
 *============================================================================
 * Description: Call the passed-in map function for all elements in the hash.
 *     The mapping function may safely remove or add elements to the hash.
 * Parameters:
 * Returns:
 */
void bitd_hash_map(bitd_hash h, bitd_hash_map_t *m, void *cookie) {
    bitd_uint32 idx;
    hash_entry *e;

    bitd_mutex_lock(h->lock);

    /* Make sure there's no key collision */
    for (idx = 0; idx < 255; idx++) {
        for (e = h->e[idx]; e; e = e->next) {
            /* Execute the map function */
            m(e->key, e->value, cookie);
        }
    }

    bitd_mutex_unlock(h->lock);
}


/*
 *============================================================================
 *                        bitd_hash_func_int32
 *============================================================================
 * Description:
 * Parameters:
 * Returns:
 */
bitd_uint32 bitd_hash_func_int32(bitd_hash_key k) {
    return (bitd_uint32)(long long)k;
}


/*
 *============================================================================
 *                        bitd_hash_compare_int32
 *============================================================================
 * Description:
 * Parameters:
 * Returns:
 */
bitd_int32 bitd_hash_compare_int32(bitd_hash_key k1, bitd_hash_key k2) {
    return (bitd_uint32)((long long)k1 - (long long)k2);
}


/*
 *============================================================================
 *                        bitd_hash_func_string
 *============================================================================
 * Description:
 * Parameters:
 * Returns:
 */
bitd_uint32 bitd_hash_func_string(bitd_hash_key k) {
    bitd_uint32 hashed_key = 0;
    char *c = (char *)k, c1;

    if (c) {
        while((c1 = *c++)) {
            hashed_key += c1;
        }
    }

    return (bitd_uint32)hashed_key;
}


/*
 *============================================================================
 *                        bitd_hash_compare_string
 *============================================================================
 * Description:
 * Parameters:
 * Returns:
 */
bitd_int32 bitd_hash_compare_string(bitd_hash_key k1, bitd_hash_key k2) {

    if (!k1) {
        return k2 ? 1 : 0;
    } else if (!k2) {
        return -1;
    } else {
        return strcmp((char *)k1, (char *)k2);
    }
}
