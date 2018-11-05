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
#include "bitd/tstamp.h"

#ifdef __linux__
# include <linux/tcp.h>
# include <linux/net_tstamp.h>
# include <linux/errqueue.h>
# include <sys/utsname.h>
# include <sys/ioctl.h>

/* Definitions in Linux kernel 4.14.44 not in Centos 7 or 
   Raspbian kernel headers */
#ifndef SCM_TSTAMP_SND
# define SCM_TSTAMP_SND 0
#endif
#ifndef SCM_TSTAMP_SCHED
# define SCM_TSTAMP_SCHED 1
#endif
#ifndef SCM_TSTAMP_ACK
# define SCM_TSTAMP_ACK 2
#endif

#ifndef SOF_TIMESTAMPING_OPT_ID
# define SOF_TIMESTAMPING_OPT_ID (1<<7)
#endif
#ifndef SOF_TIMESTAMPING_TX_SCHED
# define SOF_TIMESTAMPING_TX_SCHED (1<<8)
#endif
#ifndef SOF_TIMESTAMPING_TX_ACK
# define SOF_TIMESTAMPING_TX_ACK (1<<9)
#endif
#ifndef SOF_TIMESTAMPING_OPT_CMSG
# define SOF_TIMESTAMPING_OPT_CMSG (1<<10)
#endif
#ifndef SOF_TIMESTAMPING_OPT_TSONLY
# define SOF_TIMESTAMPING_OPT_TSONLY (1<<11)
#endif

#ifndef BITD_HAVE_STRUCT_SCM_TIMESTAMPING
struct scm_timestamping {
    struct timespec ts[3];
};
#endif

#endif

/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/
#define ts_printf if (0) printf


/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/
typedef enum {
    ts_timestamp_type_kernel,   /* Linux kernel-provided timestamps */
    ts_timestamp_type_software  /* Software-provided timestamps */
} ts_timestamp_type_t;

struct ts_socket_s {
    bitd_socket_t sock;          /* The underlying socket */
    int sock_type, sock_protocol; /* The socket type and protocol */
    bitd_socket_t tspipe;        /* The timestamp pipe, if different from sock */
    bitd_uint32 tx_count;      /* Tx frame count */
    bitd_uint32 tx_nbytes;     /* Tx byte count */
    ts_timestamp_type_t timestamp_type;
    int flags;
    bitd_mutex lock;
    struct ts_timestamp_list_s *ts_head; /* List of enqueued tx timestamps */
    struct ts_timestamp_list_s *ts_tail;
};

struct ts_timestamp_list_s {
    ts_timestamp_t ts;
    struct ts_timestamp_list_s *next;
};




/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/
#ifdef __linux__
static void enqueue_ts(ts_socket_t s,
		       int flags,
		       bitd_uint32 seq,
		       bitd_uint64 tstamp);
#endif
static void dequeue_ts(ts_socket_t s, ts_timestamp_t *ts);



/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/



/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/


/*
 *============================================================================
 *                        kernel_tstamps_supported
 *============================================================================
 * Description:     Check if platform supports kernel timestamps
 * Parameters:    
 * Returns:  
 */
#ifdef __linux__
static bitd_boolean kernel_tstamps_supported() {
    static bitd_boolean s_tstamps_supported, s_tstamps_checked;
    struct utsname utsbuf;
    int ver1, ver2, ver3;
    char *c;

    if (s_tstamps_checked) {
        return s_tstamps_supported;
    }
    
    if (getenv("BITD_SKIP_KERNEL_TSTAMPS")) {
        ts_printf("BITD_SKIP_KERNEL_TSTAMPS set, skipping kernel timestamps check\n");
        s_tstamps_checked = TRUE;
        return s_tstamps_supported;
    }

    if (!uname(&utsbuf)) {
        ver1 = atoi(utsbuf.release);
        ver2 = 0;
        ver3 = 0;
        
        c = strchr(utsbuf.release, '.');
        if (c) {
            ver2 = atoi(++c);
        }
        if (c) {
            c = strchr(c, '.');
        }
        if (c) {
            ver3 = atoi(++c);
        }
        
        ts_printf("Linux kernel %u.%u.%u\n", ver1, ver2, ver3);
        (void)ver2;
        (void)ver3;
        
        if (ver1 >= 4) {
            ts_printf("Linux timestamps supported\n");
            s_tstamps_supported = TRUE;
        } else {
            ts_printf("Linux timestamps not supported\n");
        }
    }
    
    s_tstamps_checked = TRUE;
    return s_tstamps_supported;
} 
#else
# define kernel_tstamps_supported() FALSE
#endif


