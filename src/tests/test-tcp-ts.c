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
#include "bitd/platform-inetutil.h"
#include "bitd/file.h"
#include "bitd/tstamp.h"



/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/
#define TESTTCP_SOCK_NOERROR(s)						\
    do {								\
	int ret = (s);							\
	if (ret < 0) {							\
	    fprintf(stderr,						\
		    "%s: %s:%d: Socket routine returned %d, (%s %d)\n",	\
		    g_prog_name, __FILE__, __LINE__, 			\
		    ret, strerror(bitd_socket_errno), bitd_socket_errno);	\
	    exit(1);							\
	}								\
    } while (0)

#define ADDR_DEF_CLIENT_SERVER "127.0.0.1"
#define ADDR_DEF_SERVER "0.0.0.0"

#define VERBOSE_DEF 1

/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/



/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/



/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/
static char* g_prog_name = "";
static int g_verbose = VERBOSE_DEF;
static int g_sleep_tmo = 100;
static bitd_boolean g_blocking_send = FALSE;
static bitd_boolean g_blocking_recv = FALSE;
static bitd_boolean g_do_send = TRUE;
static bitd_boolean g_do_recv = TRUE;

/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/


/*
 *============================================================================
 *                        usage
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void usage(void) {
    printf("Usage: %s OPTIONS\n", g_prog_name);
    printf("OPTIONS:\n"
           "  -n iteration_count\n"
           "      Perform that many iterations (default: 250)\n"
           "  -p|--port port\n"
           "      Server port (default: 2010)\n"
           "  -a|--addr addr\n"
           "      Server address.\n"
	   "      Default: %s when running as client-server, else %s.\n"
           "  -s|--server\n"
           "      Run in server-only mode\n"
           "  -c|--client\n"
           "      Run in client-only mode\n"
           "  -i ifname\n"
           "      The interface name to bind to.\n"
           "  -enq\n"
           "      Report enqueue timestamps.\n"
           "  -ack\n"
           "      Report ack timestamps.\n"
           "  -send-block|-send-nonblock|-no-send\n"
           "      Perform blocking send/nonblocking send/no send.\n"
           "      Default: non-blocking send.\n"
           "  -recv-block|-recv-nonblock|-no-recv\n"
           "      Perform blocking recv/nonblocking recv/no recv.\n"
           "      Default: non-blocking recv.\n"
           "  -sleep duration\n"
           "      Sleep that many millisecs when receiving, or when failing to send\n"
           "  -size-min|-size-max|-size payload_size\n"
           "  -size-send-min|-size-send-max|size-send payload_size\n"
           "  -size-recv-min|-size-recv-max|size-recv payload_size\n"
           "      Send or receive in chunks of at least or at most 'size' bytes.\n"
           "      Default: 1000 bytes.\n"
           "  -tos tos_value\n"
           "      Use the passed-in type of service value.\n"
           "      By default, the type of service is zero.\n"
           "  -v verbose_level, --verbose verbose_level\n"
           "      Set the verbosity level (default: %d).\n"
           "  -h, --help, -?\n"
           "      Show this help.\n"
	   , ADDR_DEF_CLIENT_SERVER, ADDR_DEF_SERVER, VERBOSE_DEF
        );
} 


/*
 *============================================================================
 *                        get_ts_level
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static char *get_ts_level(ts_timestamp_t *ts) {
    
    if (ts->flags & TS_FLAG_KERNEL) {
	return "kernel";
    }
    if (ts->flags & TS_FLAG_SW) {
	return "sw";
    }
    if (ts->flags & TS_FLAG_DROPPED) {
	return "dropped";
    }

    return "unknown";
} 


/*
 *============================================================================
 *                        get_ts_txtype
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static char *get_ts_txtype(ts_timestamp_t *ts) {
    
    if (ts->flags & TS_FLAG_ENQ) {
	return "enq";
    }
    if (ts->flags & TS_FLAG_SND) {
	return "snd";
    }
    if (ts->flags & TS_FLAG_ACK) {
	return "ack";
    }

    return "unknown";
} 



/*
 *============================================================================
 *                        tcp_set_ts_options
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void tcp_set_ts_options(ts_socket_t s, bitd_boolean enq_ts, bitd_boolean ack_ts) {
    int opt;
    bitd_socklen_t optlen;

    if (enq_ts || ack_ts) {
	optlen = sizeof(opt);
	
	TESTTCP_SOCK_NOERROR(ts_getsockopt(s, SOL_TS_SOCKET, SO_TS_FLAGS,
					   &opt, &optlen));
	
	if (enq_ts) {
	    opt |= TS_FLAG_ENQ;
	}
	if (ack_ts) {
	    opt |= TS_FLAG_ACK;
	}
	
	TESTTCP_SOCK_NOERROR(ts_setsockopt(s, SOL_TS_SOCKET, SO_TS_FLAGS,
					   &opt, optlen));
    }
} 

/*
 *============================================================================
 *                        tcp_send
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static int tcp_send(ts_socket_t s, char *buf, int bufsize, 
		    int i, char *prefix, int *sent) {
    int ret = 0;
    struct bitd_pollfd p;
    ts_timestamp_t ts;

    if (g_do_send) {	    
	/* Make socket blocking/non-blocking */
	TESTTCP_SOCK_NOERROR(bitd_set_blocking(ts_get_sock_fd(s), 
					     g_blocking_send));
	
	/* Do the send */
	ret = ts_send(s, buf, bufsize, 0);
	if (ret > 0) {
	    *sent += ret;
	    if (g_verbose >= 1) {
		printf("%d: %s: send() %d/%d bytes, total %d\n",
		       i, prefix,
		       ret, bufsize, *sent);
	    }

	    /* Get the timestamp file descriptor to poll for timestamps */
	    p.fd = ts_get_tstamp_fd(s);
	    p.events = BITD_POLLIN|BITD_POLLERR;
	    
	    ret = bitd_poll(&p, 1, BITD_FOREVER);
#if 0
	    printf("bitd_poll() ret %d, fd %d, %s %d\n", ret, p.fd,
		   strerror(bitd_socket_errno), bitd_socket_errno);
#endif

	    /* Read the timestamp */
	    while (ts_get_tstamp_tx(s, &ts) >= 0) {
		if (g_verbose >= 1) {
		    printf("\tTstamp %u.%09u seq %d %s %s\n",
			   (bitd_uint32)(ts.tstamp/1000000000),
			   (bitd_uint32)(ts.tstamp % 1000000000),
			   ts.seq,
			   get_ts_level(&ts),
			   get_ts_txtype(&ts));
		}
	    }
	} else {
	    printf("%d: %s: send() returns %d (%s %d), total %d\n",
		   i, prefix,
		   ret, strerror(bitd_socket_errno), bitd_socket_errno, *sent);
	    bitd_sleep(g_sleep_tmo);
	}
    }
    
    return ret;
} 


