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
#include "bitd/common.h"
#include "bitd/file.h"



/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/

#define VERBOSE_DEF 0

/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/
struct dll_cb {
    struct dll_cb *next;
    struct dll_cb *prev;
    char *name; /* dll name */
    bitd_dll h;   /* dll handle */
};

struct dll_list {
    struct dll_cb *head;
    struct dll_cb *tail;
};

#define DLL_LIST_HEAD(d) \
    ((struct dll_cb *)&(d)->head)


/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/



/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/
static char* g_prog_name = "";
static int g_verbose = VERBOSE_DEF;
static char *g_load_path;

static struct dll_list g_dlls;

/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/
typedef int (*func_t)(void);


/*
 *============================================================================
 *                        usage
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void usage() {

    printf("\nUsage: %s [OPTIONS ... ]\n\n", g_prog_name);
    printf("This program tests the thred API.\n\n");

    printf("Options:\n"
           "    -lp|--load-path path\n"
           "        Dll load library path.\n"
           "    -dll-load dllname\n"
           "        Load dll.\n"
           "    -dll-unload dllname\n"
           "        Unload dll.\n"
           "    -dll-exec dllname funcname\n"
           "        Execute funcname() in dll. Assume int ()(void)  function type.\n"
           "    -dll-exec-ret dllname funcname ret\n"
           "        Same but expect ret as integer return code.\n"
           "    -v verbose_level, --verbose verbose_level\n"
           "        Set the verbosity level (default: %d).\n"
           "    -h, --help, -?\n"
           "        Show this help.\n\n"
	   "Environment vaiables (command line options take precedence):\n"
	   "    %s - Dll load library path.\n"
           , VERBOSE_DEF, bitd_get_dll_path_name());
} 

/*
 *============================================================================
 *                        main
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int main(int argc, char ** argv) {
    char *c;
    bitd_boolean env_load_path_p = FALSE, dll_found_p;
    char *dll_name;
    bitd_dll h;
    struct dll_cb *dcb;
    char buf[1024];
    func_t f;
    int f_ret, f_expects_ret, f_expected_ret;

    bitd_sys_init();

    /* Parse program name argument */
    g_prog_name = bitd_get_leaf_filename(argv[0]);

    /* Initialize the g_dlls */
    g_dlls.head = DLL_LIST_HEAD(&g_dlls);
    g_dlls.tail = DLL_LIST_HEAD(&g_dlls);

    /* Get the dll path from the environment */
    c = getenv(bitd_get_dll_path_name());
    if (c) {
	g_load_path = strdup(c);
	env_load_path_p = TRUE;
    }

    /* Skip to next parameter */
    argc--;
    argv++;

    /* Parse vector size and help */
    while (argc) {
        if (!strcmp(argv[0], "-lp")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    if (env_load_path_p) {
		env_load_path_p = FALSE;
		if (g_load_path) {
		    free(g_load_path);
		    g_load_path = NULL;
		}
	    }

            /* Concatenate the path */
            g_load_path = bitd_envpath_add(g_load_path, argv[0]);

	    if (g_verbose) {
		printf("Path: %s\n", g_load_path ? g_load_path : "NULL");
	    }

	} else if (!strcmp(argv[0], "-dll-load")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    dll_name = bitd_get_dll_name(argv[0]);
	    
	    /* Look up the dll name in the path with 'read' permissions */
	    c = bitd_envpath_find(g_load_path, dll_name, S_IREAD);
	    if (!c) {
		fprintf(stderr, "Could not find %s in path %s.\n",
			dll_name, g_load_path);
		exit(-1);
	    }

	    h = bitd_dll_load(c);
	    if (!h) {
		fprintf(stderr, "Could not load %s: %s.\n",
			c, bitd_dll_error(buf, sizeof(buf)));
		free(c);
		exit(-1);
		
	    }

	    dcb = malloc(sizeof(*dcb));
	    dcb->name = strdup(argv[0]);
	    dcb->h = h;
	    dcb->next = DLL_LIST_HEAD(&g_dlls);
	    dcb->prev = g_dlls.tail;
	    dcb->next->prev = dcb;
	    dcb->prev->next = dcb;

	    if (g_verbose) {
		printf("Loaded %s\n",
		       c);
	    }

	    free(c);
	    free(dll_name);

	} else if (!strcmp(argv[0], "-dll-unload")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    dll_found_p = FALSE;
	    for (dcb = g_dlls.head; 
		 dcb != DLL_LIST_HEAD(&g_dlls); 
		 dcb = dcb->next) {

		if (!strcmp(dcb->name, argv[0])) {
		    dcb->next->prev = dcb->prev;
		    dcb->prev->next = dcb->next;
		    free(dcb->name);
		    bitd_dll_unload(dcb->h);

		    if (g_verbose) {
			printf("Unloaded %s\n", argv[0]);
		    }
		    free(dcb);
		    dll_found_p = TRUE;
		    break;
		}
	    }

	    if (!dll_found_p) {
		fprintf(stderr, "Could not find library %s\n", argv[0]);
		exit(-1);
	    }

	} else if (!strcmp(argv[0], "-dll-exec") ||
		   !strcmp(argv[0], "-dll-exec-ret")) {

	    f_expects_ret = FALSE;
	    if (!strcmp(argv[0], "-dll-exec-ret")) {
		f_expects_ret = TRUE;
	    }

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    dll_found_p = FALSE;
	    for (dcb = g_dlls.head; 
		 dcb != DLL_LIST_HEAD(&g_dlls); 
		 dcb = dcb->next) {

		if (!strcmp(dcb->name, argv[0])) {
		    dll_found_p = TRUE;
		    break;
		}
	    }

	    if (!dll_found_p) {
		fprintf(stderr, "Could not find library %s\n", argv[0]);
		exit(-1);
	    }

            /* Skip to next parameter */
            argc--;
            argv++;

            if (!argc) {
                usage();
		exit(-1);
            }

	    /* Look up the dll symbol */
	    f = bitd_dll_sym(dcb->h, argv[0]);
	    
	    if (!f) {
		fprintf(stderr, "Could not find symbol %s in %s\n", argv[0], 
			dcb->name);
		exit(-1);
	    }

	    f_ret = f();
	    
	    if (g_verbose > 0) {
		printf("%s: %s() returns %d\n", 
		       (argv-1)[0], 
		       argv[0], 
		       f_ret);
	    }
		
	    if (f_expects_ret) {
		/* Get the expected return code */
		argc--;
		argv++;

		if (!argc) {
		    usage();
		    exit(-1);
		}

		f_expected_ret = atoi(argv[0]);

		if (f_ret != f_expected_ret) {
		    fprintf(stderr, "%s() returns %d, expected %d\n", 
			    (argv-1)[0], 
			    f_ret,
			    atoi(argv[0]));
		    exit(-1);
		}
	    }

        } else if (!strcmp(argv[0], "-v") ||
                   !strcmp(argv[0], "--verbose")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

            /* Get the verbose level */
            g_verbose = atoi(argv[0]);
        } else if (!strcmp(argv[0], "-h") ||
                   !strcmp(argv[0], "--help") ||
                   !strcmp(argv[0], "-?")) {
            usage();
	    exit(0);
        } else {
            printf("%s: Skipping invalid parameter %s\n", g_prog_name, argv[0]);
        }

        /* Skip to next argument */
        argc--;
        argv++;        
    }

    while (g_dlls.head != DLL_LIST_HEAD(&g_dlls)) {
	dcb = g_dlls.head;
	dcb->next->prev = dcb->prev;
	dcb->prev->next = dcb->next;
	free(dcb->name);
	bitd_dll_unload(dcb->h);
	free(dcb);
    }

    bitd_sys_deinit();

    if (g_load_path) {
	free(g_load_path);
    }    

    return 0;
}