/*
 *============================================================================
 *                        open_tspipe
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static bitd_socket_t open_tspipe(ts_timestamp_type_t timestamp_type, 
				 bitd_socket_t sock) {
    bitd_socket_t tspipe = BITD_INVALID_SOCKID;
    struct sockaddr_in laddr;
    bitd_socklen_t addrlen;
    
    switch (timestamp_type) {
    case ts_timestamp_type_kernel:
        /* Use sock as tspipe */
        tspipe = sock;
        break;
    case ts_timestamp_type_software:
        /* Open an UDP socket and make it non-blocking */
        tspipe = bitd_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (tspipe == BITD_INVALID_SOCKID) {
            return BITD_INVALID_SOCKID;
        }
        bitd_set_blocking(tspipe, FALSE);

        /* Bind the socket to the loopback address */
        laddr.sin_family = AF_INET;
        laddr.sin_port = 0;
        laddr.sin_addr.s_addr = htonl(0x7f000001);
        if (bitd_bind(tspipe, (struct sockaddr *)&laddr, sizeof(laddr)) < 0) {
            ts_printf("%s:%d: bitd_bind(): %s (errno %d)\n",
		      __FILE__, __LINE__, 
		      strerror(bitd_socket_errno), bitd_socket_errno);
            bitd_close(tspipe);
            return BITD_INVALID_SOCKID;
        }

        /* Get socket's local address */
        addrlen = sizeof(laddr);
        if (bitd_getsockname(tspipe, (struct sockaddr *)&laddr, 
			   (bitd_socklen_t *)&addrlen) < 0) {
            ts_printf("%s:%d: bitd_getsockname(): %s (errno %d)\n",
		      __FILE__, __LINE__, 
		      strerror(bitd_socket_errno), bitd_socket_errno);
            bitd_close(tspipe);
            return BITD_INVALID_SOCKID;
        }
        
        /* Connect socket to itself */
        if (bitd_connect(tspipe, (struct sockaddr *)&laddr, 
		       sizeof(laddr)) < 0) {
            ts_printf("%s:%d: bitd_connect(): %s (errno %d)\n",
                       __FILE__, __LINE__, 
                       strerror(bitd_socket_errno), bitd_socket_errno);
            bitd_close(tspipe);
            return BITD_INVALID_SOCKID;
        }
        break;
        
    default:
        bitd_assert(0);
    }

    return tspipe;
} 


/*
 *============================================================================
 *                        ts_socket
 *============================================================================
 * Description:  
 * Parameters:    
 * Returns:  
 */
ts_socket_t ts_socket(int domain, int type, int protocol) {
    bitd_socket_t sock;
    bitd_socket_t tspipe;
    ts_socket_t s;
    ts_timestamp_type_t timestamp_type = ts_timestamp_type_software;
    
    if (kernel_tstamps_supported()) {
        timestamp_type = ts_timestamp_type_kernel;
    }
    
    sock = bitd_socket(domain, type, protocol);
    if (sock == BITD_INVALID_SOCKID) {
        return NULL;
    }
    
#ifdef __linux__
    if (timestamp_type == ts_timestamp_type_kernel) {
        /* Enable kernel timestamps on this socket */
        int opt;

        /* Enable rx timestamps */
        opt = 1;
        if (bitd_setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPNS,
			  &opt, sizeof(opt)) < 0) {
            ts_printf("%s:%d: bitd_setsockopt(): %s (errno %d)\n",
                       __FILE__, __LINE__, 
		      strerror(bitd_socket_errno), bitd_socket_errno);
            bitd_close(sock);
            return NULL;
        }
        
        /* Enable tx timestamps */
	opt = SOF_TIMESTAMPING_SOFTWARE |
            SOF_TIMESTAMPING_OPT_CMSG |
            SOF_TIMESTAMPING_OPT_TSONLY |
            SOF_TIMESTAMPING_TX_SOFTWARE;

        if (type != SOCK_STREAM) {
            /* Linux does not allow this option until socket is connected */
            opt |= SOF_TIMESTAMPING_OPT_ID;
        }
        
	if (bitd_setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPING,
			  (char *)&opt, sizeof(opt)) < 0) {
            ts_printf("%s:%d: bitd_setsockopt(): %s (errno %d)\n",
		      __FILE__, __LINE__, 
		      strerror(bitd_socket_errno), bitd_socket_errno);
            bitd_close(sock);
            return NULL;
        }
    }
#endif

    tspipe = open_tspipe(timestamp_type, sock);
    if (tspipe == BITD_INVALID_SOCKID) {
        bitd_close(sock);
        return NULL;
    }

    s = calloc(1, sizeof(*s));
    s->sock = sock;
    s->sock_type = type;
    s->sock_protocol = protocol;
    s->tspipe = tspipe;    
    s->timestamp_type = timestamp_type;
    s->flags = TS_FLAG_SND;
    s->lock = bitd_mutex_create();

    return s;
} 