/*
 *============================================================================
 *                        tcp_recv
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int tcp_recv(ts_socket_t s, char *buf, int bufsize, 
	     int i, char *prefix, int *recvd) {

    int ret = 0;
    ts_timestamp_t ts;
                 
    if (g_do_recv) {
	/* Make socket blocking/non-blocking */
	TESTTCP_SOCK_NOERROR((bitd_set_blocking(ts_get_sock_fd(s), 
					      g_blocking_recv)));
        
	/* Do the receive */        
	bitd_sleep(g_sleep_tmo);
	ret = ts_recv(s, buf, bufsize, 0, &ts);
	if (ret > 0) {
	    *recvd += ret;
	    if (g_verbose >= 1) {
		printf("%d: %s: recv() %d/%d bytes, total %d\n", 
		       i, prefix,
		       ret, bufsize, *recvd);
                printf("\tTstamp %u.%09u %s\n",
		       (bitd_uint32)(ts.tstamp/1000000000),
		       (bitd_uint32)(ts.tstamp % 1000000000),
		       get_ts_level(&ts));
	    }
	} else {
	    printf("%d: %s: recv() returns %d (%s %d), total %d\n", 
		   i, prefix,
		   ret, strerror(bitd_socket_errno), bitd_socket_errno, *recvd);
	}
    }

    return ret;
} 


