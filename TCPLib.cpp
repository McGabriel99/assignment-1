/**
 * Implements TCPLib
 */

#include "TCPLib.h"

/*
 * Keeps transmitting a buffer until the buffer is transmitted
 * @param socket - the socket to use for sending
 * @param buffer - the buffer to send
 * @param buffSize - the size of the buffer
 * @param progress_callback - a callback function to call after each transmission
 * @return - the number of bytes transmitted, or -1 on error 
 */
int tcp_send(int socket, const void* buffer, int buffSize, void (*progress_callback)(long long, long long))
{
	/* @numSent - the number of bytes sent in one shot
	 * @totalBytesSent - the cumulative number of bytes sent
	 */
	int numSent = 0, totalBytesSent = 0;
	
	/* Keep transmitting until all is transmitted */
	while(totalBytesSent != buffSize)
	{
		/* Transmit the buffer */
		if((numSent = write(socket, (char*)buffer + totalBytesSent, buffSize - totalBytesSent)) < 0)
		{
			/* Something went wrong while transmitting */
			return numSent;
		}
		
		/* Update the total number of bytes sent */
		totalBytesSent += numSent;
	}
		
	/* Successful transmission */	
	return totalBytesSent;
}


/**
 * Keeps receiving a buffer until all bytes are received
 * @param socket - the socket to use for receiving
 * @param buffer - the buffer to send
 * @param buffSize - the size of the buffer
 * @param progress_callback - a callback function to call after each transmission
 * @return - the number of bytes received, or -1 on error 
 */
int tcp_recv(int socket, void* buffer, int buffSize, void (*progress_callback)(long long, long long))
{
	int total_received = 0;
    int received;
    char *buf = static_cast<char *>(buffer);

	// Keep receiving until all is received
    while (total_received < buffSize) {
		// recv() returns the number of bytes received, or -1 if an error occurred
        received = recv(socket, buf + total_received, buffSize - total_received, 0);
        if (received <= 0) {
            break;
        }
        total_received += received;

		// Call the callback function, if it exists
        if (progress_callback) {
            progress_callback(total_received, buffSize);
        }
    }
	// Return the number of bytes received, or -1 if an error occurred
    return (received < 0) ? -1 : total_received;
	
}



