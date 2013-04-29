#ifndef PSEUDOHEADER_H
#define PSEUDOHEADER_H

/**
 * \file pseudoheader.h
 * \brief Header defining a pseudoheader used in place of a full header before it is ready
 */

/**
 * Constant declaration of the maximum size of the data buffer
 */
#define MAXBUF 10000

/**
 * \struct pseudoheader_t
 * \brief Structure describing a pseudoheader
 */
typedef struct {
	/** Buffer to hold the data */
    char data[MAXBUF];
	/** Size of the buffer */
    unsigned int size;
} pseudoheader_t;

#endif
