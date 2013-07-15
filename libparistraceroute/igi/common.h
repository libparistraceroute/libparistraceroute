/*
 * Copyright (c) 2006
 * Ningning Hu and the Carnegie Mellon University.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author(s) may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.  
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#define BUF_SIZE		4096
#define MAXIPSTRLEN		(16)

#define START_PORT 		(10*1024) 
#define END_PORT   		(0xFFFF)

// TODO This is the default value used by each option
#define ProbeNum 		(60)
#define PacketSize		(500)
#define MaxProbeNum		(256)

/* the maximum calm period of a probing client */
#define CLIENTTIMEOUT   	(30)

/* used by sanity_check */
#define BinWidth        	(0.000025)

/* used by binary_probing & probing */
#define SmallDelay              (50)
#define BigDelay              	(500)
#define DelayStep               (100)
#define MaxPhaseNum             (1024)
#define TurningGapAdjustment    (0.0001)
#define TurningIndexAdjustment  (2)

/* max number of repeast for each probing gap value */
#define MaxRepeat		20

#ifdef SUN
#define MAX(a, b)		((a)>(b)?(a):(b))
#endif

typedef unsigned int 		uint32;
typedef unsigned short int	uint16;

/**
 * This is the data measurement returned by the server to the client 
 */

struct pkt_rcd_t {
    // 1000000 * sec + u_sec = recv_date_usec, see get_rcd_time
	uint32 sec;   /**< The received date (s part) */
	uint32 u_sec; /**< The received date (us part */
	uint32 seq;   /**< Sequence number of the packet */
}; 
 
#endif /* !_COMMON_H_ */