/*
 *============================================================================
 *                        ts_close
 *============================================================================
 * Description:  Destroy a timestamp socket   
 * Parameters:    
 * Returns:  
 */
void ts_close(ts_socket_t s) {
    
    if (s) {
        if (s->sock != BITD_INVALID_SOCKID) {
            bitd_close(s->sock);
        }

        switch (s->timestamp_type) {
        case ts_timestamp_type_kernel:
            break;
        case ts_timestamp_type_software:
            bitd_close(s->tspipe);
            break;
        default:
            bitd_assert(0);
        }

        /* Empty the queue */
        while (s->ts_head) {
            dequeue_ts(s, NULL);
        }

        bitd_mutex_destroy(s->lock);
        free(s);
    }
}


/*
 *============================================================================
 *                        ts_get_sock_fd
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int ts_get_sock_fd(ts_socket_t s) {
    
    if (s) {
        return s->sock;
    }
    
    return BITD_INVALID_SOCKID;
} 


/*
 *============================================================================
 *                        ts_get_tstamp_fd
 *============================================================================
 * Description:     Get the timestamp pipe descriptor
 * Parameters:    
 * Returns:  
 */
int ts_get_tstamp_fd(ts_socket_t s) {
    
    if (s) {
        return s->tspipe;
    }
    
    return BITD_INVALID_SOCKID;
} 


/*
 *============================================================================
 *                        ts_connect
 *============================================================================
 * Description:  Connect a timestamp socket   
 * Parameters:    
 * Returns:  
 */
int ts_connect(ts_socket_t s, 
	       struct sockaddr *addr, bitd_socklen_t addrlen) {
    int ret;

    if (!s || s->sock == BITD_INVALID_SOCKID) {
        return -1;
    }
    
    ret = connect(s->sock, addr, addrlen); 
    if (ret < 0) {
        return ret;
    }

#ifdef __linux__
    if (s->sock_type == SOCK_STREAM &&
        s->timestamp_type == ts_timestamp_type_kernel) {
	int opt;
	bitd_socklen_t len = sizeof(opt);

        if (bitd_getsockopt(s->sock, SOL_SOCKET, SO_TIMESTAMPING,
			  (char *)&opt, &len) < 0) {
            ts_printf("%s:%d: bitd_getsockopt(): %s (errno %d)\n",
		      __FILE__, __LINE__, 
		      strerror(bitd_socket_errno), bitd_socket_errno);
            return -1;
        }
	
        /* Linux does not permit to set this for tcp 
	   until socket is connected */
        opt |= SOF_TIMESTAMPING_OPT_ID;
	
        if (bitd_setsockopt(s->sock, SOL_SOCKET, SO_TIMESTAMPING,
			  (char *)&opt, sizeof(opt)) < 0) {
            ts_printf("%s:%d: bitd_setsockopt(): %s (errno %d)\n",
		      __FILE__, __LINE__, 
		      strerror(bitd_socket_errno), bitd_socket_errno);
            return -1;
        }
    }
#endif
    
    return ret;
}


/*
 *============================================================================
 *                        ts_accept
 *============================================================================
 * Description:  Accept a timestamp socket   
 * Parameters:    
 * Returns:  
 */
ts_socket_t ts_accept(ts_socket_t s, 
		      struct sockaddr *addr, socklen_t *addrlen) {
    bitd_socket_t asock;
    bitd_socket_t atspipe;
    ts_socket_t as;

    if (!s || s->sock == BITD_INVALID_SOCKID) {
        return NULL;
    }
    
    asock = accept(s->sock, addr, addrlen); 
    if (asock == BITD_INVALID_SOCKID) {
        return NULL;
    }

#ifdef __linux__
    if (s->timestamp_type == ts_timestamp_type_kernel) {
	int opt;
	bitd_socklen_t len = sizeof(opt);

        if (bitd_getsockopt(asock, SOL_SOCKET, SO_TIMESTAMPING,
			  (char *)&opt, &len) < 0) {
            ts_printf("%s:%d: bitd_getsockopt(): %s (errno %d)\n",
		      __FILE__, __LINE__, 
		      strerror(bitd_socket_errno), bitd_socket_errno);
            bitd_close(asock);
            return NULL;
        }
	
        /* Linux does not permit to set this for tcp until 
           the socket is connected */
        opt |= SOF_TIMESTAMPING_OPT_ID;
        
        if (bitd_setsockopt(asock, SOL_SOCKET, SO_TIMESTAMPING,
			  (char *)&opt, sizeof(opt)) < 0) {
            ts_printf("%s:%d: bitd_setsockopt(): %s (errno %d)\n",
		      __FILE__, __LINE__, 
		      strerror(bitd_socket_errno), bitd_socket_errno);
            bitd_close(asock);
            return NULL;
        }
    }
#endif

    atspipe = open_tspipe(s->timestamp_type, asock);
    if (atspipe == BITD_INVALID_SOCKID) {
        bitd_close(asock);
        return NULL;
    }
    
    as = calloc(1, sizeof(*s));
    as->sock = asock;
    as->sock_type = s->sock_type;
    as->sock_protocol = s->sock_protocol;
    as->timestamp_type = s->timestamp_type;
    as->tspipe = atspipe;
    as->flags = s->flags;
    as->lock = bitd_mutex_create();
    
    return as;
}


