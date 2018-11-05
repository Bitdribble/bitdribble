/*****************************************************************************
 *
 * Original Author: Andrei Radulescu-Banu
 * Creation Date:   
 * Description: 
 * 
 * Copyright 2018 by Andrei Radulescu-Banu.  All Rights Reserved.
 * Unauthorized reproduction, modification, distribution, transmission, 
 * republication, display or performance are strictly prohibited.
 ****************************************************************************/

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/platform-netdb.h"

#if defined(BITD_HAVE_NETDB_H)
# include "netdb.h"
#endif


/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/



/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/



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
 *                        bitd_hostent_clone
 *============================================================================
 * Description:     Clone bitd_hostent into bitd_hostent
 * Parameters:    
 * Returns:  
 */
bitd_hostent bitd_hostent_clone(bitd_hostent h) {
    bitd_uint32 i, size, idx, len, n_aliases = 0, n_addr = 0;
    bitd_hostent h_out;
    char *buf;

    if (!h) {
        return NULL;
    }

    /* Initial size is size of hostent struct */
    size = sizeof(*h_out);

    /* Eight byte alignment */
    size = ((size + 7) / 8) * 8;
    
    /* Add name length. Account for NULL termination */
    if (h->h_name) {
        size += strlen(h->h_name);
    }
    size++;
    
    /* Add size of alias names */
    if (h->h_aliases) {
        for (i = 0; h->h_aliases[i]; i++) {
            size += (strlen(h->h_aliases[i]) + 1);
            n_aliases++;
        }
    }

    /* Account for the alias array size */
    size += ((n_aliases + 1) * sizeof(char *));

    /* Add size of address array */
    for (i = 0; h->h_addr_list[i]; i++) {
        size += h->h_length;
        n_addr++;
    }

    /* Account for the host array size */
    size += (n_addr + 1) * sizeof(char *);

    /* Allocate the hostent struct */
    buf = calloc(1, size);
    if (!buf) {
        return NULL;
    }

    h_out = (bitd_hostent)buf;

    /* Point the index at the end of the struct */
    idx = sizeof(*h_out);

    /* Eight byte alignment for the index */
    idx = ((idx + 7) / 8) * 8;

    /* Set up the aliases array */
    h_out->h_aliases = (char **)&buf[idx];
    idx += (n_aliases+1) * sizeof(char *);

    /* Set up the hosts array */
    h_out->h_addr_list = (char **)&buf[idx];
    idx += (n_addr+1) * sizeof(char *);

    /* Copy over member by member */
    h_out->h_name = &buf[idx];
    if (h->h_name) {
        len = strlen(h->h_name) + 1;
        memcpy(h_out->h_name, h->h_name, len);
        idx += len;
    } else {
        idx++;
    }

    bitd_assert(idx <= size);

    /* Copy each alias */
    for (i = 0; h->h_aliases[i]; i++) {
        h_out->h_aliases[i] = &buf[idx];
        len = strlen(h->h_aliases[i]) + 1;
        memcpy(h_out->h_aliases[i], h->h_aliases[i], len);
	idx += len;
    }
    
    /* Copy the NULL alias */
    h_out->h_aliases[i] = NULL;

    h_out->h_addrtype = h->h_addrtype;    
    h_out->h_length = h->h_length;

    /* Copy each address */
    for (i = 0; h->h_addr_list[i]; i++) {
        h_out->h_addr_list[i] = &buf[idx];
        memcpy(h_out->h_addr_list[i], h->h_addr_list[i], h_out->h_length);
        idx += h->h_length;
    }

    /* Copy NULL address */
    h_out->h_addr_list[i] = NULL;
    
    return h_out;
} 


/*
 *============================================================================
 *                        bitd_platform_hostent_clone
 *============================================================================
 * Description:     Same as above, but clone 'struct hostent *' into bitd_hostent
 * Parameters:    
 * Returns:  
 */
bitd_hostent bitd_platform_hostent_clone(struct hostent *h) {
    bitd_uint32 i, size, idx, len, n_aliases = 0, n_addr = 0;
    bitd_hostent h_out;
    char *buf;

    if (!h) {
        return NULL;
    }

    /* Initial size is size of hostent struct */
    size = sizeof(*h_out);

    /* Eight byte alignment */
    size = ((size + 7) / 8) * 8;
    
    /* Add name length. Account for NULL termination */
    if (h->h_name) {
        size += strlen(h->h_name);
    }
    size++;
    
    /* Add size of alias names */
    if (h->h_aliases) {
        for (i = 0; h->h_aliases[i]; i++) {
            size += (strlen(h->h_aliases[i]) + 1);
            n_aliases++;
        }
    }

    /* Account for the alias array size */
    size += ((n_aliases + 1) * sizeof(char *));

    /* Add size of address array */
    for (i = 0; h->h_addr_list[i]; i++) {
        size += h->h_length;
        n_addr++;
    }

    /* Account for the host array size */
    size += (n_addr + 1) * sizeof(char *);

    /* Allocate the hostent struct */
    buf = calloc(1, size);
    if (!buf) {
        return NULL;
    }

    h_out = (bitd_hostent)buf;

    /* Point the index at the end of the struct */
    idx = sizeof(*h_out);

    /* Eight byte alignment for the index */
    idx = ((idx + 7) / 8) * 8;

    /* Set up the aliases array */
    h_out->h_aliases = (char **)&buf[idx];
    idx += (n_aliases+1) * sizeof(char *);

    /* Set up the hosts array */
    h_out->h_addr_list = (char **)&buf[idx];
    idx += (n_addr+1) * sizeof(char *);

    /* Copy over member by member */
    h_out->h_name = &buf[idx];
    if (h->h_name) {
        len = strlen(h->h_name) + 1;
        memcpy(h_out->h_name, h->h_name, len);
        idx += len;
    } else {
        idx++;
    }

    bitd_assert(idx <= size);

    /* Copy each alias */
    for (i = 0; h->h_aliases[i]; i++) {
        h_out->h_aliases[i] = &buf[idx];
        len = strlen(h->h_aliases[i]) + 1;
        memcpy(h_out->h_aliases[i], h->h_aliases[i], len);
	idx += len;
    }
    
    /* Copy the NULL alias */
    h_out->h_aliases[i] = NULL;

    h_out->h_addrtype = h->h_addrtype;    
    h_out->h_length = h->h_length;

    /* Copy each address */
    for (i = 0; h->h_addr_list[i]; i++) {
        h_out->h_addr_list[i] = &buf[idx];
        memcpy(h_out->h_addr_list[i], h->h_addr_list[i], h_out->h_length);
        idx += h->h_length;
    }

    /* Copy NULL address */
    h_out->h_addr_list[i] = NULL;
    
    return h_out;
} 


/*
 *============================================================================
 *                        bitd_hostent_free
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_hostent_free(bitd_hostent h) {
    if (h) {
        free(h);
    }
} 
