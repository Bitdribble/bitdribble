/*****************************************************************************
 *
 * Original Author: Andrei Radulescu-Banu
 * Creation Date:   
 * Description: 
 * 
 * Copyright (C) 2018 by Andrei Radulescu-Banu.  All Rights Reserved.
 * Unauthorized reproduction, modification, distribution, transmission, 
 * republication, display or performance are strictly prohibited.
 ****************************************************************************/

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/types.h"


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
 *                        main
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int main(int argc, char **argv) {
    bitd_nvp_t nvp;
    bitd_nvp_t nvp1;
    bitd_value_t v;
    int i;
    char *buf;

    nvp = bitd_nvp_alloc(10);

    bitd_nvp_add_elem(&nvp, "name-void", NULL, bitd_type_void);

    v.value_boolean = FALSE;
    bitd_nvp_add_elem(&nvp, "name-boolean", &v, bitd_type_boolean);

    v.value_int64 = -9223372036854775807LL; /* -0x7fffffffffffffff in decimal */
    bitd_nvp_add_elem(&nvp, "name-int64", &v, bitd_type_int64);
    
    v.value_uint64 = 18446744073709551615ULL; /* 0xffffffffffffffff in decimal */
    bitd_nvp_add_elem(&nvp, "name-uint64", &v, bitd_type_uint64);
    
    v.value_double = -.99;
    bitd_nvp_add_elem(&nvp, "name-double", &v, bitd_type_double);

    v.value_string = "string-value";
    bitd_nvp_add_elem(&nvp, "name-string", &v, bitd_type_string);

    v.value_blob = bitd_blob_alloc(7);
    for (i = 0; i < 7; i++) {
	bitd_blob_payload(v.value_blob)[i] = i;
    }

    bitd_nvp_add_elem(&nvp, "name-blob", &v, bitd_type_blob);
    free(v.value_blob);

    nvp1 = bitd_nvp_alloc(10);

    v.value_nvp = nvp1;
    bitd_nvp_add_elem(&nvp, "empty-nvp-value", &v, bitd_type_nvp);
    bitd_nvp_free(nvp1);
    
    nvp1 = bitd_nvp_clone(nvp);

    v.value_nvp = nvp1;
    bitd_nvp_add_elem(&nvp, "full-nvp-value", &v, bitd_type_nvp);
    bitd_nvp_free(nvp1);

    buf = bitd_nvp_to_string(nvp, "prefix> ");
    printf("%s\n", buf);
    free(buf);

    buf = bitd_nvp_to_xml(nvp, "nvp", TRUE);
    printf("%s\n", buf);
    free(buf);

    bitd_nvp_free(nvp);

    return 0;
}