/*
 *============================================================================
 *                        ts_send_software_tstamp
 *============================================================================
 * Description:     
 * Parameters:    
 *     s - The timestamp handle
 *     send_len - The number of bytes just sent
 *     send_time - The time of the send
 *     ts_flags - The requested timestamp flags
 * Returns:  
 */
static int send_software_tstamp(ts_socket_t s, 
                                int send_len,
                                bitd_uint64 send_time) {
    ts_timestamp_t ts;

    /* This is a software timestamp */
    ts.flags = 0;
    
    /* The send time */
    ts.tstamp = send_time;

    /* The send sequence */
    if (s->sock_type == SOCK_STREAM) {
        ts.seq = s->tx_nbytes + send_len - 1;
    } else {
        ts.seq = s->tx_count;
    }
    
    if (s->flags & TS_FLAG_ENQ) {
        /* Send the timestamp as 'enqueue' timestamp */
        ts.flags = TS_FLAG_SW|TS_FLAG_ENQ;
        if (bitd_send(s->tspipe, (void *)&ts, sizeof(ts), 0) < 0) {
            ts_printf("%s:%d: bitd_send(s->tspipe): %s (errno %d)\n",
                       __FILE__, __LINE__, 
                       strerror(bitd_socket_errno), bitd_socket_errno);
            return -1;
        }
    }
    if (s->flags & TS_FLAG_SND) {
        /* Send the timestamp as 'send' timestamp */
        ts.flags = TS_FLAG_SW|TS_FLAG_SND;
        if (bitd_send(s->tspipe, (void *)&ts, sizeof(ts), 0) < 0) {
            ts_printf("%s:%d: bitd_send(s->tspipe): %s (errno %d)\n",
                       __FILE__, __LINE__, 
                       strerror(bitd_socket_errno), bitd_socket_errno);
            return -1;
        }
    }
    if ((s->sock_type == SOCK_STREAM) && (s->flags & TS_FLAG_ACK)) {
        /* Send the timestamp as 'ack' timestamp */
        ts.flags = TS_FLAG_SW|TS_FLAG_ACK;
        if (bitd_send(s->tspipe, (void *)&ts, sizeof(ts), 0) < 0) {
            ts_printf("%s:%d: bitd_send(s->tspipe): %s (errno %d)\n",
                       __FILE__, __LINE__, 
                       strerror(bitd_socket_errno), bitd_socket_errno);
            return -1;
        }
    }
    
    return 0;
} 


/*
 *============================================================================
 *                        ts_send
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int ts_send(ts_socket_t s, const void *msg, int len, int flags){
    int ret = 0;
    bitd_uint64 current_time;

    if (!s || s->sock == BITD_INVALID_SOCKID) {
        return -1;
    }
        
    switch (s->timestamp_type) {
    case ts_timestamp_type_kernel:
        ret = bitd_send(s->sock, msg, len, flags);
        break;
    case ts_timestamp_type_software:
        /* Remember the current time */        
        current_time = bitd_get_time_nsec(); 
               
        /* Send the packet */
        ret = bitd_send(s->sock, msg, len, flags);
        if (ret <= 0) {
            return ret;
        }

        /* Send the timestamp */
        if (send_software_tstamp(s, ret, current_time) < 0) {
            return -1;
        }
        break;
    default:
        bitd_assert(0);
    }

    if (ret <= 0) {
        return ret;
    }

    /* Remember actual number of transmits and transmit bytes */
    s->tx_count++;
    s->tx_nbytes += ret;

    return ret;
} 


/*
 *============================================================================
 *                        ts_sendto
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int ts_sendto(ts_socket_t s, const void *msg, int len, int flags,
	      const struct sockaddr *to, int tolen){
    int ret = 0;
    bitd_uint64 current_time;

    if (!s || s->sock == BITD_INVALID_SOCKID) {
        return -1;
    }

    switch (s->timestamp_type) {
    case ts_timestamp_type_kernel:
        ret = bitd_sendto(s->sock, msg, len, flags, to, tolen);
        break;
    case ts_timestamp_type_software:
        /* Remember the current time */        
        current_time = bitd_get_time_nsec();

        /* Send the packet */
        ret = bitd_sendto(s->sock, msg, len, flags, to, tolen);
        if (ret <= 0) {
            return ret;
        }

        /* Send the timestamp */
        if (send_software_tstamp(s, ret, current_time) < 0) {
            return -1;
        }
        break;
    default:
        bitd_assert(0);
    }

    if (ret <= 0) {
        return ret;
    }

    /* Remember actual number of transmits and transmit bytes */
    s->tx_count++;
    s->tx_nbytes += ret;

    return ret;
} 