int main(int argc, char **argv) {
    bitd_uint32 iter = 250, i;
    ts_socket_t client_sock = NULL, srv_sock = NULL, accept_sock = NULL;
    struct sockaddr_storage addr;
    int srv_sent = 0, srv_recvd = 0;
    int client_sent = 0, client_recvd = 0;
    bitd_boolean server = TRUE, client = TRUE;
    char *ifname = NULL;
    int g_sleep_tmo = 100;
    int size_send_min = 1000, size_send_max = 1000;
    int size_recv_min = 1000, size_recv_max = 1000;
    char *buf = NULL;
    int bufsize;
    int tos = 0;
    bitd_uint32 a[4];
    char *addr_str = NULL;
    bitd_uint32 port = 2010;  /* Default port */
    bitd_boolean enq_ts = FALSE, ack_ts = FALSE;
    int opt;
    
    /* Windows needs winsock.dll initialized */
    if (!bitd_sys_init()) {
        printf("Failed to initialize the system\n");
        return 1;
    }

    /* Initialize addresses */
    memset(&addr, 0, sizeof(addr));

    /* Parse program name argument */
    g_prog_name = bitd_get_leaf_filename(argv[0]);

    /* Skip to next argument */
    argc--;
    argv++;

    while(argc) {
        if (!strcmp(argv[0], "-n")) {
            /* Get next parameter */
            argc--;
            argv++;

            if (!argc) {
                usage();
                return 1;
            }

            /* Get iteration parameter */
            iter = atoi(argv[0]);
        } else if (!strcmp(argv[0], "-p") ||
                   !strcmp(argv[0], "--port")) {
            /* Get next parameter */
            argc--;
            argv++;

            if (!argc) {
                usage();
                return 1;
            }

            port = atoi(argv[0]);
            if (port <= 0 || port > 65535) {
                usage();
                return 1;                
            }

        } else if (!strcmp(argv[0], "-a") ||
                   !strcmp(argv[0], "--addr")) {
            /* Get next parameter */
            argc--;
            argv++;

            if (!argc) {
                usage();
                return 1;
            }

	    addr_str = argv[0];

        } else if (!strcmp(argv[0], "-s") ||
                   !strcmp(argv[0], "--server")) {
            server = TRUE;
            client = FALSE;
        } else if (!strcmp(argv[0], "-c") ||
                   !strcmp(argv[0], "--client")) {
            server = FALSE;
            client = TRUE;
        } else if (!strcmp(argv[0], "-i")) {

            /* Get next parameter */
            argc--;
            argv++;

            if (!argc) {
                usage();
                return 1;
            }

            ifname = argv[0];

        } else if (!strcmp(argv[0], "-enq")) {
            enq_ts = TRUE;
        } else if (!strcmp(argv[0], "-ack")) {
            ack_ts = TRUE;
        } else if (!strcmp(argv[0], "-send-block")) {
            g_do_send = TRUE;
            g_blocking_send = TRUE;
        } else if (!strcmp(argv[0], "-send-nonblock")) {
            g_do_send = TRUE;
            g_blocking_send = FALSE;
        } else if (!strcmp(argv[0], "-no-send")) {
            g_do_send = FALSE;
        } else if (!strcmp(argv[0], "-recv-block")) {
            g_do_recv = TRUE;
            g_blocking_recv = TRUE;
        } else if (!strcmp(argv[0], "-recv-nonblock")) {
            g_do_recv = TRUE;
            g_blocking_recv = FALSE;
        } else if (!strcmp(argv[0], "-no-recv")) {
            g_do_recv = FALSE;
        } else if (!strcmp(argv[0], "-sleep")) {

            /* Get next parameter */
            argc--;
            argv++;

            if (!argc) {
                usage();
                return 1;
            }

            g_sleep_tmo = atoi(argv[0]);
            if (g_sleep_tmo < 0 || g_sleep_tmo > 60000) {
                usage();
                return 1;                
            }
        } else if (!strcmp(argv[0], "-size-min")) {
            /* Get next parameter */
            argc--;
            argv++;

            if (!argc) {
                usage();
                return 1;
            }

            size_send_min = atoi(argv[0]);
            if (size_send_min <= 0) {
                usage();
                return 1;                
            }
            size_recv_min = size_send_min;
        } else if (!strcmp(argv[0], "-size-max")) {
            /* Get next parameter */
            argc--;
            argv++;

            if (!argc) {
                usage();
                return 1;
            }

            size_send_max = atoi(argv[0]);
            if (size_send_max <= 0) {
                usage();
                return 1;                
            }
            size_recv_max = size_send_max;
        } else if (!strcmp(argv[0], "-size")) {
            /* Get next parameter */
            argc--;
            argv++;

            if (!argc) {
                usage();
                return 1;
            }

            size_send_max = atoi(argv[0]);
            if (size_send_max <= 0) {
                usage();
                return 1;                
            }
            size_send_min = size_send_max;
            size_recv_max = size_send_max;
            size_recv_min = size_send_max;
        } else if (!strcmp(argv[0], "-size-send-min")) {
            /* Get next parameter */
            argc--;
            argv++;

            if (!argc) {
                usage();
                return 1;
            }

            size_send_min = atoi(argv[0]);
            if (size_send_min <= 0) {
                usage();
                return 1;                
            }
        } else if (!strcmp(argv[0], "-size-send-max")) {
            /* Get next parameter */
            argc--;
            argv++;

            if (!argc) {
                usage();
                return 1;
            }

            size_send_max = atoi(argv[0]);
            if (size_send_max <= 0) {
                usage();
                return 1;                
            }
        } else if (!strcmp(argv[0], "-size-send")) {
            /* Get next parameter */
            argc--;
            argv++;

            if (!argc) {
                usage();
                return 1;
            }

            size_send_max = atoi(argv[0]);
            if (size_send_max <= 0) {
                usage();
                return 1;                
            }
            size_send_min = size_send_max;
        } else if (!strcmp(argv[0], "-size-recv-min")) {
            /* Get next parameter */
            argc--;
            argv++;

            if (!argc) {
                usage();
                return 1;
            }

            size_recv_min = atoi(argv[0]);
            if (size_recv_min <= 0) {
                usage();
                return 1;                
            }
        } else if (!strcmp(argv[0], "-size-recv-max")) {
            /* Get next parameter */
            argc--;
            argv++;

            if (!argc) {
                usage();
                return 1;
            }

            size_recv_max = atoi(argv[0]);
            if (size_recv_max <= 0) {
                usage();
                return 1;                
            }
        } else if (!strcmp(argv[0], "-size-recv")) {
            /* Get next parameter */
            argc--;
            argv++;

            if (!argc) {
                usage();
                return 1;
            }

            size_recv_max = atoi(argv[0]);
            if (size_send_max <= 0) {
                usage();
                return 1;                
            }
            size_recv_min = size_recv_max;
        } else if (!strcmp(argv[0], "-tos")) {
            /* Get next parameter */
            argc--;
            argv++;

            if (!argc) {
                usage();
                return 1;
            }

            tos = atoi(argv[0]);
            if (tos < 0 || tos > 255) {
                printf("Invalid type of service value %s\n", argv[0]);
                usage();
                return 1;
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
            return 1;
        } else {
            printf("%s: Skipping unknown option %s\n", 
                   g_prog_name, argv[0]);
        }

        argc--;
        argv++;
    }

    /* Normalize size_min and size_max */
    size_send_max = MAX(size_send_max, size_send_min);
    size_recv_max = MAX(size_recv_max, size_recv_min);

    /* Allocate the buffer */
    bufsize = MAX(size_send_max, size_recv_max);
    buf = calloc(1, bufsize);

    /* Get the default address */
    if (!addr_str) {
	if (!client) {
	    addr_str = ADDR_DEF_SERVER;
	} else {
	    addr_str = ADDR_DEF_CLIENT_SERVER;
	}
    }

    /* Get the address and port */
    if (inet_pton(AF_INET, addr_str, a) > 0) {
	/* An ipv4 address */
        bitd_set_sin_family(&addr, AF_INET);
    } else if (inet_pton(AF_INET6, addr_str, a) > 0) {
	/* An ipv6 address */
        bitd_set_sin_family(&addr, AF_INET6);
    } else {
	usage();
	return 1;                
    }
    
    /* Copy the address and port */
    memcpy(bitd_sin_addr(&addr), a, bitd_sin_addrlen(&addr));
    *bitd_sin_port(&addr) = htons((bitd_uint16)port);

    /* Open and bind sockets */
    if (server) {
        srv_sock = ts_socket(bitd_sin_family(&addr), 
			     SOCK_STREAM, IPPROTO_TCP);
	if (!srv_sock) {
	    fprintf(stderr, 
		    "%s: %s:%d: Cannot create socket, %s (errno %d)\n",
		    g_prog_name, __FILE__, __LINE__,
		    strerror(bitd_socket_errno), bitd_socket_errno);
	    return 1;
	}

	if (g_verbose >= 1) {
	    printf("Server socket: %d\n", ts_get_sock_fd(srv_sock));
	}

        if (ifname) {
#if defined(SO_BINDTODEVICE)
            TESTTCP_SOCK_NOERROR(ts_setsockopt(srv_sock, SOL_SOCKET, 
					       SO_BINDTODEVICE,
					       ifname, strlen(ifname) + 1));
#endif
        }

#ifdef SO_REUSEADDR
	opt = 1;
        TESTTCP_SOCK_NOERROR(ts_setsockopt(srv_sock, SOL_SOCKET, SO_REUSEADDR, 
					   (void *)&opt, sizeof(opt)));
#endif

#ifdef IP_TOS
	if (tos) {
	    int level = IPPROTO_IP;
	    int optname = IP_TOS;

	    if (bitd_sin_family(&addr) == AF_INET6) {
#if defined(IPPROTO_IPV6) && defined(IPV6_TCLASS)
		level = IPPROTO_IPV6;
		optname = IPV6_TCLASS;
#endif
	    }

	    /* Set TOS */
	    TESTTCP_SOCK_NOERROR(ts_setsockopt(srv_sock, level, optname, 
					       (void *)&tos, sizeof(tos)));
	}
#endif

        TESTTCP_SOCK_NOERROR(ts_bind(srv_sock, &addr, bitd_sockaddrlen(&addr)));

        /* Listen for connections */
        TESTTCP_SOCK_NOERROR(ts_listen(srv_sock, 1));
    }

    if (client) {
        client_sock = ts_socket(bitd_sin_family(&addr), 
				SOCK_STREAM, IPPROTO_TCP);
	if (!client_sock) {
	    fprintf(stderr, 
		    "%s: %s:%d: Cannot create client socket, %s (errno %d)\n",
		    g_prog_name, __FILE__, __LINE__,
		    strerror(bitd_socket_errno), bitd_socket_errno);
	    return 1;
	}

	if (g_verbose >= 1) {
	    printf("Client socket: %d\n", ts_get_sock_fd(client_sock));
	}

        if (ifname) {
#if defined(SO_BINDTODEVICE)
            TESTTCP_SOCK_NOERROR(ts_setsockopt(srv_sock, SOL_SOCKET, 
					       SO_BINDTODEVICE,
					       ifname, strlen(ifname) + 1));
#endif
        }

#ifdef IP_TOS
	if (tos) {
	    int level = IPPROTO_IP;
	    int optname = IP_TOS;

	    if (bitd_sin_family(&addr) == AF_INET6) {
#if defined(IPPROTO_IPV6) && defined(IPV6_TCLASS)
		level = IPPROTO_IPV6;
		optname = IPV6_TCLASS;
#endif
	    }

	    /* Set TOS */
	    TESTTCP_SOCK_NOERROR(ts_setsockopt(client_sock, level, optname, 
					       (void *)&tos, sizeof(tos)));
	}
#endif

        /* Connect */
        TESTTCP_SOCK_NOERROR(ts_connect(client_sock, 
					(struct sockaddr *)&addr, 
					bitd_sockaddrlen(&addr)));
    }

    if (server) {
        /* Accept connection */
        i = sizeof(addr); 
        accept_sock = ts_accept(srv_sock, (struct sockaddr *)&addr, 
				(socklen_t *)&i);
	if (!accept_sock) {
	    fprintf(stderr, 
		    "%s: %s:%d: Cannot accept socket, %s (errno %d)\n",
		    g_prog_name, __FILE__, __LINE__,
		    strerror(bitd_socket_errno), bitd_socket_errno);
	    return 1;
	}
	
	if (g_verbose >= 1) {
	    printf("Accept socket: %d\n", ts_get_sock_fd(accept_sock));
	}
    }

    tcp_set_ts_options(accept_sock, enq_ts, ack_ts);
    tcp_set_ts_options(client_sock, enq_ts, ack_ts);
    
    for (i = 0; i < iter; i++) {
        int size_send, size_recv;
        
        /* Compute size */
        size_send = size_send_min + ((size_send_max + 1 - size_send_min) * 
				     (bitd_random() & 0xffff)) / 0xffff;
        size_recv = size_recv_min + ((size_recv_max + 1 - size_recv_min) * 
				     (bitd_random() & 0xffff)) / 0xffff;
        
	bitd_assert(size_send <= bufsize);
        bitd_assert(size_recv <= bufsize);

	/* Sending is done first */
	if (server) {
	    tcp_send(accept_sock, buf, size_send, i, "srv", &srv_sent);
	}
	if (client) {
	    tcp_send(client_sock, buf, size_send, i, "client", &client_sent);
	}
        
	/* Receive after sending */
	if (server) {
	    tcp_recv(accept_sock, buf, size_recv, i, "srv", &srv_recvd);
	}
	if (client) {
	    tcp_recv(client_sock, buf, size_recv, i, "client", &client_recvd);
	}
    }

    /* Close the sockets */
    if (server) {
        ts_close(srv_sock);
        ts_close(accept_sock);
    } 
    if (client) {
        ts_close(client_sock);
    }

    bitd_sys_deinit();

    if (buf) {
	free(buf);
    }

    return 0;
}
