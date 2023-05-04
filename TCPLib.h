/**
 * Declares functions useful for transmitting
 * and receiving information using TCP.
 */

#ifndef TCP_LIB
#define TCP_LIB

#include <stdio.h>      /* Contains common I/O functions */
#include <sys/types.h>  /* Contains definitions of data types used in system calls */
#include <sys/socket.h> /* Includes definitions of structures needed for sockets */
#include <netinet/in.h> /* Contains constants and structures needed for internet domain addresses. */
#include <unistd.h>     /* Contains standard unix functions */
#include <stdlib.h>     /* For atoi() and exit() */
#include <string.h> 	/* For memset(), c.str() */
#include <arpa/inet.h>  /* For inet_pton() */
#include <time.h>       /* For retrieving the time */
#include <limits.h>	    /* For the maximum integer size */
#include <cstdlib>  
#include <cstddef>

/* The size of the buffer size message */
#define SIZE_MSG_SIZE 100

/**
 * Keeps transmitting a buffer until the buffer is transmitted
 * @param socket - the socket to use for sending
 * @param buffer - the buffer to send
 * @param buffSize - the size of the buffer
 * @param progress_callback - a callback function to call after each transmission
 * @return - the number of bytes transmitted, or -1 on error 
 */
int tcp_send(int socket, const void* buffer, int buffSize, void (*progress_callback)(long long, long long) = nullptr);


/**
 * Keeps receiving a buffer until all bytes are received
 * @param socket - the socket to use for sending
 * @param buffer - the buffer to send
 * @param buffSize - the size of the buffer
 * @param progress_callback - a callback function to call after each transmission
 * @return - the number of bytes transmitted, or -1 on error 
 */
int tcp_recv(int socket, void* buffer, int buffSize, void (*progress_callback)(long long, long long) = nullptr);

#endif