/*
 *============================================================================
 *                        ts_recv
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int ts_recv(ts_socket_t s, void *buf, int len, int flags,
	    ts_timestamp_t *ts) {
    int ret = 0;
#ifdef __linux__
    struct msghdr msg;
    struct iovec entry;
    struct {
        struct cmsghdr cm;
        char control[512];
    } control;    
    struct cmsghdr *cmsg;
#endif

    if (!s || s->sock == BITD_INVALID_SOCKID) {
        return -1;
    }

    if (ts) {
        memset(ts, 0, sizeof(*ts));
    }

    switch (s->timestamp_type) {
    case ts_timestamp_type_kernel:
#ifdef __linux__
        /* Use rcvmsg to receive */
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = &entry;
        msg.msg_iovlen = 1;
        entry.iov_base = buf;
        entry.iov_len = len;
        msg.msg_control = &control;
        msg.msg_controllen = sizeof(control);
        
        ret = recvmsg(s->sock, &msg, flags);
        if (ret < 0) {
            return ret;
        }

        /* Read the timestamp from the cmsghdr */
        for (cmsg = CMSG_FIRSTHDR(&msg);
             cmsg;
             cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            struct timespec *stamp;

            ts_printf("   cmsg len %zu:\n", cmsg->cmsg_len);
            switch (cmsg->cmsg_level) {
            case SOL_SOCKET:
                ts_printf("SOL_SOCKET\n");
                switch (cmsg->cmsg_type) {
                case SO_TIMESTAMPNS:
                    stamp = (struct timespec *)CMSG_DATA(cmsg);
                    
                    ts_printf("SO_TIMESTAMPNS %ld.%09ld\n",
                               (long)stamp->tv_sec,
                               (long)stamp->tv_nsec);
		    
		    if (ts) {
			ts->tstamp = 1000000000ULL*stamp->tv_sec + 
			    stamp->tv_nsec;
			ts->flags |= TS_FLAG_KERNEL;
		    }
                    break;
                case SO_TIMESTAMP:
                    ts_printf("SO_TIMESTAMP\n");
                    break;
                case SO_TIMESTAMPING:
                    ts_printf("SO_TIMESTAMPING\n");
                    break;
                default:
                    ts_printf("type %d\n", cmsg->cmsg_type);
                    break;
                }
                break;
            default:
                ts_printf("level %d type %d\n",
                           cmsg->cmsg_level,
                           cmsg->cmsg_type);
                break;
            }
        }
#endif
        break;
    case ts_timestamp_type_software:

        ret = recv(s->sock, buf, len, flags);
        if (ret < 0) {
            return ret;
        }

        break;
    default:
        bitd_assert(0);
    }

#ifdef SIOCGSTAMPNS
    if (ts && !ts->flags) {
        struct timespec stamp;
        
        if (ioctl(s->sock, SIOCGSTAMPNS, &stamp)) {
            ts_printf("   %s: %s\n", "SIOCGSTAMPNS", strerror(errno));
            /* Clear the errno */
            errno = 0;
        } else {
            ts_printf("SIOCGSTAMPNS %ld.%09ld ns\n",
                       (long)stamp.tv_sec,
                       (long)stamp.tv_nsec);
            
            ts->tstamp = 1000000000ULL*stamp.tv_sec + stamp.tv_nsec;
            ts->flags = TS_FLAG_KERNEL;
        }
    }
#endif /* SIOCGSTAMPNS */
    
    if (ts && !ts->flags) {
        /* Get the current time */        
        ts->tstamp = bitd_get_time_nsec();
        ts->flags = TS_FLAG_SW;
    }        

    return ret;
} 
        

/*
 *============================================================================
 *                        ts_recvfrom
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int ts_recvfrom(ts_socket_t s, void *buf, int len, int flags,
		struct sockaddr *from, int *fromlen,
		ts_timestamp_t *ts) {
    int ret = 0;
#ifdef __linux__
    struct msghdr msg;
    struct iovec entry;
    struct {
        struct cmsghdr cm;
        char control[512];
    } control;    
    struct cmsghdr *cmsg;
#endif

    if (!s || s->sock == BITD_INVALID_SOCKID) {
        return -1;
    }

    if (ts) {
        memset(ts, 0, sizeof(*ts));
    }

    switch (s->timestamp_type) {
    case ts_timestamp_type_kernel:
#ifdef __linux__
        /* Use rcvmsg to receive */
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = &entry;
        msg.msg_iovlen = 1;
        msg.msg_name = from;
        msg.msg_namelen = *fromlen;
        entry.iov_base = buf;
        entry.iov_len = len;
        msg.msg_control = &control;
        msg.msg_controllen = sizeof(control);
        
        ret = recvmsg(s->sock, &msg, flags);
        if (ret < 0) {
            return ret;
        }

        /* Read the timestamp from the cmsghdr */
        for (cmsg = CMSG_FIRSTHDR(&msg);
             cmsg;
             cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            struct timespec *stamp;

            ts_printf("   cmsg len %zu:\n", cmsg->cmsg_len);
            switch (cmsg->cmsg_level) {
            case SOL_SOCKET:
                ts_printf("SOL_SOCKET\n");
                switch (cmsg->cmsg_type) {
                case SO_TIMESTAMPNS:
                    stamp = (struct timespec *)CMSG_DATA(cmsg);
                    
                    ts_printf("SO_TIMESTAMPNS %ld.%09ld\n",
                               (long)stamp->tv_sec,
                               (long)stamp->tv_nsec);
                                        
                    ts->tstamp = 1000000000ULL*stamp->tv_sec + stamp->tv_nsec;
                    ts->flags |= TS_FLAG_KERNEL;
                    break;
                case SO_TIMESTAMP:
                    ts_printf("SO_TIMESTAMP\n");
                    break;
                case SO_TIMESTAMPING:
                    ts_printf("SO_TIMESTAMPING\n");
                    break;
                default:
                    ts_printf("type %d\n", cmsg->cmsg_type);
                    break;
                }
                break;
            default:
                ts_printf("level %d type %d\n",
                           cmsg->cmsg_level,
                           cmsg->cmsg_type);
                break;
            }
        }
#endif
        break;
    case ts_timestamp_type_software:

        ret = bitd_recvfrom(s->sock, buf, len, flags, from, (bitd_socklen_t *)fromlen);
        if (ret < 0) {
            return ret;
        }
        break;
    default:
        bitd_assert(0);
    }

#ifdef SIOCGSTAMPNS
    if (!ts->flags) {
        struct timespec stamp;
        
        if (ioctl(s->sock, SIOCGSTAMPNS, &stamp)) {
            ts_printf("   %s: %s\n", "SIOCGSTAMPNS", strerror(errno));
            /* Clear the errno */
            errno = 0;
        } else {
            ts_printf("SIOCGSTAMPNS %ld.%09ld ns\n",
                       (long)stamp.tv_sec,
                       (long)stamp.tv_nsec);
            
            ts->tstamp = 1000000000ULL*stamp.tv_sec + stamp.tv_nsec;
            ts->flags = TS_FLAG_KERNEL;
        }
    }
#endif /* SIOCGSTAMPNS */
    
    if (!ts->flags) {
        /* Get the current time */        
        ts->tstamp = bitd_get_time_nsec();
        ts->flags = TS_FLAG_SW;
    }        

    return ret;
} 

#ifdef __linux__
static void __print_timestamp(const char *name, struct timespec *cur,
			      uint32_t key, int payload_len)
{
    if (!(cur->tv_sec | cur->tv_nsec))
        return;
    
    fprintf(stderr, "  %s: %lu s %lu us (seq=%u, len=%u)\n",
            name, cur->tv_sec, cur->tv_nsec / 1000,
            key, payload_len);
}

static void print_timestamp(struct scm_timestamping *tss, int tstype,
			    int tskey, int payload_len)
{
    const char *tsname;

    switch (tstype) {
    case SCM_TSTAMP_SCHED:
        tsname = "  ENQ";
        break;
    case SCM_TSTAMP_SND:
        tsname = "  SND";
        break;
    case SCM_TSTAMP_ACK:
        tsname = "  ACK";
        break;
    default:
        bitd_assert(0);
}
    __print_timestamp(tsname, &tss->ts[0], tskey, payload_len);
}


/*
 *============================================================================
 *                        enqueue_ts
 *============================================================================
 * Description:     Enqueue a timestamp. 
 * Parameters:    
 * Returns:  
 */
 void enqueue_ts(ts_socket_t s,
		 int flags,
		 bitd_uint32 seq,
		 bitd_uint64 tstamp) {
    struct ts_timestamp_list_s *tsl;
    
    tsl = malloc(sizeof(*tsl));
    tsl->next = NULL;

    bitd_mutex_lock(s->lock);
    if (!s->ts_head) {
	s->ts_head = tsl;
    }
    if (s->ts_tail) {
	s->ts_tail->next = tsl;
    }
    s->ts_tail = tsl;

    tsl->ts.flags = flags;
    tsl->ts.seq = seq;
    tsl->ts.tstamp = tstamp;
    bitd_mutex_unlock(s->lock);
}
#endif /* __linux__ */


 /*
  *============================================================================
  *                        dequeue_ts
  *============================================================================
  * Description:     Dequeue a timestamp. 
  * Parameters:    
  *     s - the timestamp handle
  *     ts [OUT] - pointer to where to copy the timestamp to
  * Returns:  Timestamp is dequeued if ts->flags is non-zero
  */
 void dequeue_ts(ts_socket_t s, ts_timestamp_t *ts) {
    struct ts_timestamp_list_s *tsl;
    
    bitd_mutex_lock(s->lock);
    tsl = s->ts_head;
    if (tsl) {
	if (ts) {
	    memcpy(ts, &tsl->ts, sizeof(*ts));
	}
	s->ts_head = tsl->next;
	if (!s->ts_head) {
	    s->ts_tail = NULL;
	}
	free(tsl);
    }
    bitd_mutex_unlock(s->lock);
}


/*
 *============================================================================
 *                        ts_get_tstamp_tx
 *============================================================================
 * Description:     Get a tx timestamp
 * Parameters:    
 * Returns:  
 */
 int ts_get_tstamp_tx(ts_socket_t s, ts_timestamp_t *ts) {
     int ret = 0;

     if (!s || s->sock == BITD_INVALID_SOCKID || !ts) {
	 return -1;
     }

     memset(ts, 0, sizeof(*ts));

     /* Do we have any enqueued timestamps? */
     dequeue_ts(s, ts);
    
     if (ts->flags) {
	 /* We dequeued a timestamp - return it */
	 return 0;
     }

     switch (s->timestamp_type) {
     case ts_timestamp_type_kernel:        
#ifdef __linux__
	 {
	     char ctrl[1024 /* overprovision*/];
	     struct msghdr msg;
	     struct iovec entry;
	     char data[10];
	     struct sock_extended_err *serr = NULL;
	     struct scm_timestamping *tss = NULL;
	     struct cmsghdr *cm;
	     int batch = 0;
	     bitd_uint64 tstamp;
	     unsigned int flags;

	     memset(&msg, 0, sizeof(msg));
	     memset(&entry, 0, sizeof(entry));
	     memset(ctrl, 0, sizeof(ctrl));

	     entry.iov_base = data;
	     entry.iov_len = sizeof(data);
	     msg.msg_iov = &entry;
	     msg.msg_iovlen = 1;
	     msg.msg_name = NULL;
	     msg.msg_namelen = 0;
	     msg.msg_control = ctrl;
	     msg.msg_controllen = sizeof(ctrl);

	     ret = recvmsg(s->tspipe, &msg, MSG_ERRQUEUE);
	     if (ret < 0 && errno != EAGAIN) {
		 ts_printf("%s:%d: recvmsg(s->tspipe): %s (errno %d)\n",
			   __FILE__, __LINE__, 
			   strerror(bitd_socket_errno), bitd_socket_errno);
		 return ret;
	     }

	     /* Clear the errno */
	     errno = 0;

	     for (cm = CMSG_FIRSTHDR(&msg);
		  cm && cm->cmsg_len;
		  cm = CMSG_NXTHDR(&msg, cm)) {
		 if (cm->cmsg_level == SOL_SOCKET &&
		     cm->cmsg_type == SCM_TIMESTAMPING) {
		     tss = (void *) CMSG_DATA(cm);
		 } else if ((cm->cmsg_level == SOL_IP &&
			     cm->cmsg_type == IP_RECVERR) ||
			    (cm->cmsg_level == SOL_IPV6 &&
			     cm->cmsg_type == IPV6_RECVERR)) {
		     serr = (void *) CMSG_DATA(cm);
		     if (serr->ee_errno != ENOMSG ||
			 serr->ee_origin != SO_EE_ORIGIN_TIMESTAMPING) {
			 fprintf(stderr, "unknown ip error %d %d\n",
				 serr->ee_errno,
				 serr->ee_origin);
			 serr = NULL;
		     }
		 }
            
		 if (serr && tss) {
		     /* Commented out unless we need to debug */
		     if (0) print_timestamp(tss, serr->ee_info, serr->ee_data, 
					    sizeof(data));

		     /* Compute the timestamp flags */
		     flags = TS_FLAG_KERNEL;
		     switch (serr->ee_info) {
		     case SCM_TSTAMP_SCHED:
			 flags |= TS_FLAG_ENQ;
			 break;
		     case SCM_TSTAMP_SND:
			 flags |= TS_FLAG_SND;
			 break;
		     case SCM_TSTAMP_ACK:
			 flags |= TS_FLAG_ACK;
			 break;
		     default:
			 bitd_assert(0);
		     }

		     /* Compute the timestamp time */
		     tstamp = tss->ts[0].tv_nsec + 1000000000ULL*tss->ts[0].tv_sec;

		     if (ts->flags) {
			 /* Timestamp already copied to OUT parameter.
			    Enqueue timestamp */
			 enqueue_ts(s,
				    flags,
				    serr->ee_data,
				    tstamp);
		     } else {
			 /* Copy timestamp to OUT parameter */
			 ts->flags = flags;
			 ts->seq = serr->ee_data;
			 ts->tstamp = tstamp;
		     }
		     serr = NULL;
		     tss = NULL;
		     batch++;
		 }
	     }
        
	     if (batch > 1) {
		 ts_printf("Batched %d timestamps\n", batch);
	     }
	 }
#endif
	 break;
     case ts_timestamp_type_software:
	 ret = bitd_recv(s->tspipe, (void *)ts, sizeof(*ts), 0);
	 if (ret != sizeof(*ts)) {
	     ret = -1;
            
	     /* Clear the errno */
	     errno = 0;
	 }
	 break;
     default:
	 bitd_assert(0);
     }

     return ret;
 } 


 /*
  *============================================================================
  *                        ts_getsockopt
  *============================================================================
  * Description:     
  * Parameters:    
  * Returns:  
  */
 int ts_getsockopt(ts_socket_t s, int level, int optname, 
                   void* optval, bitd_socklen_t *optlen) {    

     if (!s || s->sock == BITD_INVALID_SOCKID || !optval || !optlen) {
	 return -1;
     }

     if (level == SOL_TS_SOCKET) {
	 if (optname == SO_TS_FLAGS) {
	     if (*optlen < sizeof(int)) {
		 return -1;
	     }
            
	     /* Copy atomically the flags */
	     *(int *)optval = (s->flags & (TS_FLAG_ENQ|TS_FLAG_SND|TS_FLAG_ACK));
	     return 0;
	 } else {
	     /* Option not supported */
	     return -1;
	 }
     }

     return getsockopt(s->sock, level, optname, optval, (bitd_socklen_t *)optlen);
 } 


 /*
  *============================================================================
  *                        ts_setsockopt
  *============================================================================
  * Description:     
  * Parameters:    
  * Returns:  
  */
 int ts_setsockopt(ts_socket_t s, int level, int optname, 
                   void *optval, bitd_socklen_t optlen) {
     int sflags;

     if (!s || s->sock == BITD_INVALID_SOCKID || !optval ) {
	 return -1;
     }

     if (level == SOL_TS_SOCKET) {
	 if (optname == SO_TS_FLAGS) {
	     if (optlen < sizeof(int)) {
		 return -1;
	     }
            
	     sflags = s->flags & (TS_FLAG_ENQ|TS_FLAG_SND|TS_FLAG_ACK);
	     sflags |= *(int *)optval;
            
	     /* Copy atomically the flags */
	     s->flags = sflags;

#ifdef __linux__
	     if (s->timestamp_type == ts_timestamp_type_kernel) {
		 int opt = 0;
		 bitd_socklen_t len = sizeof(opt);

		 /* Pass the socket options down */
		 if (bitd_getsockopt(s->sock, SOL_SOCKET, SO_TIMESTAMPING,
                                   (char *)&opt, &len) < 0) {
		     ts_printf("%s:%d: bitd_getsockopt(): %s (errno %d)\n",
                               __FILE__, __LINE__, 
                               strerror(bitd_socket_errno), bitd_socket_errno);
		     return -1;
		 }
                
		 if (s->flags & TS_FLAG_ENQ) {
		     opt |= SOF_TIMESTAMPING_TX_SCHED;
		 } else {
		     opt &= ~SOF_TIMESTAMPING_TX_SCHED;
		 }
		 if (s->flags & TS_FLAG_SND) {
		     opt |= SOF_TIMESTAMPING_TX_SOFTWARE;
		 } else {
		     opt &= ~SOF_TIMESTAMPING_TX_SOFTWARE;
		 }
		 if (s->sock_type == SOCK_STREAM) {
		     if (s->flags & TS_FLAG_ACK) {
			 opt |= SOF_TIMESTAMPING_TX_ACK;
		     } else {
			 opt &= ~SOF_TIMESTAMPING_TX_ACK;
		     }
		 }

		 if (bitd_setsockopt(s->sock, SOL_SOCKET, SO_TIMESTAMPING,
                                   (char *)&opt, sizeof(opt)) < 0) {
		     ts_printf("%s:%d: bitd_setsockopt(): %s (errno %d)\n",
                               __FILE__, __LINE__, 
                               strerror(bitd_socket_errno), bitd_socket_errno);
		     return -1;
		 }
	     }
#endif

	     return 0;
	 } else {
	     /* Option not supported */
	     return -1;
	 }
     }

     return setsockopt(s->sock, level, optname, optval, optlen);
 } 

